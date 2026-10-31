#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Subsystems/WorldSubsystem.h"
#include "ForestIsometricPlayer.generated.h"

class ACharacter;
class APlayerController;
class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UInputComponent;
class UForestMinimapWidget;
class UAnimSequence;
class UBoxComponent;
class APhysicsVolume;
class UMaterialParameterCollection;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

UENUM(BlueprintType)
enum class EForestViewMode : uint8
{
 Overhead,
 FirstPerson,
 ThirdPerson
};

/** Owns the three camera modes and their movement on the inventory/harvest pawn. */
UCLASS(ClassGroup=Forest, meta=(BlueprintSpawnableComponent))
class DEMO3_UE58_API UForestIsometricPlayer : public UActorComponent
{
 GENERATED_BODY()
public:
 UForestIsometricPlayer();
 virtual void BeginPlay() override;
 virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction) override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Camera") float CameraYaw=-45.f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Camera") float CameraPitch=-55.f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Camera") float CameraDistance=1350.f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Camera") float ObservationSensitivity=.25f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Camera",meta=(ClampMin="0.0",Units="s")) float CameraReturnIdleSeconds=5.f;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Camera") EForestViewMode ViewMode=EForestViewMode::Overhead;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Camera") float ThirdPersonDistance=400.f;
 UFUNCTION(BlueprintCallable,Category="Camera") void CycleViewMode();
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Camera") bool bObserving=false;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Swimming") bool bSwimming=false;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Swimming") float SwimSpeed=230.f;
 UFUNCTION(BlueprintCallable) void HarvestNearby();
 bool BeginGather(class AForestElementResource* Resource);
 bool ActionBusy() const { return bGathering || bClimbing; }
 void RefreshGameplayInput() { ApplyGameplayInputMode(); }
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Actions") bool bGathering=false;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Actions") bool bClimbing=false;
 UPROPERTY(EditAnywhere,Category="Climbing") float ClimbSpeed=110.f;
 UPROPERTY(EditAnywhere,Category="Actions",meta=(ClampMin="0.2",Units="s")) float GatherDuration=1.f;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Climbing") bool bCanClimb=false;
 UPROPERTY(EditAnywhere,Category="Climbing",meta=(ClampMin="0.0")) float ClimbInputBuffer=.35f;
 UPROPERTY(EditAnywhere,Category="Climbing",meta=(ClampMin="0.0")) float ClimbSurfaceGrace=.15f;
 UFUNCTION(BlueprintCallable,Category="Forest|Migration") static bool RebuildNavigation(UObject* WorldContextObject);
private:
 friend class FForestClimbCollisionTest;
 UPROPERTY() TArray<TObjectPtr<UAnimSequence>> GatherAnimations;
 UPROPERTY() TArray<TObjectPtr<UAnimSequence>> ClimbAnimations;
 UPROPERTY() TObjectPtr<UAnimSequence> ActiveActionAnimation;
 TWeakObjectPtr<class AForestElementResource> GatheringResource;
 FVector ClimbNormal=FVector::ForwardVector;
 TWeakObjectPtr<class UPrimitiveComponent> ClimbSurface;
 bool bClimbSurfaceWasIgnored=false,bClimbExitPending=false;
 FVector MantleStart=FVector::ZeroVector,MantleLift=FVector::ZeroVector,MantleEnd=FVector::ZeroVector;
 float ActionAge=0,MantleAge=0;
 float ClimbRequestRemaining=0,ClimbReattachCooldown=0,ClimbLostSurfaceAge=0,ClimbProbeAge=0;
 bool bGatherInWater=false,bMantling=false;
 bool bGatherAnimationStarted=false;
 UPROPERTY(EditAnywhere,Category="Actions",meta=(ClampMin="0.05",ClampMax="0.5")) float GatherHighlightDuration=.18f;
 bool bSavedControllerYaw=false,bSavedOrientToMovement=false;
 void InitializeActions();
 void UpdateActions(float DeltaTime,bool bBlocked);
 void ToggleClimbing();
 bool FindClimbCandidate(struct FHitResult& Hit) const;
 bool BeginClimbing(const struct FHitResult& Hit);
 void StopClimbing(bool bForce=false);
 bool SetClimbCollisionSurface(class UPrimitiveComponent* Surface);
 void RestoreClimbCollision();
 bool IsClimbCapsuleClear(const FVector& Position,class UPrimitiveComponent* Ignore=nullptr) const;
 bool FindSafeClimbExit(FVector& Position) const;
 void MoveClimbing(const FVector& Delta,const FQuat& Rotation,struct FHitResult& Hit);
 bool FindClimbSurface(const FVector& Position,const FVector& Direction,struct FHitResult& Hit) const;
 bool TryMantle();
 void PlayAction(UAnimSequence* Animation,bool bLoop);
 void RestoreLocomotion();
 void FinishGather(bool bCommit);
 UPROPERTY() TObjectPtr<USpringArmComponent> Boom;
 UPROPERTY() TObjectPtr<UCameraComponent> Camera;
 UPROPERTY() TObjectPtr<UCameraComponent> FirstPersonCamera;
 UPROPERTY() TObjectPtr<UInputMappingContext> TemplateMappingContext;
 UPROPERTY() TObjectPtr<UInputAction> TemplateMoveAction;
 UPROPERTY() TObjectPtr<UInputAction> TemplateLookAction;
 UPROPERTY() TObjectPtr<UInputAction> TemplateJumpAction;
 UPROPERTY() TObjectPtr<UInputComponent> Controls;
 UPROPERTY() TObjectPtr<UForestMinimapWidget> Minimap;
 UPROPERTY() TObjectPtr<UMaterialParameterCollection> CameraOcclusionParameters;
 UPROPERTY() TObjectPtr<UAnimSequence> SwimmingAnimation;
 UPROPERTY() TObjectPtr<APhysicsVolume> SwimmingVolume;
 UPROPERTY() TObjectPtr<UBoxComponent> SwimmingBounds;
 UPROPERTY() TSubclassOf<class UAnimInstance> WalkingAnimClass;
 TArray<TWeakObjectPtr<UStaticMeshComponent>> WaterSurfaces;
 float WalkingHalfHeight=96.f;
 FVector WalkingMeshOffset=FVector::ZeroVector;
 void InitializeSwimming();
 void UpdateSwimming(float DeltaTime);
 void LeaveWater(const struct FHitResult* Shore);
 TWeakObjectPtr<ACharacter> Character;
 TWeakObjectPtr<APlayerController> Player;
 TArray<TWeakObjectPtr<UStaticMeshComponent>> Mushrooms;
 TArray<TWeakObjectPtr<AActor>> Harvested;
 TArray<TWeakObjectPtr<UStaticMeshComponent>> Occluded;
 TArray<TWeakObjectPtr<UStaticMeshComponent>> OverheadMeshes;
 float ScanDelay=0,RoofDelay=0;
 int32 Day=0;
 bool bMenuWasOpen=false;
 float ObservationYaw=0,ObservationPitch=0;
 float CameraIdleSeconds=0;
 double LastUserInteractionTime=0;
 bool bAddedTemplateContext=false;
 bool bPawnInputWasEnabled=false;
 bool bMapWasOpen=false;
 bool bSkipLookDelta=true;
 float SavedMouseX=0,SavedMouseY=0;
 bool bHadMousePosition=false;
 void StartObserving();
 void StopObserving();
 void ZoomIn();
 void ZoomOut();
 void InteractNearby();
 void ClickHarvestTarget();
 void ToggleMap();
 void ApplyGameplayInputMode();
 void ApplyTemplateView();
 bool CanUseTemplateInput() const;
 void TemplateMove(const FInputActionValue& Value);
 void TemplateLook(const FInputActionValue& Value);
 void TemplateJumpStart();
 void TemplateJumpEnd();
 void UpdateViewAndMovement(float DeltaTime,bool bBlocked);
 bool MenuOpen() const;
 void UpdateOcclusion();
};

UCLASS()
class DEMO3_UE58_API UForestIsometricWorldSubsystem : public UTickableWorldSubsystem
{
 GENERATED_BODY()
public:
 virtual void Tick(float DeltaTime) override;
 virtual TStatId GetStatId() const override;
 virtual bool IsTickableInEditor() const override { return false; }
};
