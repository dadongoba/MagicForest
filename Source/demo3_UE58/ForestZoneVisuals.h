#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ForestZoneVisuals.generated.h"
class ULocalFogVolumeComponent;
class UInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/** Appearance only: no score thresholds, resource values or gameplay penalties. */
USTRUCT(BlueprintType)
struct DEMO3_UE58_API FForestZoneVisualPreset
{
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FName Id;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FLinearColor FogColor=FLinearColor(.25f,.2f,.15f);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FLinearColor ResourceColor=FLinearColor(.25f,.23f,.2f);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float FogDensity=.35f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float FogRadius=850.f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float ResourceScale=1.f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float Glow=.0f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bShowInactiveResources=true;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bUseRegionColor=false;
 bool IsValid() const;
 void ResolveRegionColor(FName ZoneId);
 static bool Find(FName Id,const TArray<FForestZoneVisualPreset>& Rows,FForestZoneVisualPreset& Out);
};

/** Soft local fog patches and drifting motes; spawned only for an active visual preset. */
UCLASS(NotBlueprintable,Transient)
class DEMO3_UE58_API AForestZoneVisuals : public AActor
{
 GENERATED_BODY()
public:
 AForestZoneVisuals();
 void Configure(const TArray<FVector>& SurfacePoints,const FForestZoneVisualPreset& Preset);
 virtual void Tick(float DeltaTime) override;
private:
 UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Motes;
 UPROPERTY() TArray<TObjectPtr<ULocalFogVolumeComponent>> Fog;
 UPROPERTY() TObjectPtr<UStaticMesh> MoteMesh;
 UPROPERTY() TObjectPtr<UMaterialInterface> GlowMaterial;
 UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> GlowInstance;
 TArray<FVector> Origins;
 FForestZoneVisualPreset Appearance;
};
