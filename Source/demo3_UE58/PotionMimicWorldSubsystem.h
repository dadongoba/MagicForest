#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "PotionMimicWorldSubsystem.generated.h"

class AActor;
class AStaticMeshActor;
class UMaterialInterface;

/**
 * Finds the seven potion bottles placed around the Witch House workbench cauldron
 * and gives them a small flocking flight controlled by MimicBook.
 *
 * The original level actors are kept in place and restored when flight stops. The
 * system deliberately identifies the workbench as the cauldron whose nearby potion
 * cluster is closest to seven items, so the shop-window and storage bottles remain
 * untouched even when the level is instanced at a different world transform.
 */
UCLASS()
class DEMO3_UE58_API UPotionMimicWorldSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickableInEditor() const override { return false; }

	/** Starts the bottle flight. An optional MimicBook becomes the moving centre. */
	UFUNCTION(BlueprintCallable, Category = "Witch House|Potion Mimic")
	void StartWorkbenchPotionFlight(AActor* InMimicBookActor);

	/** Returns every bound bottle to its original transform. */
	UFUNCTION(BlueprintCallable, Category = "Witch House|Potion Mimic")
	void StopWorkbenchPotionFlight(bool bSnapHome = false);

	/** Changes the leader without restarting the flight. Pass null for workbench-centred flight. */
	UFUNCTION(BlueprintCallable, Category = "Witch House|Potion Mimic")
	void SetMimicBookActor(AActor* InMimicBookActor);

	/** Enables or disables the automatic showcase used when no MimicBook is placed. */
	UFUNCTION(BlueprintCallable, Category = "Witch House|Potion Mimic")
	void SetAutomaticWorkbenchFlight(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Witch House|Potion Mimic")
	int32 GetBoundWorkbenchPotionCount() const;

	UFUNCTION(BlueprintPure, Category = "Witch House|Potion Mimic")
	bool IsWorkbenchPotionFlightActive() const { return bFlightRequested; }

	/**
	 * Marks one of the seven bottles as magically inactive. Inactive is distinct
	 * from empty: the liquid remains, but darkens and stops moving while the bottle
	 * drops out of the MimicBook flight.
	 */
	UFUNCTION(BlueprintCallable, Category = "Witch House|Potion Mimic|Inactive")
	bool SetWorkbenchPotionInactive(int32 PotionIndex, bool bInactive);

	UFUNCTION(BlueprintCallable, Category = "Witch House|Potion Mimic|Inactive")
	bool SetWorkbenchPotionActorInactive(AActor* PotionActor, bool bInactive);

	UFUNCTION(BlueprintCallable, Category = "Witch House|Potion Mimic|Inactive")
	void SetAllWorkbenchPotionsInactive(bool bInactive);

	UFUNCTION(BlueprintPure, Category = "Witch House|Potion Mimic|Inactive")
	bool IsWorkbenchPotionInactive(int32 PotionIndex) const;

	UFUNCTION(BlueprintPure, Category = "Witch House|Potion Mimic|Inactive")
	int32 GetInactiveWorkbenchPotionCount() const;

 bool GetWorkbenchPotionHomeTransform(const AActor* Actor,FTransform& Out) const;

private:
	struct FBoundPotion
	{
		TWeakObjectPtr<AStaticMeshActor> Actor;
		FTransform HomeTransform;
		FTransform InactiveTransform;
		TEnumAsByte<ECollisionEnabled::Type> HomeCollision = ECollisionEnabled::QueryAndPhysics;
		TArray<TObjectPtr<UMaterialInterface>> HomeMaterials;
		float Phase = 0.0f;
		float Radius = 70.0f;
		float Height = 85.0f;
		float LocalFlightBlend = 0.0f;
		bool bInactive = false;
	};

	TArray<FBoundPotion> BoundPotions;
	TWeakObjectPtr<AActor> WorkbenchCauldron;
	TWeakObjectPtr<AActor> MimicBookActor;
	FVector LastMimicBookLocation = FVector::ZeroVector;
	float TimeSinceBeginPlay = 0.0f;
	float TimeSinceLastDiscovery = 0.0f;
	float FlightClock = 0.0f;
	float MimicBookSpeed = 0.0f;
	bool bFlightRequested = false;
	bool bAutomaticWorkbenchFlight = true;
	bool bHasSeenMimicBook = false;

	void DiscoverWorkbenchPotions();
	void DiscoverMimicBook();
	void UpdateBookDrivenState(float DeltaTime);
	void UpdatePotionTransforms(float DeltaTime);
	void RestorePotion(FBoundPotion& Potion, bool bSnapHome);
	void ApplyInactiveAppearance(FBoundPotion& Potion);
	void RestoreActiveAppearance(FBoundPotion& Potion);
	bool SetPotionInactive(FBoundPotion& Potion, bool bInactive);
	bool IsMimicBookFolded(const AActor* Actor) const;
	FVector GetFlightCentre() const;
};
