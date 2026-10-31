#include "ForestZoneVisuals.h"
#include "Components/LocalFogVolumeComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

bool FForestZoneVisualPreset::IsValid() const
{
 auto ValidColor=[](const FLinearColor& C){return FMath::IsFinite(C.R) && FMath::IsFinite(C.G) && FMath::IsFinite(C.B) && FMath::IsFinite(C.A) && C.R>=0 && C.G>=0 && C.B>=0;};
 return !Id.IsNone() && ValidColor(FogColor) && ValidColor(ResourceColor) &&
  FMath::IsFinite(FogDensity) && FogDensity>=0 && FogDensity<=2 &&
  FMath::IsFinite(FogRadius) && FogRadius>=200 && FogRadius<=1500 &&
  FMath::IsFinite(ResourceScale) && ResourceScale>=.5f && ResourceScale<=1.5f &&
  FMath::IsFinite(Glow) && Glow>=0 && Glow<=2;
}
bool FForestZoneVisualPreset::Find(FName Id,const TArray<FForestZoneVisualPreset>& Rows,FForestZoneVisualPreset& Out)
{
 // Explicit normal suppresses any fallback. Unknown keys are not silently assigned a look.
 if(Id.IsNone() || Id==TEXT("normal"))return false;
 for(const auto& Row:Rows)if(Row.Id==Id && Row.IsValid()){Out=Row;return true;}
 return false;
}
void FForestZoneVisualPreset::ResolveRegionColor(FName ZoneId)
{
 if(!bUseRegionColor)return;
 const FString Zone=ZoneId.ToString();
 ResourceColor=Zone.StartsWith(TEXT("fire"))?FLinearColor(1,.32f,.06f):Zone.StartsWith(TEXT("wind"))?FLinearColor(.25f,1,.55f):FLinearColor(.15f,.55f,1);
 FogColor=ResourceColor*.55f;FogColor.A=1;
}
AForestZoneVisuals::AForestZoneVisuals()
{
 PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickInterval=.1f;
 auto* Root=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));SetRootComponent(Root);
 Motes=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Motes"));Motes->SetupAttachment(Root);
 Motes->SetCollisionEnabled(ECollisionEnabled::NoCollision);Motes->SetCanEverAffectNavigation(false);Motes->SetCastShadow(false);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
 static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
 MoteMesh=Sphere.Object;GlowMaterial=Material.Object;
 Tags.Add(TEXT("Forest.ZoneProtected"));SetActorEnableCollision(false);
}
void AForestZoneVisuals::Configure(const TArray<FVector>& Points,const FForestZoneVisualPreset& Preset)
{
 Appearance=Preset;
 for(auto F:Fog)if(F)F->DestroyComponent();Fog.Reset();Motes->ClearInstances();Origins.Reset();
 if(MoteMesh && GlowMaterial)
 {
  Motes->SetStaticMesh(MoteMesh);GlowInstance=UMaterialInstanceDynamic::Create(GlowMaterial,this);
  GlowInstance->SetVectorParameterValue(TEXT("Color"),Preset.ResourceColor*Preset.Glow);
  Motes->SetMaterial(0,GlowInstance);
 }
 for(const FVector& Point:Points)
 {
  auto* F=NewObject<ULocalFogVolumeComponent>(this);AddInstanceComponent(F);F->SetupAttachment(GetRootComponent());
  F->SetMobility(EComponentMobility::Movable);
  F->RadialFogExtinction=Preset.FogDensity;F->HeightFogExtinction=0;F->FogPhaseG=0;
  F->FogAlbedo=Preset.FogColor;F->FogEmissive=Preset.FogColor*.04f;
  F->SetRelativeLocation(Point+FVector(0,0,90));F->SetRelativeScale3D(FVector(Preset.FogRadius/ULocalFogVolumeComponent::GetBaseVolumeSize()));
  F->RegisterComponent();Fog.Add(F);
  if(Preset.Glow>0 && GlowInstance)for(int32 I=0;I<5;++I)
  {
   const float A=I*2.399963f+Origins.Num();const FVector O=Point+FVector(FMath::Cos(A)*200,FMath::Sin(A)*200,90+I*25);
   Origins.Add(O);Motes->AddInstance(FTransform(FQuat::Identity,O,FVector(.10f)));
  }
 }
 SetActorTickEnabled(!Origins.IsEmpty());
}
void AForestZoneVisuals::Tick(float DT)
{
 Super::Tick(DT);const float T=GetWorld()->GetTimeSeconds();
 for(int32 I=0;I<Origins.Num();++I)
 {
  const FVector Offset(45*FMath::Sin(T*.5f+I),45*FMath::Cos(T*.4f+I),40*FMath::Sin(T*.8f+I));
  const float S=.08f+.035f*(1+FMath::Sin(T*1.4f+I));
  Motes->UpdateInstanceTransform(I,FTransform(FQuat::Identity,Origins[I]+Offset,FVector(S)),false,false,true);
 }
 if(!Origins.IsEmpty())Motes->MarkRenderStateDirty();
}
