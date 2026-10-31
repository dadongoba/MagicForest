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
 bool ClimbNormalAllowed(const FHitResult& Hit)
 {
  // These cliff meshes have overhanging skirts/bands before the vertical face.
  // Keep the general limit unchanged and still reject horizontal undersides.
  const auto* Mesh=Cast<UStaticMeshComponent>(Hit.GetComponent());
  const bool ExtendedOverhang=(Mesh && Mesh->GetStaticMesh() && (Mesh->GetStaticMesh()->GetName()==TEXT("SM_Cliff_04") || Mesh->GetStaticMesh()->GetName()==TEXT("SM_Cliff_02")))
   || (Hit.GetComponent() && Hit.GetComponent()->ComponentHasTag(TEXT("ForestClimbOverhang")));
  return Hit.ImpactNormal.Z<.65f && Hit.ImpactNormal.Z>(ExtendedOverhang?-.65f:-.2f);
 }
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
 for(const TCHAR* Name:{TEXT("ClimbIdle"),TEXT("ClimbUpLoop"),TEXT("ClimbDown"),TEXT("ClimbLeft"),TEXT("ClimbRight"),TEXT("ClimbTop")})
 {
  const FString Path=FString::Printf(TEXT("/Game/Characters/Actions/AS_%s.AS_%s"),Name,Name);
  ConstructorHelpers::FObjectFinder<UAnimSequence> Asset(*Path);ClimbAnimations.Add(Asset.Object);
 }
}

void UForestIsometricPlayer::PlayAction(UAnimSequence* Animation,bool Loop)
{
 auto* C=Character.Get();if(!C || !Animation || ActiveActionAnimation==Animation)return;
 ActiveActionAnimation=Animation;C->GetMesh()->PlayAnimation(Animation,Loop);
 if(auto* Instance=C->GetMesh()->GetSingleNodeInstance())Instance->SetPlayRate(1.f);
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
 auto* Regions=GetWorld()->GetSubsystem<UForestElementRegions>();if(!Regions || !Regions->CanHarvestResource(C,bSwimming,Resource))return false;
 GatheringResource=Resource;bGathering=true;bGatherInWater=bSwimming;bGatherAnimationStarted=false;ActionAge=-FMath::Clamp(GatherHighlightDuration,.05f,.5f);
 Resource->SetHarvestHighlighted(true);
 C->GetCharacterMovement()->StopMovementImmediately();C->ConsumeMovementInputVector();
 // Keep the existing camera/controller direction. Only the visible body reaches toward the target.
 const FVector Direction=(Resource->GetActorLocation()-C->GetActorLocation()).GetSafeNormal2D();
 if(!Direction.IsNearlyZero())C->GetMesh()->SetWorldRotation(FRotator(0,Direction.Rotation().Yaw-90.f,0));
 return true;
}
void UForestIsometricPlayer::FinishGather(bool Commit)
{
 auto* Resource=GatheringResource.Get();if(Resource)Resource->SetHarvestHighlighted(false);GatheringResource.Reset();bGathering=false;bGatherAnimationStarted=false;
 RestoreLocomotion();
 if(auto* C=Character.Get())
 {
  C->GetMesh()->SetRelativeRotation(FRotator(0,-90,0));
  if(Commit)if(auto* Regions=GetWorld()->GetSubsystem<UForestElementRegions>())Regions->CollectResource(C,bSwimming,Resource);
 }
}

bool UForestIsometricPlayer::IsClimbCapsuleClear(const FVector& Position,UPrimitiveComponent* Ignore) const
{
 auto* C=Character.Get();if(!C)return false;
 auto* Capsule=C->GetCapsuleComponent();
 FCollisionQueryParams Query(SCENE_QUERY_STAT(ForestClimbClearance),false,C);
 if(Ignore)Query.AddIgnoredComponent(Ignore);
 return !GetWorld()->OverlapBlockingTestByChannel(Position,FQuat::Identity,Capsule->GetCollisionObjectType(),
  FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(),Capsule->GetScaledCapsuleHalfHeight()),Query);
}
void UForestIsometricPlayer::RestoreClimbCollision()
{
 if(auto* C=Character.Get())if(auto* Surface=ClimbSurface.Get(true))
  C->GetCapsuleComponent()->IgnoreComponentWhenMoving(Surface,bClimbSurfaceWasIgnored);
 ClimbSurface.Reset();bClimbSurfaceWasIgnored=false;
}
bool UForestIsometricPlayer::FindSafeClimbExit(FVector& Position) const
{
 auto* C=Character.Get();if(!C)return false;
 Position=C->GetActorLocation();if(IsClimbCapsuleClear(Position))return true;
 auto* Capsule=C->GetCapsuleComponent();
 FCollisionQueryParams Query(SCENE_QUERY_STAT(ForestClimbExit),false,C);
 if(ClimbSurface.IsValid())Query.AddIgnoredComponent(ClimbSurface.Get());
 const auto Shape=FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(),Capsule->GetScaledCapsuleHalfHeight());
 const FVector Outward=ClimbNormal.GetSafeNormal2D();
 if(Outward.IsNearlyZero())return false;
 for(float Lift:{0.f,24.f,48.f})for(float Distance=8.f;Distance<=160.f;Distance+=8.f)
 {
  const FVector Candidate=C->GetActorLocation()+Outward*Distance+FVector(0,0,Lift);
  FHitResult Block;
  if(IsClimbCapsuleClear(Candidate) && !GetWorld()->SweepSingleByChannel(Block,C->GetActorLocation(),Candidate,FQuat::Identity,Capsule->GetCollisionObjectType(),Shape,Query))
  {Position=Candidate;return true;}
 }
 return false;
}
bool UForestIsometricPlayer::SetClimbCollisionSurface(UPrimitiveComponent* Surface)
{
 auto* C=Character.Get();if(!C || !IsValid(Surface) || Surface->Mobility!=EComponentMobility::Static)return false;
 if(ClimbSurface==Surface)return true;
 // Never restore the old component while the capsule is still embedded in it.
 // During a seam transition there is still exactly one component on our list.
 if(ClimbSurface.IsValid() && !IsClimbCapsuleClear(C->GetActorLocation(),Surface))
 {
  FVector Exit;if(!FindSafeClimbExit(Exit))return false;
  FHitResult Block;C->GetCharacterMovement()->SafeMoveUpdatedComponent(Exit-C->GetActorLocation(),C->GetActorQuat(),true,Block);
  if(!IsClimbCapsuleClear(C->GetActorLocation(),Surface))return false;
 }
 RestoreClimbCollision();
 auto* Capsule=C->GetCapsuleComponent();bClimbSurfaceWasIgnored=Capsule->GetMoveIgnoreComponents().Contains(Surface);
 ClimbSurface=Surface;Capsule->IgnoreComponentWhenMoving(Surface,true);return true;
}
void UForestIsometricPlayer::MoveClimbing(const FVector& Delta,const FQuat& Rotation,FHitResult& Hit)
{
 auto* C=Character.Get();if(!C)return;
 // Independent world queries deliberately do NOT copy the capsule's movement
 // ignore list. A small torso core still blocks going through the rock itself;
 // the full capsule continues sweeping against every other component.
 FCollisionQueryParams Query(SCENE_QUERY_STAT(ForestClimbBodyCore),false,C);
 const auto* Capsule=C->GetCapsuleComponent();
 const auto Core=FCollisionShape::MakeSphere(FMath::Min(14.f,Capsule->GetScaledCapsuleRadius()*.35f));
 float Fraction=1.f;FHitResult CoreHit;
 if(!Delta.IsNearlyZero())for(float Height:{-24.f,0.f,24.f})
 {
  FHitResult Block;const FVector Start=C->GetActorLocation()+FVector(0,0,Height);
  if(GetWorld()->SweepSingleByChannel(Block,Start,Start+Delta,FQuat::Identity,Capsule->GetCollisionObjectType(),Core,Query) && Block.Time<Fraction)
  {Fraction=FMath::Max(0.f,Block.Time-.01f);CoreHit=Block;}
 }
 C->GetCharacterMovement()->SafeMoveUpdatedComponent(Delta*Fraction,Rotation,true,Hit);
 if(Fraction<1.f && !Hit.bBlockingHit)Hit=CoreHit;
}
bool UForestIsometricPlayer::FindClimbSurface(const FVector& Position,const FVector& Direction,FHitResult& Hit) const
{
 auto* C=Character.Get();if(!C)return false;
 // Surface detection remains independent of IgnoreComponentWhenMoving.
 FCollisionQueryParams Query(SCENE_QUERY_STAT(ForestClimbWall),false,C);
 const FVector Forward=Direction.GetSafeNormal2D();if(Forward.IsNearlyZero())return false;
 const FVector Right=FVector::CrossProduct(FVector::UpVector,Forward);
 const float Radius=C->GetCapsuleComponent()->GetScaledCapsuleRadius();
 TArray<FHitResult> Supports;
 for(const FVector2D Offset:{FVector2D(0,0),FVector2D(0,32),FVector2D(0,-28),FVector2D(-Radius*.55f,12),FVector2D(Radius*.55f,12)})
 {
  const FVector Start=Position+Right*Offset.X+FVector::UpVector*Offset.Y;
  FHitResult Sample;
  if(GetWorld()->SweepSingleByChannel(Sample,Start,Start+Forward*150.f,FQuat::Identity,C->GetCapsuleComponent()->GetCollisionObjectType(),FCollisionShape::MakeSphere(6.f),Query)
   && !Sample.bStartPenetrating && NaturalClimbSurface(Sample) && ClimbNormalAllowed(Sample)
   && FVector::DotProduct(Sample.ImpactNormal,-Forward)>.35f)Supports.Add(Sample);
 }
 if(Supports.IsEmpty())return false;
 // Keep the nearest surface; never average across a sharp corner or two actors.
 Supports.Sort([](const FHitResult& A,const FHitResult& B){return A.Distance<B.Distance;});
 Hit=Supports[0];FVector Normal=Hit.ImpactNormal;int32 Count=1;
 for(int32 Index=1;Index<Supports.Num();++Index)
  if(Supports[Index].GetComponent()==Hit.GetComponent() && FVector::DotProduct(Supports[Index].ImpactNormal,Hit.ImpactNormal)>.85f)
   {Normal+=Supports[Index].ImpactNormal;++Count;}
 Hit.ImpactNormal=(Normal/Count).GetSafeNormal();return true;
}
bool UForestIsometricPlayer::FindClimbCandidate(FHitResult& Hit) const
{
 auto* C=Character.Get();auto* PC=Player.Get();if(!C || !PC)return false;
 FVector Forward=C->GetActorForwardVector();
 if(ViewMode==EForestViewMode::Overhead)Forward=C->GetMesh()->GetRightVector();
 else if(!C->GetLastMovementInputVector().IsNearlyZero())Forward=C->GetLastMovementInputVector();
 else Forward=PC->GetControlRotation().Vector();
 Forward=Forward.GetSafeNormal2D();
 for(float Angle:{0.f,-20.f,20.f})
  if(FindClimbSurface(C->GetActorLocation(),Forward.RotateAngleAxis(Angle,FVector::UpVector),Hit)
   && Hit.Distance<=C->GetCapsuleComponent()->GetScaledCapsuleRadius()+65.f)return true;
 return false;
}
void UForestIsometricPlayer::ToggleClimbing()
{
 if(bClimbing){StopClimbing();return;}
 auto* C=Character.Get();if(!C || bGathering || bSwimming || MenuOpen())return;
 if(ClimbReattachCooldown>0)return;
 ClimbRequestRemaining=ClimbInputBuffer;
 FHitResult Hit;if(FindClimbCandidate(Hit))BeginClimbing(Hit);
}
bool UForestIsometricPlayer::BeginClimbing(const FHitResult& Hit)
{
 auto* C=Character.Get();if(!C)return false;
 for(const auto& Animation:ClimbAnimations)if(!Animation)return false;
 if(!SetClimbCollisionSurface(Hit.GetComponent()))return false;
 // Align to a collision-safe contact before entering the climbing state.
 const auto* Capsule=C->GetCapsuleComponent();
 const float Clearance=Capsule->GetScaledCapsuleRadius()+(Capsule->GetScaledCapsuleHalfHeight()-Capsule->GetScaledCapsuleRadius())*FMath::Abs(Hit.ImpactNormal.Z)+2.f;
 const float Distance=FVector::DotProduct(C->GetActorLocation()-Hit.ImpactPoint,Hit.ImpactNormal);
 const FVector Correction=Hit.ImpactNormal*(Clearance-Distance);
 FHitResult Contact;MoveClimbing(Correction,C->GetActorQuat(),Contact);
 if(Contact.bStartPenetrating){RestoreClimbCollision();return false;}
 ClimbRequestRemaining=0;ClimbLostSurfaceAge=0;bCanClimb=false;bClimbExitPending=false;
 auto* Move=C->GetCharacterMovement();
 bSavedControllerYaw=C->bUseControllerRotationYaw;bSavedOrientToMovement=Move->bOrientRotationToMovement;
 C->bUseControllerRotationYaw=false;Move->bOrientRotationToMovement=false;
 C->StopJumping();C->ConsumeMovementInputVector();Move->StopMovementImmediately();Move->SetMovementMode(MOVE_Flying);
 ClimbNormal=Hit.ImpactNormal;bClimbing=true;bMantling=false;
 C->SetActorRotation((-ClimbNormal).GetSafeNormal2D().Rotation());C->GetMesh()->SetRelativeRotation(FRotator(0,-90,0));
 PlayAction(ClimbAnimations[0],true);
 return true;
}
void UForestIsometricPlayer::StopClimbing(bool bForce)
{
 if(!bClimbing){RestoreClimbCollision();return;}
 if(!bForce)if(auto* C=Character.Get())
 {
  FVector Exit;
  if(!FindSafeClimbExit(Exit)){bClimbExitPending=true;bMantling=false;return;}
  FHitResult Block;C->GetCharacterMovement()->SafeMoveUpdatedComponent(Exit-C->GetActorLocation(),C->GetActorQuat(),true,Block);
  if(!IsClimbCapsuleClear(C->GetActorLocation())){bClimbExitPending=true;bMantling=false;return;}
 }
 RestoreClimbCollision();bClimbExitPending=false;
 bClimbing=bMantling=false;
 ClimbRequestRemaining=0;ClimbReattachCooldown=.4f;ClimbLostSurfaceAge=0;bCanClimb=false;
 if(auto* C=Character.Get())
 {
  C->bUseControllerRotationYaw=bSavedControllerYaw;
  auto* Move=C->GetCharacterMovement();Move->bOrientRotationToMovement=bSavedOrientToMovement;
  Move->StopMovementImmediately();if(!bForce)Move->SetMovementMode(MOVE_Falling);
 }
 if(!bForce)RestoreLocomotion();
}
bool UForestIsometricPlayer::TryMantle()
{
 auto* C=Character.Get();if(!C)return false;
 const auto* Capsule=C->GetCapsuleComponent();const float Half=Capsule->GetScaledCapsuleHalfHeight(),Radius=Capsule->GetScaledCapsuleRadius();
 const FVector Position=C->GetActorLocation(),Inward=-ClimbNormal.GetSafeNormal2D();
 FCollisionQueryParams Query(SCENE_QUERY_STAT(ForestClimbTop),false,C);
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
 if(ClimbSurface.IsValid())Query.AddIgnoredComponent(ClimbSurface.Get());
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
 ClimbReattachCooldown=FMath::Max(0.f,ClimbReattachCooldown-DT);
 if(!bClimbing)
 {
  if(Blocked || bGathering || bSwimming){ClimbRequestRemaining=0;bCanClimb=false;}
  else if(ClimbReattachCooldown<=0)
  {
   ClimbProbeAge-=DT;
   if(ClimbRequestRemaining>0 || ClimbProbeAge<=0)
   {
    ClimbProbeAge=.08f;FHitResult Candidate;bCanClimb=FindClimbCandidate(Candidate);
    if(bCanClimb && ClimbRequestRemaining>0)BeginClimbing(Candidate);
   }
   ClimbRequestRemaining=FMath::Max(0.f,ClimbRequestRemaining-DT);
  }
 }
 if(bGathering)
 {
  if(Blocked || bSwimming!=bGatherInWater || !GatheringResource.IsValid() || !GatheringResource->bZoneAvailable || (!bSwimming && !Move->IsMovingOnGround())){FinishGather(false);return;}
  C->ConsumeMovementInputVector();Move->StopMovementImmediately();ActionAge+=DT;
  if(ActionAge>=0 && !bGatherAnimationStarted)
  {
   bGatherAnimationStarted=true;GatheringResource->SetHarvestHighlighted(false);
   const int32 Offset=bGatherInWater?3:0;PlayAction(GatherAnimations[Offset+1],false);
   if(auto* Instance=C->GetMesh()->GetSingleNodeInstance())Instance->SetPlayRate(GatherAnimations[Offset+1]->GetPlayLength()/FMath::Max(.2f,GatherDuration));
  }
  if(ActionAge>=FMath::Max(.2f,GatherDuration))FinishGather(true);
  return;
 }
 if(!bClimbing)return;
 if(Move->MovementMode!=MOVE_Flying){StopClimbing(true);return;}
 if(!ClimbSurface.IsValid()){StopClimbing();return;}
 if(bClimbExitPending){StopClimbing();if(!bClimbing)return;}
 C->ConsumeMovementInputVector();Move->StopMovementImmediately();
 if(Blocked){if(auto* Animation=C->GetMesh()->GetSingleNodeInstance())Animation->SetPlaying(false);return;}
 if(auto* Animation=C->GetMesh()->GetSingleNodeInstance())Animation->SetPlaying(true);
 if(bMantling)
 {
  MantleAge+=DT;const float Length=FMath::Max(.1f,ClimbAnimations[5]->GetPlayLength());
  const float Alpha=FMath::Clamp(MantleAge/Length,0.f,1.f);
  const FVector Position=Alpha<.55f?FMath::Lerp(MantleStart,MantleLift,Alpha/.55f):FMath::Lerp(MantleLift,MantleEnd,(Alpha-.55f)/.45f);
  FHitResult Hit;MoveClimbing(Position-C->GetActorLocation(),C->GetActorQuat(),Hit);
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
  if(!FindClimbSurface(C->GetActorLocation(),-ClimbNormal,Surface))
  {
   ClimbLostSurfaceAge+=DT;PlayAction(ClimbAnimations[0],true);
   if(ClimbLostSurfaceAge>ClimbSurfaceGrace)StopClimbing();
   return;
  }
  PlayAction(ClimbAnimations[0],true);return;
 }
 ClimbLostSurfaceAge=0;
 if(!SetClimbCollisionSurface(Surface.GetComponent())){PlayAction(ClimbAnimations[0],true);return;}
 // An upright capsule needs greater clearance on slopes than on vertical walls.
 const float Radius=C->GetCapsuleComponent()->GetScaledCapsuleRadius(),Half=C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
 const float Clearance=Radius+(Half-Radius)*FMath::Abs(Surface.ImpactNormal.Z)+2.f;
 const float Distance=FVector::DotProduct(C->GetActorLocation()-Surface.ImpactPoint,Surface.ImpactNormal);
 const FVector Correction=Surface.ImpactNormal*FMath::Clamp((Clearance-Distance)*(1.f-FMath::Exp(-18.f*DT)),-220.f*DT,220.f*DT);
 ClimbNormal=FMath::VInterpTo(ClimbNormal,Surface.ImpactNormal,DT,12.f).GetSafeNormal();
 const FVector BeforeMove=C->GetActorLocation();
 FHitResult Hit;MoveClimbing(Delta+Correction,(-ClimbNormal).GetSafeNormal2D().Rotation().Quaternion(),Hit);
 // At a lip the full capsule can stop before the centre trace loses the wall.
 if(Axis.Y>0 && (Hit.bBlockingHit || !FindClimbSurface(C->GetActorLocation()+FVector(0,0,Half*.7f),-ClimbNormal,Surface)))
  if(TryMantle())return;
 if(Axis.Y<0)
 {
  FHitResult Floor;FCollisionQueryParams Query(SCENE_QUERY_STAT(ForestClimbLand),false,C);
  if(GetWorld()->LineTraceSingleByChannel(Floor,C->GetActorLocation(),C->GetActorLocation()-FVector(0,0,Half+12),C->GetCapsuleComponent()->GetCollisionObjectType(),Query) && Move->IsWalkable(Floor)){StopClimbing();return;}
 }
 const float ActualSpeed=FVector::VectorPlaneProject(C->GetActorLocation()-BeforeMove,ClimbNormal).Size()/FMath::Max(DT,.001f);
 const int32 Index=Axis.IsNearlyZero() || ActualSpeed<3.f?0:FMath::Abs(Axis.Y)>=FMath::Abs(Axis.X)?(Axis.Y>0?1:2):(Axis.X<0?3:4);
 PlayAction(ClimbAnimations[Index],true);
 if(auto* Animation=C->GetMesh()->GetSingleNodeInstance())Animation->SetPlayRate(Index==0?1.f:Index==1?FMath::Clamp(ActualSpeed*ClimbAnimations[1]->GetPlayLength()/70.f,.25f,2.8f):FMath::Clamp(ActualSpeed/ClimbSpeed,.25f,1.35f));
}
