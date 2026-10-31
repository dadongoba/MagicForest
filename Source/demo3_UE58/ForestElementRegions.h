#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/WorldSubsystem.h"
#include "AlchemyGameplayTypes.h"
#include "ForestElementRegions.generated.h"

class ACharacter;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

USTRUCT(BlueprintType)
struct FForestElementRegion
{
 GENERATED_BODY()
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) EAlchemyElement Element=EAlchemyElement::Fire;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FVector Center=FVector::ZeroVector;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) float Radius=4000.f;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FString AnchorId;
};

/** A fixed elemental source, rather than a random daily mushroom card. */
UCLASS()
class DEMO3_UE58_API AForestElementResource : public AActor
{
 GENERATED_BODY()
public:
 AForestElementResource();
 void SetHarvestHighlighted(bool bHighlighted);
 virtual void Tick(float DeltaTime) override;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Mesh;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) EAlchemyElement Element=EAlchemyElement::Fire;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool bWaterShell=false;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool bCollected=false;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FString ResourceId;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FVector RestLocation=FVector::ZeroVector;
 float FloatPhase=0;
private:
 UPROPERTY() TObjectPtr<UMaterialInterface> HighlightBase;
 UPROPERTY(Transient) TObjectPtr<UMaterialInterface> SavedOverlay;
 UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> HighlightOverlay;
 bool bHarvestHighlighted=false;

};

UCLASS()
class DEMO3_UE58_API UForestRegionHarvestSave : public USaveGame
{
 GENERATED_BODY()
public:
 UPROPERTY() TSet<FString> Collected;
};

/** Finds the three rune stones and grows only region-appropriate resources. */
UCLASS(Config=ForestResources,DefaultConfig)
class DEMO3_UE58_API UForestElementRegions : public UTickableWorldSubsystem
{
 GENERATED_BODY()
public:
 UForestElementRegions();
 virtual void Tick(float DeltaTime) override;
 virtual TStatId GetStatId() const override;
 virtual bool IsTickableInEditor() const override {return false;}
 virtual void Deinitialize() override;
 UPROPERTY(Config,EditAnywhere,Category="Regions",meta=(ClampMin="1000")) float FireRadius=4000.f;
 UPROPERTY(Config,EditAnywhere,Category="Regions",meta=(ClampMin="1000")) float WindRadius=4000.f;
 UPROPERTY(Config,EditAnywhere,Category="Regions",meta=(ClampMin="1000")) float WaterRadius=6000.f;
 UPROPERTY(Config,EditAnywhere,Category="Resources",meta=(ClampMin="1",ClampMax="150")) int32 MushroomsPerRegion=36;
 UPROPERTY(Config,EditAnywhere,Category="Resources",meta=(ClampMin="1",ClampMax="100")) int32 ShellsPerRegion=24;
 UPROPERTY(Config,EditAnywhere,Category="Resources",meta=(ClampMin="1",ClampMax="9")) int32 MaterialPipValue=3;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Regions") TArray<FForestElementRegion> Regions;
 bool IsRegionalWorld() const;
 bool CanHarvestResource(const ACharacter* Character,bool bSwimming,const AForestElementResource* Resource) const;
 bool TryHarvest(ACharacter* Character,bool bSwimming);
 bool CollectResource(ACharacter* Character,bool bSwimming,AForestElementResource* Resource);
 FString GetHarvestHint(const ACharacter* Character,bool bSwimming) const;
private:
 // Hard asset references on the CDO also make these assets visible to the cooker.
 UPROPERTY() TObjectPtr<UStaticMesh> FireMushroom;
 UPROPERTY() TObjectPtr<UStaticMesh> WindMushroom;
 UPROPERTY() TArray<TObjectPtr<UStaticMesh>> ShellMeshes;
 UPROPERTY() TObjectPtr<UForestRegionHarvestSave> HarvestSave;
 UPROPERTY() TArray<TObjectPtr<AForestElementResource>> Resources;
 TArray<TWeakObjectPtr<UStaticMeshComponent>> WaterSurfaces;
 struct FOriginalMesh {TWeakObjectPtr<UStaticMeshComponent> Mesh;bool bVisible=true;ECollisionEnabled::Type Collision=ECollisionEnabled::NoCollision;};
 TArray<FOriginalMesh> OriginalMushrooms;
 TSet<TWeakObjectPtr<UStaticMeshComponent>> SuppressedMeshes;
 int32 CurrentDay=0;
 bool bInitialized=false;
 bool bReportedMissingDependencies=false;
 float DiscoverDelay=0;
 int32 BuildRegion=0,BuildAttempt=0,BuildCount=0;
 FRandomStream PlacementRandom;
 bool Discover();
 void BeginRegrowth(int32 Day);
 void GrowCandidates(int32 Budget);
 bool FindWater(const FVector& Point,FVector& Surface) const;
 bool FindGround(const FVector& Point,FHitResult& Ground) const;
 bool IsInsideRegion(const FForestElementRegion& Region,const FVector& Point) const;
 AForestElementResource* Nearest(const ACharacter* Character,float Range) const;
 FString CollectionKey(const AForestElementResource* Resource) const;
};
