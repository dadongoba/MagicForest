#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "PotionMimicWorldSubsystem.generated.h"

class AActor;
class AStaticMeshActor;

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

private:
	struct FBoundPotion
	{
		TWeakObjectPtr<AStaticMeshActor> Actor;
		FTransform HomeTransform;
		TEnumAsByte<ECollisionEnabled::Type> HomeCollision = ECollisionEnabled::QueryAndPhysics;
		float Phase = 0.0f;
		float Radius = 70.0f;
		float Height = 85.0f;
	};

	TArray<FBoundPotion> BoundPotions;
	TWeakObjectPtr<AActor> WorkbenchCauldron;
	TWeakObjectPtr<AActor> MimicBookActor;
	FVector LastMimicBookLocation = FVector::ZeroVector;
	float TimeSinceBeginPlay = 0.0f;
	float TimeSinceLastDiscovery = 0.0f;
	float FlightBlend = 0.0f;
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
	bool IsMimicBookFolded(const AActor* Actor) const;
	FVector GetFlightCentre() const;
};
