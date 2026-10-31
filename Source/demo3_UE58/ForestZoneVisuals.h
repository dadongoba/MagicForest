#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ForestZoneVisuals.generated.h"
class ULocalFogVolumeComponent;
class UInstancedStaticMeshComponent;
class UDecalComponent;
class UTexture2D;
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
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FLinearColor GroundColor=FLinearColor(.22f,.18f,.12f);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float GroundOpacity=.65f;
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
 bool ConfigureGround(const FVector& Center,float Radius,UTexture2D* ColorMask,UTexture2D* PackedMask,const FForestZoneVisualPreset& Preset);
 virtual void Tick(float DeltaTime) override;
private:
 UPROPERTY() TObjectPtr<UDecalComponent> Ground;
 UPROPERTY() TObjectPtr<UMaterialInterface> GroundBase;
 UPROPERTY() TObjectPtr<UTexture2D> FlatNormal;
 UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> GroundInstance;
 UPROPERTY() TObjectPtr<UTexture2D> GroundColorMask;
 UPROPERTY() TObjectPtr<UTexture2D> GroundPackedMask;
 UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Motes;
 UPROPERTY() TArray<TObjectPtr<ULocalFogVolumeComponent>> Fog;
 UPROPERTY() TObjectPtr<UStaticMesh> MoteMesh;
 UPROPERTY() TObjectPtr<UMaterialInterface> GlowMaterial;
 UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> GlowInstance;
 TArray<FVector> Origins;
 FForestZoneVisualPreset Appearance;
};
