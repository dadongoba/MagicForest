#include "ForestIsometricPlayer.h"
#include "ForestNPCInteraction.h"
#include "ForestHarvestMagic.h"
#include "ForestAlchemy.h"
#include "ForestMinimap.h"
#include "DailyMushroomSubsystem.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Components/BrushComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "GameFramework/PhysicsVolume.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameterCollection.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "UObject/UnrealType.h"
#include "NavigationSystem.h"

namespace
{
 FString MushroomKey(FString Name)
 {
  int32 Suffix=Name.Find(TEXT("_LOD"));if(Suffix>=0)Name.LeftInline(Suffix);
  Suffix=Name.Find(TEXT("_Alt"));if(Suffix>=0)Name.LeftInline(Suffix);
  Name.ReplaceInline(TEXT("_Small_Group_"),TEXT("_Small_"));return Name;
 }
 bool IsCameraRoof(const UStaticMeshComponent* Mesh)
 {
  if(!Mesh || !Mesh->GetStaticMesh())return false;
  const FString Name=Mesh->GetStaticMesh()->GetName();
  // The shop's ceiling uses a generic exported mesh name. Furniture must never
  // enter the occluder list simply because it is large or above the player.
  return Mesh->ComponentHasTag(TEXT("ForestCameraRoof")) ||
   Name.Contains(TEXT("roof"),ESearchCase::IgnoreCase) || Name==TEXT("SM_Shop_Cube_092");
 }
}

UForestIsometricPlayer::UForestIsometricPlayer()
{
 PrimaryComponentTick.bCanEverTick=true;
 PrimaryComponentTick.TickGroup=TG_PrePhysics;
}
bool UForestIsometricPlayer::RebuildNavigation(UObject* Context)
{
#if WITH_EDITOR
 UWorld* W=Context?Context->GetWorld():nullptr;
 auto* Nav=W?FNavigationSystem::GetCurrent<UNavigationSystemV1>(W):nullptr;
 if(Nav){Nav->Build();return true;}
#endif
 return false;
}

void UForestIsometricPlayer::BeginPlay()
{
 Super::BeginPlay();
 auto* C=Cast<ACharacter>(GetOwner());
 auto* PC=C?Cast<APlayerController>(C->GetController()):nullptr;
 if(!C || !PC)return;
 Character=C;Player=PC;
 // One owner for movement/look: leave the pawn's inventory data and events intact,
 // but remove the imported template's input so it cannot add a second move/look.
 bPawnInputWasEnabled=C->InputEnabled();C->DisableInput(PC);
 AddTickPrerequisiteActor(PC);
 C->GetCharacterMovement()->AddTickPrerequisiteComponent(this);
 LookYaw=CameraYaw;
 if(FSlateApplication::IsInitialized())LastUserInteractionTime=FSlateApplication::Get().GetLastUserInteractionTime();
 // This component owns facing; controller and template rotation must not fight it.
 C->bUseControllerRotationYaw=false;C->bUseControllerRotationPitch=false;C->bUseControllerRotationRoll=false;
 C->GetCharacterMovement()->bOrientRotationToMovement=false;
 C->GetCharacterMovement()->RotationRate=FRotator(0,640,0);
 C->GetCharacterMovement()->MaxWalkSpeed=450;
 PC->SetIgnoreLookInput(true);
 PC->SetControlRotation(FRotator(0,CameraYaw,0));
 auto* Body=C->GetMesh();
 Body->SetSkeletalMeshAsset(LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/ChengYi/Mesh/SK_CHENGYI_01.SK_CHENGYI_01")));
 Body->SetRelativeLocation(FVector(0,0,-C->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));
 Body->SetRelativeRotation(FRotator(0,-90,0));
 Body->SetOnlyOwnerSee(false);Body->SetOwnerNoSee(false);Body->SetVisibility(true,true);
 Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Body->SetAnimationMode(EAnimationMode::AnimationBlueprint);
 Body->SetAnimInstanceClass(LoadClass<UAnimInstance>(nullptr,TEXT("/Game/ChengYi/Demo/Animations/ThirdPerson_AnimBP.ThirdPerson_AnimBP_C")));
 InitializeSwimming();
 Body->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
 TArray<USkeletalMeshComponent*> Meshes;C->GetComponents(Meshes);
 for(auto* M:Meshes)if(M!=Body)M->SetVisibility(false,true);
 TArray<UCameraComponent*> OldCameras;C->GetComponents(OldCameras);
 for(auto* Old:OldCameras)Old->Deactivate();
 Boom=NewObject<USpringArmComponent>(C,TEXT("ForestIsometricBoom"));
 C->AddInstanceComponent(Boom);Boom->SetupAttachment(C->GetRootComponent());
 Boom->SetUsingAbsoluteRotation(true);Boom->SetWorldRotation(FRotator(CameraPitch,CameraYaw,0));
 Boom->TargetOffset=FVector(0,0,70);Boom->TargetArmLength=CameraDistance;
 // A roof must not push this overhead camera down into the player's head.
 Boom->bDoCollisionTest=false;Boom->bUsePawnControlRotation=false;Boom->RegisterComponent();
 Boom->AddTickPrerequisiteComponent(this);
 Camera=NewObject<UCameraComponent>(C,TEXT("ForestIsometricCamera"));C->AddInstanceComponent(Camera);
 Camera->SetupAttachment(Boom,USpringArmComponent::SocketName);Camera->FieldOfView=45;
 Camera->bUsePawnControlRotation=false;Camera->RegisterComponent();Camera->Activate();
 CameraOcclusionParameters=LoadObject<UMaterialParameterCollection>(nullptr,TEXT("/Game/Forest/Camera/MPC_ForestCameraOcclusion.MPC_ForestCameraOcclusion"));
 PC->SetViewTarget(C);PC->bShowMouseCursor=true;
 FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);PC->SetInputMode(Mode);
 // Alchemy (200) handles focused bottles/cauldrons first. This replaces the
 // inherited first-person harvest key. The NPC manager owns C; V cycles views.
 Controls=NewObject<UInputComponent>(C,TEXT("ForestIsometricInput"));Controls->RegisterComponent();Controls->Priority=150;
 Controls->BindKey(EKeys::MouseScrollUp,IE_Pressed,this,&UForestIsometricPlayer::ZoomIn);
 Controls->BindKey(EKeys::MouseScrollDown,IE_Pressed,this,&UForestIsometricPlayer::ZoomOut);
 Controls->BindKey(EKeys::E,IE_Pressed,this,&UForestIsometricPlayer::InteractNearby).bConsumeInput=true;
 Controls->BindKey(EKeys::M,IE_Pressed,this,&UForestIsometricPlayer::ToggleMap).bConsumeInput=true;
 Controls->BindKey(EKeys::V,IE_Pressed,this,&UForestIsometricPlayer::CycleViewMode).bConsumeInput=true;
 Controls->BindKey(EKeys::RightMouseButton,IE_Pressed,this,&UForestIsometricPlayer::StartObserving).bConsumeInput=true;
 // Release is checked against Slate's physical state in Tick, so a synthetic
 // release caused by an input-mode focus change cannot interrupt a drag.
 PC->PushInputComponent(Controls);
 Minimap=CreateWidget<UForestMinimapWidget>(PC);
 if(Minimap){Minimap->SetVisibility(ESlateVisibility::HitTestInvisible);Minimap->AddToViewport(80);Minimap->UpdateMap(0);}
 UE_LOG(LogTemp,Log,TEXT("Forest isometric ready: %s, ChengYi mesh, inherited inventory preserved"),*C->GetName());
}
bool UForestIsometricPlayer::MenuOpen() const
{
 if(auto* H=GetWorld()->GetSubsystem<UForestHarvestMagic>();H && H->IsInventoryOpen())return true;
 if(auto* A=GetWorld()->GetSubsystem<UForestAlchemy>();A && A->IsWorkbenchOpen())return true;
 auto* M=Cast<AForestNPCInteraction>(UGameplayStatics::GetActorOfClass(GetWorld(),AForestNPCInteraction::StaticClass()));
 return M && (M->IsDialogueOpen() || M->IsFinderMenuOpen());
}
void UForestIsometricPlayer::ZoomIn()
{
 if(MenuOpen() || (Minimap && Minimap->bOverview))return;
 if(ViewMode==EForestViewMode::Overhead)CameraDistance=FMath::Clamp(CameraDistance-120,700.f,2200.f);
 else if(ViewMode==EForestViewMode::ThirdPerson)ThirdPersonDistance=FMath::Clamp(ThirdPersonDistance-40,200.f,700.f);
}
void UForestIsometricPlayer::ZoomOut()
{
 if(MenuOpen() || (Minimap && Minimap->bOverview))return;
 if(ViewMode==EForestViewMode::Overhead)CameraDistance=FMath::Clamp(CameraDistance+120,700.f,2200.f);
 else if(ViewMode==EForestViewMode::ThirdPerson)ThirdPersonDistance=FMath::Clamp(ThirdPersonDistance+40,200.f,700.f);
}
void UForestIsometricPlayer::ApplyGameplayInputMode()
{
 auto* PC=Player.Get();if(!PC || MenuOpen())return;
 bSkipLookDelta=true;
 const bool FreeCursor=(Minimap && Minimap->bOverview) || (ViewMode==EForestViewMode::Overhead && !bObserving);
 PC->bShowMouseCursor=FreeCursor;
 if(FreeCursor){FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);PC->SetInputMode(Mode);}
 else {FInputModeGameOnly Mode;Mode.SetConsumeCaptureMouseDown(false);PC->SetInputMode(Mode);}
}
void UForestIsometricPlayer::CycleViewMode()
{
 if(MenuOpen() || (Minimap && Minimap->bOverview))return;
 if(bObserving)StopObserving();
 if(ViewMode==EForestViewMode::Overhead){LookYaw=FMath::UnwindDegrees(CameraYaw+ObservationYaw);ViewMode=EForestViewMode::FirstPerson;}
 else if(ViewMode==EForestViewMode::FirstPerson)ViewMode=EForestViewMode::ThirdPerson;
 else ViewMode=EForestViewMode::Overhead;
 CameraIdleSeconds=0;
 // Do not interpolate from a distant overhead position through walls or the body.
 Boom->TargetArmLength=ViewMode==EForestViewMode::Overhead?CameraDistance:ViewMode==EForestViewMode::ThirdPerson?ThirdPersonDistance:0.f;
 ApplyGameplayInputMode();
 UE_LOG(LogTemp,Log,TEXT("Forest view mode: %d (0 overhead, 1 first person, 2 third person)"),static_cast<int32>(ViewMode));
}
void UForestIsometricPlayer::ToggleMap(){if(Minimap && !MenuOpen()){if(bObserving)StopObserving();Minimap->ToggleOverview();ApplyGameplayInputMode();}}
void UForestIsometricPlayer::StartObserving()
{
 auto* PC=Player.Get();if(!PC || ViewMode!=EForestViewMode::Overhead || bObserving || MenuOpen() || (Minimap && Minimap->bOverview))return;
 bHadMousePosition=PC->GetMousePosition(SavedMouseX,SavedMouseY);
 bObserving=true;PC->bShowMouseCursor=false;
 bSkipLookDelta=true;
 CameraIdleSeconds=0;
 FInputModeGameOnly Mode;Mode.SetConsumeCaptureMouseDown(false);PC->SetInputMode(Mode);
}
void UForestIsometricPlayer::StopObserving()
{
 if(!bObserving)return;
 bObserving=false;
 CameraIdleSeconds=0;
 if(auto* PC=Player.Get())
 {
  PC->bShowMouseCursor=true;
  // A dialogue may have opened during the drag. Keep its existing UI focus.
  if(!MenuOpen())
  {
   FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);PC->SetInputMode(Mode);
   if(bHadMousePosition)PC->SetMouseLocation(FMath::RoundToInt(SavedMouseX),FMath::RoundToInt(SavedMouseY));
  }
 }
 bHadMousePosition=false;
}
void UForestIsometricPlayer::InteractNearby()
{
 auto* C=Character.Get();if(!C || MenuOpen())return;
 auto* Manager=Cast<AForestNPCInteraction>(UGameplayStatics::GetActorOfClass(GetWorld(),AForestNPCInteraction::StaticClass()));
 if(Manager)
 {
  TArray<ACharacter*> Nearby;
  for(TActorIterator<ACharacter> It(GetWorld());It;++It)
   if(*It!=C && FVector::DistSquared(It->GetActorLocation(),C->GetActorLocation())<=FMath::Square(Manager->InteractionRange))Nearby.Add(*It);
  Nearby.Sort([C](const ACharacter& A,const ACharacter& B){return FVector::DistSquared(A.GetActorLocation(),C->GetActorLocation())<FVector::DistSquared(B.GetActorLocation(),C->GetActorLocation());});
  for(auto* NPC:Nearby)if(Manager->TryInteractWith(NPC))return;
 }
 HarvestNearby();
}
void UForestIsometricPlayer::TickComponent(float DT,ELevelTick Type,FActorComponentTickFunction* Fn)
{
 Super::TickComponent(DT,Type,Fn);
 if(Minimap)Minimap->UpdateMap(DT);
 auto* C=Character.Get();auto* PC=Player.Get();if(!C || !PC || !Boom)return;
 const bool Open=MenuOpen();
 // Slate tracks the physical button across viewport capture/focus changes.
 // PlayerInput may flush its key state when the input mode changes mid-press.
 const bool RightHeld=FSlateApplication::IsInitialized()
  ? FSlateApplication::Get().IsActive() && FSlateApplication::Get().GetPressedMouseButtons().Contains(EKeys::RightMouseButton)
  : PC->IsInputKeyDown(EKeys::RightMouseButton);
 if(bObserving && (Open || (Minimap && Minimap->bOverview) || !RightHeld))StopObserving();
 UpdateSwimming(DT);
 UpdateViewAndMovement(DT,Open || (Minimap && Minimap->bOverview));
 if(CameraOcclusionParameters)
 {
  const FVector Eye=Camera->GetComponentLocation(),Target=C->GetActorLocation()+FVector(0,0,40);
  UKismetMaterialLibrary::SetVectorParameterValue(this,CameraOcclusionParameters,TEXT("ForestCameraPosition"),FLinearColor(Eye.X,Eye.Y,Eye.Z,1));
  UKismetMaterialLibrary::SetVectorParameterValue(this,CameraOcclusionParameters,TEXT("ForestPlayerPosition"),FLinearColor(Target.X,Target.Y,Target.Z,1));
  UKismetMaterialLibrary::SetScalarParameterValue(this,CameraOcclusionParameters,TEXT("ForestCameraFadeEnabled"),ViewMode==EForestViewMode::Overhead?1.f:0.f);
 }
 const bool MapOpen=Minimap && Minimap->bOverview;
 if((bMenuWasOpen && !Open) || (bMapWasOpen && !MapOpen))ApplyGameplayInputMode();
 bMenuWasOpen=Open;bMapWasOpen=MapOpen;
 ScanDelay-=DT;
 if(ScanDelay<=0)
 {
  ScanDelay=1;Mushrooms.Reset();OverheadMeshes.Reset();
  for(TActorIterator<AActor> It(GetWorld());It;++It)
  {
   TArray<UStaticMeshComponent*> Parts;It->GetComponents(Parts);
   for(auto* M:Parts)if(M->GetStaticMesh())
   {
    // This migrated shop has a closed decorative door, without a door script.
    // Keep its entrance open in this map so the cabin and outdoor navmesh join.
    if(M->GetStaticMesh()->GetName()==TEXT("SM_EntranceAreas_Plane_047"))
    {M->SetCollisionEnabled(ECollisionEnabled::NoCollision);M->SetVisibility(false,false);}
    if(M->GetStaticMesh()->GetName().StartsWith(TEXT("SM_Env_Mushroom"))){UForestHarvestMagic::ConfigureMushroomCollision(M);Mushrooms.Add(M);}
    if(IsCameraRoof(M))OverheadMeshes.Add(M);
   }
  }
  auto* D=GetWorld()->GetGameInstance()->GetSubsystem<UDailyMushroomSubsystem>();const int32 Current=D?D->GetCurrentDay():1;
  if(Day && Current!=Day){for(auto& W:Harvested)if(auto* A=W.Get()){A->SetActorHiddenInGame(false);A->SetActorEnableCollision(true);}Harvested.Reset();}Day=Current;
 }
 RoofDelay-=DT;if(RoofDelay<=0){RoofDelay=.15f;UpdateOcclusion();}
}
void UForestIsometricPlayer::UpdateViewAndMovement(float DT,bool Blocked)
{
 auto* PC=Player.Get();auto* C=Character.Get();if(!PC || !C || !Boom || !Camera)return;
 const auto* Viewport=GetWorld()->GetGameViewport();
 const bool Focused=Viewport && Viewport->Viewport && Viewport->Viewport->HasFocus();
 Blocked|=!Focused;
 const bool Overhead=ViewMode==EForestViewMode::Overhead;
 const bool FirstPerson=ViewMode==EForestViewMode::FirstPerson;
 float X=0,Y=0;PC->GetInputMouseDelta(X,Y);
 if(bSkipLookDelta){X=0;Y=0;bSkipLookDelta=false;}
 bool HasActivity=bObserving || Blocked || X!=0 || Y!=0;
 if(FSlateApplication::IsInitialized())
 {
  const double InteractionTime=FSlateApplication::Get().GetLastUserInteractionTime();
  HasActivity|=InteractionTime!=LastUserInteractionTime;
  LastUserInteractionTime=InteractionTime;
 }
 // A held movement/button input counts as activity even without new key events.
 static const TArray<FKey> Keys=[](){TArray<FKey> Result;EKeys::GetAllKeys(Result);return Result;}();
 for(const FKey& Key:Keys)
  if(PC->IsInputKeyDown(Key) || PC->WasInputKeyJustReleased(Key)){HasActivity=true;break;}
 CameraIdleSeconds=HasActivity?0.f:FMath::Min(CameraIdleSeconds+DT,FMath::Max(0.f,CameraReturnIdleSeconds));
 if(Overhead && bObserving && !Blocked)
 {
  ObservationYaw=FMath::UnwindDegrees(ObservationYaw+X*ObservationSensitivity);
  ObservationPitch=FMath::Clamp(ObservationPitch+Y*ObservationSensitivity,-80.f-CameraPitch,-15.f-CameraPitch);
 }
 else if(Overhead && !HasActivity && CameraIdleSeconds>=FMath::Max(0.f,CameraReturnIdleSeconds))
 {
  ObservationYaw=FMath::FInterpTo(ObservationYaw,0.f,DT,9.f);
  ObservationPitch=FMath::FInterpTo(ObservationPitch,0.f,DT,9.f);
 }
 if(!Overhead && !Blocked)
 {
  LookYaw=FMath::UnwindDegrees(LookYaw+X*ObservationSensitivity+PC->GetInputAnalogKeyState(EKeys::Gamepad_RightX)*120.f*DT);
  LookPitch=FMath::Clamp(LookPitch+Y*ObservationSensitivity+PC->GetInputAnalogKeyState(EKeys::Gamepad_RightY)*90.f*DT,-80.f,80.f);
 }
 const float Yaw=Overhead?CameraYaw+ObservationYaw:LookYaw;
 Boom->TargetOffset=FVector(0,0,FirstPerson?C->BaseEyeHeight:Overhead?70.f:60.f);
 Boom->SocketOffset=ViewMode==EForestViewMode::ThirdPerson?FVector(0,45,0):FVector::ZeroVector;
 Boom->bDoCollisionTest=ViewMode==EForestViewMode::ThirdPerson;
 Boom->ProbeSize=15.f;
 const float Distance=Overhead?CameraDistance:FirstPerson?0.f:ThirdPersonDistance;
 Boom->TargetArmLength=FMath::FInterpTo(Boom->TargetArmLength,Distance,DT,8);
 Boom->SetWorldRotation(FRotator(Overhead?CameraPitch+ObservationPitch:LookPitch,Yaw,0));
 Camera->SetFieldOfView(Overhead?45.f:FirstPerson?85.f:75.f);
 C->GetMesh()->SetOwnerNoSee(FirstPerson);
 // The movement basis uses camera yaw only: looking up never tilts walking.
 PC->SetControlRotation(FRotator(0,Yaw,0));
 if(!Overhead)
 {
  C->SetActorRotation(FRotator(0,Yaw,0));C->GetMesh()->SetRelativeRotation(FRotator(0,-90,0));
 }
 else if(C->GetVelocity().SizeSquared2D()>9)
 {
  const FRotator Facing(0,C->GetVelocity().Rotation().Yaw-90,0);
  C->GetMesh()->SetWorldRotation(FMath::RInterpTo(C->GetMesh()->GetComponentRotation(),Facing,DT,12));
 }
 if(!Blocked && !PC->IsMoveInputIgnored())
 {
  const float Forward=(PC->IsInputKeyDown(EKeys::W)?1.f:0.f)-(PC->IsInputKeyDown(EKeys::S)?1.f:0.f)+PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY);
  const float Right=(PC->IsInputKeyDown(EKeys::D)?1.f:0.f)-(PC->IsInputKeyDown(EKeys::A)?1.f:0.f)+PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX);
  const FVector2D Axis=FVector2D(Forward,Right).GetClampedToMaxSize(1.f);
  const FRotationMatrix Basis(FRotator(0,Yaw,0));
  C->AddMovementInput(Basis.GetUnitAxis(EAxis::X),Axis.X);
  C->AddMovementInput(Basis.GetUnitAxis(EAxis::Y),Axis.Y);
  if(PC->WasInputKeyJustPressed(EKeys::SpaceBar) || PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom))C->Jump();
 }
 if(Blocked || PC->WasInputKeyJustReleased(EKeys::SpaceBar) || PC->WasInputKeyJustReleased(EKeys::Gamepad_FaceButton_Bottom))C->StopJumping();
}
void UForestIsometricPlayer::UpdateOcclusion()
{
 for(auto& W:Occluded)if(auto* M=W.Get())M->SetVisibility(true,false);Occluded.Reset();
 auto* C=Character.Get();if(!C || !Camera || ViewMode!=EForestViewMode::Overhead)return;
 FCollisionObjectQueryParams Objects;Objects.AddObjectTypesToQuery(ECC_WorldStatic);Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
 FCollisionQueryParams Params(SCENE_QUERY_STAT(ForestCameraRoof),true,C);TArray<FHitResult> Hits;
 GetWorld()->SweepMultiByObjectType(Hits,Camera->GetComponentLocation(),C->GetActorLocation()+FVector(0,0,40),FQuat::Identity,Objects,FCollisionShape::MakeSphere(180),Params);
 for(const auto& H:Hits)if(auto* M=Cast<UStaticMeshComponent>(H.GetComponent());IsCameraRoof(M) && M->IsVisible() && H.ImpactPoint.Z>C->GetActorLocation().Z+80){M->SetVisibility(false,false);Occluded.AddUnique(M);}
 const FVector P=C->GetActorLocation();
 const ULevel* IndoorLevel=nullptr;
 for(auto& W:OverheadMeshes)if(auto* M=W.Get();IsCameraRoof(M))
 {
  const FBox B=M->Bounds.GetBox();
  if(B.Min.Z>P.Z+120 && B.Min.Z<P.Z+1200 && P.X>B.Min.X-100 && P.X<B.Max.X+100 && P.Y>B.Min.Y-100 && P.Y<B.Max.Y+100)
  {IndoorLevel=M->GetOwner()->GetLevel();break;}
 }
 // Hide the nearby roof assembly when indoors, including collisionless beams.
 for(auto& W:OverheadMeshes)if(auto* M=W.Get();IsCameraRoof(M) && M->IsVisible() && IndoorLevel && M->GetOwner()->GetLevel()==IndoorLevel && FVector::DistSquared2D(M->Bounds.Origin,P)<FMath::Square(2200.f) && M->Bounds.GetBox().Max.Z>P.Z+80)
 {
  M->SetVisibility(false,false);Occluded.AddUnique(M);
 }
}
void UForestIsometricPlayer::HarvestNearby()
{
 auto* C=Character.Get();if(!C || MenuOpen())return;
 auto* Daily=GetWorld()->GetGameInstance()->GetSubsystem<UDailyMushroomSubsystem>();
 UStaticMeshComponent* Best=nullptr;float Dist=FMath::Square(230.f);
 for(auto& W:Mushrooms)if(auto* M=W.Get();M && !M->GetOwner()->IsHidden())
 {
  if(Daily && !Daily->IsMushroomAvailable(FName(*MushroomKey(M->GetStaticMesh()->GetName()))))continue;
  const float D=FVector::DistSquared(C->GetActorLocation(),M->Bounds.GetBox().GetClosestPointTo(C->GetActorLocation()));
  if(D>=Dist)continue;
  FCollisionQueryParams Params(SCENE_QUERY_STAT(ForestHarvestSight),false,C);Params.AddIgnoredActor(M->GetOwner());FHitResult Hit;
  if(GetWorld()->LineTraceSingleByChannel(Hit,C->GetActorLocation(),M->Bounds.Origin,ECC_Visibility,Params))continue;
  Best=M;Dist=D;
 }
 if(!Best)return;
 const FString Key=MushroomKey(Best->GetStaticMesh()->GetName());
 auto* Map=FindFProperty<FMapProperty>(C->GetClass(),TEXT("Backpack"));auto* K=Map?CastField<FStrProperty>(Map->KeyProp):nullptr;auto* V=Map?CastField<FIntProperty>(Map->ValueProp):nullptr;if(!K || !V)return;
 FScriptMapHelper Bag(Map,Map->ContainerPtrToValuePtr<void>(C));bool Found=false;
 for(int32 I=0;I<Bag.GetMaxIndex();++I)if(Bag.IsValidIndex(I) && K->GetPropertyValue(Bag.GetKeyPtr(I))==Key){V->SetPropertyValue(Bag.GetValuePtr(I),V->GetPropertyValue(Bag.GetValuePtr(I))+1);Found=true;break;}
 if(!Found){int32 One=1;Bag.AddPair(&Key,&One);}
 if(auto* Last=FindFProperty<FStrProperty>(C->GetClass(),TEXT("LastPicked")))Last->SetPropertyValue_InContainer(C,Key);
 Best->GetOwner()->SetActorHiddenInGame(true);Best->GetOwner()->SetActorEnableCollision(false);Harvested.AddUnique(Best->GetOwner());
 if(auto* F=C->FindFunction(TEXT("RefreshHUD")))C->ProcessEvent(F,nullptr);
 UE_LOG(LogTemp,Log,TEXT("Forest isometric harvest: %s"),*Key);
}
void UForestIsometricPlayer::EndPlay(const EEndPlayReason::Type Reason)
{
 if(CameraOcclusionParameters)UKismetMaterialLibrary::SetScalarParameterValue(this,CameraOcclusionParameters,TEXT("ForestCameraFadeEnabled"),0.f);
 if(SwimmingVolume)SwimmingVolume->Destroy();
 for(auto& W:Occluded)if(auto* M=W.Get())M->SetVisibility(true,false);
 if(auto* PC=Player.Get()){if(Controls)PC->PopInputComponent(Controls);PC->SetIgnoreLookInput(false);PC->bShowMouseCursor=true;}bObserving=false;
 if(auto* C=Character.Get()){C->GetMesh()->SetOwnerNoSee(false);if(bPawnInputWasEnabled)C->EnableInput(Player.Get());}
 Super::EndPlay(Reason);
 if(Minimap){Minimap->RemoveFromParent();Minimap=nullptr;}
}
void UForestIsometricWorldSubsystem::Tick(float)
{
 UWorld* W=GetWorld();if(!W || !W->IsGameWorld() || !W->GetMapName().Contains(TEXT("DemoMap")))return;
 auto* C=Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(W,0));
 if(!C || !C->IsPlayerControlled() || C->FindComponentByClass<UForestIsometricPlayer>())return;
 auto* Component=NewObject<UForestIsometricPlayer>(C,TEXT("ForestIsometricPlayer"));C->AddInstanceComponent(Component);Component->RegisterComponent();
}
TStatId UForestIsometricWorldSubsystem::GetStatId() const {RETURN_QUICK_DECLARE_CYCLE_STAT(UForestIsometricWorldSubsystem,STATGROUP_Tickables);}

void UForestIsometricPlayer::InitializeSwimming()
{
 auto* C=Character.Get();if(!C)return;
 SwimmingAnimation=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/Swimming/AS_Swimming.AS_Swimming"));
 WalkingAnimClass=C->GetMesh()->GetAnimClass();
 WalkingHalfHeight=C->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
 WalkingMeshOffset=C->GetMesh()->GetRelativeLocation();
 auto* Move=C->GetCharacterMovement();
 Move->GetNavAgentPropertiesRef().bCanSwim=true;
 Move->MaxSwimSpeed=SwimSpeed;Move->Buoyancy=1.f;
 for(TActorIterator<AActor> It(GetWorld());It;++It)
 {
  TArray<UStaticMeshComponent*> Parts;It->GetComponents(Parts);
  for(auto* M:Parts)
  {
   // Actor tags survive cooking and Blueprint construction scripts recreating
   // river spline meshes. Component tags identify explicitly authored surfaces.
   bool Water=M->ComponentHasTag(TEXT("ForestSwimSurface"));
   if(!Water && It->ActorHasTag(TEXT("ForestSwimWater")))
    for(int32 I=0;I<M->GetNumMaterials();++I)
     if(auto* Mat=M->GetMaterial(I);Mat && Mat->GetPathName().Contains(TEXT("/Water/")))Water=true;
   if(!Water)continue;
   M->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
   M->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
   M->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
   M->SetCollisionResponseToChannel(ECC_Camera,ECR_Ignore);
   WaterSurfaces.Add(M);
  }
 }
 FActorSpawnParameters Spawn;Spawn.ObjectFlags|=RF_Transient;Spawn.Owner=C;
 SwimmingVolume=GetWorld()->SpawnActor<APhysicsVolume>(APhysicsVolume::StaticClass(),FVector::ZeroVector,FRotator::ZeroRotator,Spawn);
 if(SwimmingVolume)
 {
  SwimmingVolume->bWaterVolume=true;SwimmingVolume->Priority=100;SwimmingVolume->FluidFriction=1.f;
  auto* Brush=SwimmingVolume->GetBrushComponent();Brush->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Brush->SetMobility(EComponentMobility::Movable);
  SwimmingBounds=NewObject<UBoxComponent>(SwimmingVolume,TEXT("ForestSwimmingBounds"));
  SwimmingVolume->AddInstanceComponent(SwimmingBounds);SwimmingVolume->SetRootComponent(SwimmingBounds);
  Brush->AttachToComponent(SwimmingBounds,FAttachmentTransformRules::KeepWorldTransform);
  SwimmingBounds->SetBoxExtent(FVector(500,500,300));
  SwimmingBounds->SetCollisionObjectType(ECC_WorldDynamic);
  SwimmingBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
  SwimmingBounds->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap);
  SwimmingBounds->SetGenerateOverlapEvents(true);
  SwimmingBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);SwimmingBounds->RegisterComponent();
 }
 UE_LOG(LogTemp,Log,TEXT("Forest swimming initialized: %d surfaces, animation=%s"),WaterSurfaces.Num(),*GetNameSafe(SwimmingAnimation));
}

void UForestIsometricPlayer::LeaveWater(const FHitResult* Shore)
{
 auto* C=Character.Get();if(!C)return;
 auto* Move=C->GetCharacterMovement();
 bSwimming=false;
 SwimmingBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 C->GetCapsuleComponent()->SetPhysicsVolume(GetWorld()->GetDefaultPhysicsVolume(),true);
 // Restore standing clearance before expanding the capsule at a shallow bank.
 FVector P=C->GetActorLocation();P.Z+=WalkingHalfHeight-C->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
 if(Shore)P.Z=FMath::Max(P.Z,Shore->ImpactPoint.Z+WalkingHalfHeight+2.f);
 C->SetActorLocation(P,false);
 C->GetCapsuleComponent()->SetCapsuleHalfHeight(WalkingHalfHeight,true);
 C->GetMesh()->SetRelativeLocation(WalkingMeshOffset);
 C->GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
 C->GetMesh()->SetAnimInstanceClass(WalkingAnimClass);
 Move->SetMovementMode(MOVE_Falling);
 UE_LOG(LogTemp,Log,TEXT("Forest swimming: exited water, locomotion restored"));
}

void UForestIsometricPlayer::UpdateSwimming(float DT)
{
 auto* C=Character.Get();if(!C || !SwimmingAnimation || !SwimmingBounds)return;
 auto* Move=C->GetCharacterMovement();const FVector P=C->GetActorLocation();
 FCollisionQueryParams WaterQuery(SCENE_QUERY_STAT(ForestSwimmingSurface),true,C);
 bool HasSurface=false;float SurfaceZ=-MAX_flt;
 // Trace the actual deformed triangles rather than rectangular river bounds.
 for(auto& Weak:WaterSurfaces)if(auto* M=Weak.Get())
 {
  const FBox B=M->Bounds.GetBox();
  if(P.X<B.Min.X || P.X>B.Max.X || P.Y<B.Min.Y || P.Y>B.Max.Y || P.Z<B.Min.Z-300 || P.Z>B.Max.Z+600)continue;
  FHitResult H;
  if(M->LineTraceComponent(H,P+FVector(0,0,300),P-FVector(0,0,600),WaterQuery) && H.ImpactNormal.Z>.25f && H.ImpactPoint.Z>SurfaceZ)
  {HasSurface=true;SurfaceZ=H.ImpactPoint.Z;}
 }
 FHitResult Ground;
 FCollisionQueryParams GroundQuery(SCENE_QUERY_STAT(ForestSwimmingShore),true,C);
 GroundQuery.AddIgnoredActor(SwimmingVolume);
 for(auto& Weak:WaterSurfaces)if(auto* M=Weak.Get())GroundQuery.AddIgnoredComponent(M);
 const bool GroundHit=HasSurface && GetWorld()->LineTraceSingleByChannel(Ground,P+FVector(0,0,70),P-FVector(0,0,500),ECC_Visibility,GroundQuery);
 const bool Shore=GroundHit && Move->IsWalkable(Ground) && Ground.ImpactPoint.Z>=SurfaceZ-35.f;
 if(bSwimming)
 {
  if(!HasSurface || Shore || P.Z>SurfaceZ+150.f)
  {LeaveWater(Shore?&Ground:nullptr);return;}
 }
 else
 {
  const float Feet=P.Z-C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
  if(!HasSurface || Shore || Feet>SurfaceZ-15.f || P.Z>SurfaceZ+90.f || Move->IsFlying())return;
  bSwimming=true;
  C->GetCapsuleComponent()->SetCapsuleHalfHeight(FMath::Max(44.f,C->GetCapsuleComponent()->GetUnscaledCapsuleRadius()),true);
  C->GetMesh()->SetRelativeLocation(FVector(0,0,-60));
  C->GetMesh()->PlayAnimation(SwimmingAnimation,true);
  Move->Velocity.Z=0;
  UE_LOG(LogTemp,Log,TEXT("Forest swimming: entered water at Z=%.1f"),SurfaceZ);
 }
 SwimmingVolume->SetActorLocation(FVector(P.X,P.Y,SurfaceZ-250.f));
 SwimmingBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 C->GetCapsuleComponent()->SetPhysicsVolume(SwimmingVolume,true);
 if(!Move->IsSwimming())Move->SetMovementMode(MOVE_Swimming);
 Move->MaxSwimSpeed=SwimSpeed;Move->Velocity.Z=0;
 FVector SurfacePosition=C->GetActorLocation();
 SurfacePosition.Z=FMath::FInterpTo(SurfacePosition.Z,SurfaceZ-15.f,DT,10.f);
 C->SetActorLocation(SurfacePosition,true);
 if(auto* Anim=C->GetMesh()->GetSingleNodeInstance())Anim->SetPlayRate(FMath::Lerp(.65f,1.f,FMath::Clamp(C->GetVelocity().Size2D()/SwimSpeed,0.f,1.f)));
}
