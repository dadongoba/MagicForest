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
 UFUNCTION(BlueprintCallable,Category="Forest|Migration") static bool RebuildNavigation(UObject* WorldContextObject);
private:
 UPROPERTY() TObjectPtr<USpringArmComponent> Boom;
 UPROPERTY() TObjectPtr<UCameraComponent> Camera;
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
 float LookYaw=0,LookPitch=-15.f;
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
 void ToggleMap();
 void ApplyGameplayInputMode();
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
