#include "ForestIsometricPlayer.h"
#include "ForestElementRegions.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
 bool NaturalClimbSurface(const FHitResult& Hit)
 {
  auto* Component=Hit.GetComponent();auto* Actor=Hit.GetActor();
  if(!Component || !Actor || Component->Mobility!=EComponentMobility::Static || Component->ComponentHasTag(TEXT("ForestNoClimb")) || Actor->ActorHasTag(TEXT("ForestNoClimb")))return false;
  if(Component->ComponentHasTag(TEXT("ForestClimbable")) || Actor->ActorHasTag(TEXT("ForestClimbable")))return true;
  if(Actor->GetClass()->GetName().Contains(TEXT("Landscape")))return true;
  const auto* Mesh=Cast<UStaticMeshComponent>(Component);if(!Mesh || !Mesh->GetStaticMesh())return false;
  const FString Name=Mesh->GetStaticMesh()->GetName();
  return Name.Contains(TEXT("Rock")) || Name.Contains(TEXT("Cliff")) || Name.Contains(TEXT("Terrain")) || Name.Contains(TEXT("Ground")) || Name.Contains(TEXT("Boulder"));
 }
}

void UForestIsometricPlayer::InitializeActions()
{
 // Called by the component constructor: hard references retain the assets in cooks.
 for(const TCHAR* Name:{TEXT("GatherCrouchedStart"),TEXT("GatherCrouchedLoop"),TEXT("GatherCrouchedEnd"),TEXT("GatherStandingStart"),TEXT("GatherStandingLoop"),TEXT("GatherStandingEnd")})
 {
  const FString Path=FString::Printf(TEXT("/Game/Characters/Actions/AS_%s.AS_%s"),Name,Name);
  ConstructorHelpers::FObjectFinder<UAnimSequence> Asset(*Path);GatherAnimations.Add(Asset.Object);
 }
 for(const TCHAR* Name:{TEXT("ClimbIdle"),TEXT("ClimbUp"),TEXT("ClimbDown"),TEXT("ClimbLeft"),TEXT("ClimbRight"),TEXT("ClimbTop")})
 {
  const FString Path=FString::Printf(TEXT("/Game/Characters/Actions/AS_%s.AS_%s"),Name,Name);
  ConstructorHelpers::FObjectFinder<UAnimSequence> Asset(*Path);ClimbAnimations.Add(Asset.Object);
 }
}

void UForestIsometricPlayer::PlayAction(UAnimSequence* Animation,bool Loop)
{
 auto* C=Character.Get();if(!C || !Animation || ActiveActionAnimation==Animation)return;
 ActiveActionAnimation=Animation;C->GetMesh()->PlayAnimation(Animation,Loop);
}
void UForestIsometricPlayer::RestoreLocomotion()
{
 ActiveActionAnimation=nullptr;
 if(auto* C=Character.Get())
 {
  if(bSwimming)C->GetMesh()->PlayAnimation(SwimmingAnimation,true);
  else {C->GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);C->GetMesh()->SetAnimInstanceClass(WalkingAnimClass);}
 }
}
bool UForestIsometricPlayer::BeginGather(AForestElementResource* Resource)
{
 auto* C=Character.Get();if(!C || !IsValid(Resource) || Resource->bCollected || ActionBusy() || MenuOpen())return false;
 if(!bSwimming && !C->GetCharacterMovement()->IsMovingOnGround())return false;
 const int32 Offset=bSwimming?3:0;
 for(int32 Index=0;Index<3;++Index)if(!GatherAnimations.IsValidIndex(Offset+Index) || !GatherAnimations[Offset+Index])return false;
 GatheringResource=Resource;bGathering=true;bGatherInWater=bSwimming;GatherStage=0;ActionAge=StageAge=0;
 C->GetCharacterMovement()->StopMovementImmediately();C->ConsumeMovementInputVector();
 // Keep the existing camera/controller direction. Only the visible body reaches toward the target.
 const FVector Direction=(Resource->GetActorLocation()-C->GetActorLocation()).GetSafeNormal2D();
 if(!Direction.IsNearlyZero())C->GetMesh()->SetWorldRotation(FRotator(0,Direction.Rotation().Yaw-90.f,0));
 PlayAction(GatherAnimations[Offset],false);return true;
}
void UForestIsometricPlayer::FinishGather(bool Commit)
{
 auto* Resource=GatheringResource.Get();GatheringResource.Reset();bGathering=false;
 RestoreLocomotion();
 if(auto* C=Character.Get())
 {
  C->GetMesh()->SetRelativeRotation(FRotator(0,-90,0));
  if(Commit)if(auto* Regions=GetWorld()->GetSubsystem<UForestElementRegions>())Regions->CollectResource(C,bSwimming,Resource);
 }
}

bool UForestIsometricPlayer::FindClimbSurface(const FVector& Position,const FVector& Direction,FHitResult& Hit) const
{
 auto* C=Character.Get();if(!C)return false;
 FCollisionQueryParams Query(SCENE_QUERY_STAT(ForestClimbWall),true,C);
 // Trace the same channel as the character capsule, so decorative non-blockers cannot be climbed.
 const bool Found=GetWorld()->LineTraceSingleByChannel(Hit,Position,Position+Direction.GetSafeNormal()*150.f,C->GetCapsuleComponent()->GetCollisionObjectType(),Query);
 return Found && NaturalClimbSurface(Hit) && Hit.ImpactNormal.Z<.65f && Hit.ImpactNormal.Z>-.2f;
}
void UForestIsometricPlayer::ToggleClimbing()
{
 if(bClimbing){StopClimbing();return;}
 auto* C=Character.Get();if(!C || bGathering || bSwimming || MenuOpen())return;
 for(const auto& Animation:ClimbAnimations)if(!Animation)return;
 FVector Direction=ViewMode==EForestViewMode::Overhead?C->GetMesh()->GetRightVector():Player->GetControlRotation().Vector().GetSafeNormal2D();
 FHitResult Hit;
 if(!FindClimbSurface(C->GetActorLocation(),Direction,Hit))return;
 auto* Move=C->GetCharacterMovement();
 bSavedControllerYaw=C->bUseControllerRotationYaw;bSavedOrientToMovement=Move->bOrientRotationToMovement;
 C->bUseControllerRotationYaw=false;Move->bOrientRotationToMovement=false;
 C->StopJumping();C->ConsumeMovementInputVector();Move->StopMovementImmediately();Move->SetMovementMode(MOVE_Flying);
 ClimbNormal=Hit.ImpactNormal;bClimbing=true;bMantling=false;
 C->SetActorRotation((-ClimbNormal).GetSafeNormal2D().Rotation());C->GetMesh()->SetRelativeRotation(FRotator(0,-90,0));
 PlayAction(ClimbAnimations[0],true);
}
void UForestIsometricPlayer::StopClimbing()
{
 if(!bClimbing)return;
 bClimbing=bMantling=false;
 if(auto* C=Character.Get())
 {
  C->bUseControllerRotationYaw=bSavedControllerYaw;
  auto* Move=C->GetCharacterMovement();Move->bOrientRotationToMovement=bSavedOrientToMovement;
  Move->StopMovementImmediately();Move->SetMovementMode(MOVE_Falling);
 }
 RestoreLocomotion();
}
bool UForestIsometricPlayer::TryMantle()
{
 auto* C=Character.Get();if(!C)return false;
 const auto* Capsule=C->GetCapsuleComponent();const float Half=Capsule->GetScaledCapsuleHalfHeight(),Radius=Capsule->GetScaledCapsuleRadius();
 const FVector Position=C->GetActorLocation(),Inward=-ClimbNormal.GetSafeNormal2D();
 FCollisionQueryParams Query(SCENE_QUERY_STAT(ForestClimbTop),true,C);
 FHitResult Top;
 const FVector Probe=Position+Inward*(Radius+90.f);
 const auto Channel=Capsule->GetCollisionObjectType();
 if(!GetWorld()->LineTraceSingleByChannel(Top,Probe+FVector(0,0,Half+160),Probe-FVector(0,0,Half),Channel,Query)
  || !NaturalClimbSurface(Top) || !C->GetCharacterMovement()->IsWalkable(Top))return false;
 // Limit the step above the current feet; never teleport to a distant top.
 const float Rise=Top.ImpactPoint.Z-(Position.Z-Half);
 if(Rise<20.f || Rise>Half*2.f+100.f)return false;
 const FVector End=Top.ImpactPoint+FVector(0,0,Half+5.f);
 const FVector Lift(Position.X,Position.Y,End.Z+6.f);
 const FCollisionShape Shape=FCollisionShape::MakeCapsule(Radius,Half);
 if(GetWorld()->OverlapBlockingTestByChannel(End,FQuat::Identity,Channel,Shape,Query))return false;
 FHitResult Block;
 if(GetWorld()->SweepSingleByChannel(Block,Position,Lift,FQuat::Identity,Channel,Shape,Query)
  || GetWorld()->SweepSingleByChannel(Block,Lift,End,FQuat::Identity,Channel,Shape,Query))return false;
 MantleStart=Position;MantleLift=Lift;MantleEnd=End;MantleAge=0;bMantling=true;
 PlayAction(ClimbAnimations[5],false);return true;
}
void UForestIsometricPlayer::UpdateActions(float DT,bool Blocked)
{
 auto* C=Character.Get();auto* PC=Player.Get();if(!C || !PC)return;
 auto* Move=C->GetCharacterMovement();
 if(bGathering)
 {
  if(Blocked || bSwimming!=bGatherInWater || !GatheringResource.IsValid() || (!bSwimming && !Move->IsMovingOnGround())){FinishGather(false);return;}
  C->ConsumeMovementInputVector();Move->StopMovementImmediately();ActionAge+=DT;StageAge+=DT;
  const int32 Offset=bGatherInWater?3:0;
  if(StageAge>=GatherAnimations[Offset+GatherStage]->GetPlayLength())
  {
   StageAge=0;++GatherStage;
   if(GatherStage==3){FinishGather(true);return;}
   PlayAction(GatherAnimations[Offset+GatherStage],false);
  }
  return;
 }
 if(!bClimbing)return;
 C->ConsumeMovementInputVector();Move->StopMovementImmediately();
 if(Blocked){if(auto* Animation=C->GetMesh()->GetSingleNodeInstance())Animation->SetPlaying(false);return;}
 if(auto* Animation=C->GetMesh()->GetSingleNodeInstance())Animation->SetPlaying(true);
 if(bMantling)
 {
  MantleAge+=DT;const float Length=FMath::Max(.1f,ClimbAnimations[5]->GetPlayLength());
  const float Alpha=FMath::Clamp(MantleAge/Length,0.f,1.f);
  const FVector Position=Alpha<.55f?FMath::Lerp(MantleStart,MantleLift,Alpha/.55f):FMath::Lerp(MantleLift,MantleEnd,(Alpha-.55f)/.45f);
  FHitResult Hit;Move->SafeMoveUpdatedComponent(Position-C->GetActorLocation(),C->GetActorQuat(),true,Hit);
  if(Hit.bBlockingHit || Alpha>=1.f)StopClimbing();
  return;
 }
 const float Up=(PC->IsInputKeyDown(EKeys::W)?1.f:0.f)-(PC->IsInputKeyDown(EKeys::S)?1.f:0.f)+PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY);
 const float Right=(PC->IsInputKeyDown(EKeys::D)?1.f:0.f)-(PC->IsInputKeyDown(EKeys::A)?1.f:0.f)+PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX);
 const FVector2D Axis=FVector2D(Right,Up).GetClampedToMaxSize(1.f);
 const FVector AlongUp=FVector::VectorPlaneProject(FVector::UpVector,ClimbNormal).GetSafeNormal();
 const FVector AlongRight=FVector::CrossProduct(FVector::UpVector,-ClimbNormal).GetSafeNormal();
 const FVector Delta=(AlongUp*Axis.Y+AlongRight*Axis.X)*ClimbSpeed*FMath::Min(DT,.05f);
 FHitResult Surface;
 if(!FindClimbSurface(C->GetActorLocation()+Delta,-ClimbNormal,Surface))
 {
  if(Axis.Y>0 && TryMantle())return;
  if(!FindClimbSurface(C->GetActorLocation(),-ClimbNormal,Surface)){StopClimbing();return;}
  PlayAction(ClimbAnimations[0],true);return;
 }
 // An upright capsule needs greater clearance on slopes than on vertical walls.
 const float Radius=C->GetCapsuleComponent()->GetScaledCapsuleRadius(),Half=C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
 const float Clearance=Radius+(Half-Radius)*FMath::Abs(Surface.ImpactNormal.Z)+5.f;
 const float Distance=FVector::DotProduct(C->GetActorLocation()-Surface.ImpactPoint,Surface.ImpactNormal);
 const FVector Correction=Surface.ImpactNormal*FMath::Clamp(Clearance-Distance,-160.f*DT,160.f*DT);
 ClimbNormal=Surface.ImpactNormal;
 FHitResult Hit;Move->SafeMoveUpdatedComponent(Delta+Correction,(-ClimbNormal).GetSafeNormal2D().Rotation().Quaternion(),true,Hit);
 // At a lip the full capsule can stop before the centre trace loses the wall.
 if(Axis.Y>0 && (Hit.bBlockingHit || !FindClimbSurface(C->GetActorLocation()+FVector(0,0,Half*.7f),-ClimbNormal,Surface)))
  if(TryMantle())return;
 if(Axis.Y<0)
 {
  FHitResult Floor;FCollisionQueryParams Query(SCENE_QUERY_STAT(ForestClimbLand),false,C);
  if(GetWorld()->LineTraceSingleByChannel(Floor,C->GetActorLocation(),C->GetActorLocation()-FVector(0,0,Half+12),C->GetCapsuleComponent()->GetCollisionObjectType(),Query) && Move->IsWalkable(Floor)){StopClimbing();return;}
 }
 const int32 Index=Axis.IsNearlyZero()?0:FMath::Abs(Axis.Y)>=FMath::Abs(Axis.X)?(Axis.Y>0?1:2):(Axis.X<0?3:4);
 PlayAction(ClimbAnimations[Index],true);
}
