#include "ForestElementRegions.h"
#include "ForestWorldZones.h"
#include "ForestZonePolicy.h"
#include "ForestHarvestMagic.h"
#include "ForestIsometricPlayer.h"
#include "DailyMushroomSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
 const TCHAR* HarvestSlot=TEXT("ForestRegionalHarvests");
 bool IsWaterMesh(const UStaticMeshComponent* Mesh)
 {
  if(Mesh->ComponentHasTag(TEXT("ForestSwimSurface")))return true;
  if(!Mesh->GetOwner()->ActorHasTag(TEXT("ForestSwimWater")))return false;
  for(int32 I=0;I<Mesh->GetNumMaterials();++I)
   if(const auto* Mat=Mesh->GetMaterial(I);Mat && Mat->GetPathName().Contains(TEXT("/Water/")))return true;
  return false;
 }
 bool IsNaturalGround(const FHitResult& Hit)
 {
  if(!Hit.GetActor() || Hit.ImpactNormal.Z<.75f)return false;
  if(Hit.GetActor()->GetClass()->GetName().Contains(TEXT("Landscape")))return true;
  const auto* Mesh=Cast<UStaticMeshComponent>(Hit.GetComponent());
  if(!Mesh || !Mesh->GetStaticMesh())return false;
  if(Mesh->ComponentHasTag(TEXT("ForestResourceGround")))return true;
  const FString Name=Mesh->GetStaticMesh()->GetName();
  return Name.Contains(TEXT("Ground")) || Name.Contains(TEXT("Terrain")) || Name.Contains(TEXT("Cliff")) || Name.Contains(TEXT("Rock"));
 }
}

AForestElementResource::AForestElementResource()
{
 PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickInterval=1.f/30.f;
 Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ResourceMesh"));SetRootComponent(Mesh);
 Mesh->SetMobility(EComponentMobility::Movable);Mesh->SetCanEverAffectNavigation(false);
 Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Mesh->SetCollisionObjectType(ECC_WorldDynamic);
 Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);Mesh->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
 Tags.Add(TEXT("ForestRegionalResource"));
 static ConstructorHelpers::FObjectFinder<UMaterialInterface> Highlight(TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
 HighlightBase=Highlight.Object;
 static ConstructorHelpers::FObjectFinder<UMaterialInterface> Inactive(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
 InactiveBase=Inactive.Object;
}
void AForestElementResource::SetHarvestHighlighted(bool Highlighted)
{
 if(!Mesh || Highlighted==bHarvestHighlighted)return;
 if(Highlighted)
 {
  if(!HighlightBase)return;
  SavedOverlay=Mesh->GetOverlayMaterial();
  if(!HighlightOverlay)HighlightOverlay=UMaterialInstanceDynamic::Create(HighlightBase,this);
  const FLinearColor Color=Element==EAlchemyElement::Fire?FLinearColor(4,1.2f,.25f):Element==EAlchemyElement::Wind?FLinearColor(1.2f,4,1.5f):FLinearColor(.5f,2.5f,4);
  HighlightOverlay->SetVectorParameterValue(TEXT("Color"),Color);HighlightOverlay->SetVectorParameterValue(TEXT("EmissiveColor"),Color);
  Mesh->SetOverlayMaterial(HighlightOverlay);
 }
 else{Mesh->SetOverlayMaterial(SavedOverlay);SavedOverlay=nullptr;}
 bHarvestHighlighted=Highlighted;
}
void AForestElementResource::ApplyZoneAvailability(bool Available,bool HideUnavailable)
{
 const bool Hidden=!Available && HideUnavailable;
 if(Available==bZoneAvailable && Hidden==bZoneHidden)return;
 bZoneAvailable=Available;bZoneHidden=Hidden;
 if(!Available)SetHarvestHighlighted(false);
 const auto P=ForestZonePolicy::ResourcePresentation(bCollected,Available,HideUnavailable,bWaterShell);
 SetActorHiddenInGame(P.Hidden);SetActorEnableCollision(P.Collision);SetActorTickEnabled(P.Tick);
}
void AForestElementResource::ApplyZoneAppearance(const FForestZoneVisualPreset* Preset)
{
 if(!Mesh)return;
 const bool Inactive=!bZoneAvailable;
 if(Preset && AppliedZoneAppearance==Preset->Id && bAppearanceInactive==Inactive)return;
 if(!Preset && AppliedZoneAppearance.IsNone())return;
 const bool WasHighlighted=bHarvestHighlighted;if(WasHighlighted)SetHarvestHighlighted(false);
 if(AppliedZoneAppearance.IsNone())
 {
  OriginalZoneMaterials.Reset();for(int32 I=0;I<Mesh->GetNumMaterials();++I)OriginalZoneMaterials.Add(Mesh->GetMaterial(I));
  OriginalZoneScale=GetActorScale3D();OriginalZoneOverlay=Mesh->GetOverlayMaterial();
 }
 // First restore so transitions never compound scale or retain the previous state's gray material.
 for(int32 I=0;I<OriginalZoneMaterials.Num();++I)Mesh->SetMaterial(I,OriginalZoneMaterials[I]);
 Mesh->SetOverlayMaterial(OriginalZoneOverlay);SetActorScale3D(OriginalZoneScale);
 ZoneMaterial=nullptr;ZoneOverlay=nullptr;
 if(!Preset)
 {AppliedZoneAppearance=NAME_None;OriginalZoneMaterials.Reset();OriginalZoneOverlay=nullptr;}
 else
 {
  AppliedZoneAppearance=Preset->Id;bAppearanceInactive=Inactive;
  SetActorScale3D(OriginalZoneScale*(Inactive?.85f:Preset->ResourceScale));
  if(Inactive && InactiveBase)
  {
   ZoneMaterial=UMaterialInstanceDynamic::Create(InactiveBase,this);
   ZoneMaterial->SetVectorParameterValue(TEXT("Color"),FLinearColor(.19f,.18f,.16f));
   for(int32 I=0;I<Mesh->GetNumMaterials();++I)Mesh->SetMaterial(I,ZoneMaterial);
  }
  else if(HighlightBase)
  {
   ZoneOverlay=UMaterialInstanceDynamic::Create(HighlightBase,this);
   ZoneOverlay->SetVectorParameterValue(TEXT("Color"),Preset->ResourceColor*FMath::Max(.12f,Preset->Glow));
   Mesh->SetOverlayMaterial(ZoneOverlay);
  }
 }
 if(WasHighlighted && bZoneAvailable)SetHarvestHighlighted(true);
}
void AForestElementResource::Tick(float DT)
{
 Super::Tick(DT);
 if(bWaterShell && !bCollected)
 {
  const float Time=GetWorld()->GetTimeSeconds();
  // Small anchored drift, never a physics object that can float onto land.
  SetActorLocation(RestLocation+FVector(18.f*FMath::Sin(Time*.3f+FloatPhase),18.f*FMath::Cos(Time*.3f+FloatPhase),7.f*FMath::Sin(Time*1.5f+FloatPhase)));
 }
}

UForestElementRegions::UForestElementRegions()
{
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Fire(TEXT("/Game/Biomes/PNB_Enchanted_Forest/Models/Environment/SM_Env_Mushroom_01.SM_Env_Mushroom_01"));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Wind(TEXT("/Game/Biomes/PNB_Enchanted_Forest/Models/Environment/SM_Env_Mushroom_02.SM_Env_Mushroom_02"));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Shell1(TEXT("/Game/Corals2/Meshes/Shells_Shell1_SM.Shells_Shell1_SM"));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Shell2(TEXT("/Game/Corals2/Meshes/Shells_Shell2_SM.Shells_Shell2_SM"));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Shell3(TEXT("/Game/Corals2/Meshes/Shells_Shell3_SM.Shells_Shell3_SM"));
 FireMushroom=Fire.Object;WindMushroom=Wind.Object;
 if(Shell1.Object)ShellMeshes.Add(Shell1.Object);if(Shell2.Object)ShellMeshes.Add(Shell2.Object);if(Shell3.Object)ShellMeshes.Add(Shell3.Object);
}
bool UForestElementRegions::IsRegionalWorld() const
{
 return GetWorld() && GetWorld()->IsGameWorld() && GetWorld()->GetMapName().Contains(TEXT("DemoMap"));
}
bool UForestElementRegions::Discover()
{
 Regions.Reset();WaterSurfaces.Reset();
 const auto* Pawn=UGameplayStatics::GetPlayerPawn(GetWorld(),0);
 if(!Pawn || !Pawn->FindComponentByClass<UForestIsometricPlayer>())return false;
 for(TActorIterator<AActor> It(GetWorld());It;++It)
 {
  // These are the saved actor names behind the user's Fire/Water/Wind labels.
  // Explicit tags support future copies/renames without relying on editor labels.
  const FString Name=It->GetName();
  EAlchemyElement Element=EAlchemyElement::Fire;bool Anchor=false;
  if(It->ActorHasTag(TEXT("Forest.Region.Fire")) || Name==TEXT("SM_RuneStone_01_WIP4") || Name==TEXT("SM_RuneStone_01_Fire")){Element=EAlchemyElement::Fire;Anchor=true;}
  else if(It->ActorHasTag(TEXT("Forest.Region.Water")) || Name==TEXT("SM_RuneStone_01_WIP5") || Name==TEXT("SM_RuneStone_01_Water")){Element=EAlchemyElement::Water;Anchor=true;}
  else if(It->ActorHasTag(TEXT("Forest.Region.Wind")) || Name==TEXT("SM_RuneStone_01_WIP_2") || Name==TEXT("SM_RuneStone_01_Wind")){Element=EAlchemyElement::Wind;Anchor=true;}
  if(Anchor)
  {
   // The wind approach is the rune-stone mesh itself, not a Rock/Cliff asset.
   // Mark only its static mesh components; never make foliage/chains climbable.
   if(Element==EAlchemyElement::Wind)
   {
    TArray<UStaticMeshComponent*> Surfaces;It->GetComponents(Surfaces);
    for(auto* Surface:Surfaces)if(Surface && Surface->Mobility==EComponentMobility::Static)
    {Surface->ComponentTags.AddUnique(TEXT("ForestClimbable"));Surface->ComponentTags.AddUnique(TEXT("ForestClimbOverhang"));}
   }
   FForestElementRegion Region;Region.Element=Element;Region.Center=It->GetActorLocation();Region.AnchorId=Name;
   Region.Radius=FMath::Max(1000.f,Element==EAlchemyElement::Fire?FireRadius:Element==EAlchemyElement::Water?WaterRadius:WindRadius);
   Regions.Add(Region);
  }
  TArray<UStaticMeshComponent*> Meshes;It->GetComponents(Meshes);
  for(auto* Mesh:Meshes)if(IsWaterMesh(Mesh))WaterSurfaces.Add(Mesh);
 }
 Regions.Sort([](const auto& A,const auto& B){return A.AnchorId<B.AnchorId;});
 const bool HasFire=Regions.ContainsByPredicate([](const auto& R){return R.Element==EAlchemyElement::Fire;});
 const bool HasWater=Regions.ContainsByPredicate([](const auto& R){return R.Element==EAlchemyElement::Water;});
 const bool HasWind=Regions.ContainsByPredicate([](const auto& R){return R.Element==EAlchemyElement::Wind;});
 if(!HasFire || !HasWater || !HasWind || WaterSurfaces.IsEmpty() || !FireMushroom || !WindMushroom || ShellMeshes.IsEmpty())
 {
  if(!bReportedMissingDependencies)UE_LOG(LogTemp,Warning,TEXT("Element regions waiting: fire=%d water=%d wind=%d water surfaces=%d mushroom meshes=%d shell meshes=%d"),HasFire,HasWater,HasWind,WaterSurfaces.Num(),FireMushroom && WindMushroom,ShellMeshes.Num());
  bReportedMissingDependencies=true;return false;
 }
 HarvestSave=Cast<UForestRegionHarvestSave>(UGameplayStatics::LoadGameFromSlot(HarvestSlot,0));
 if(!HarvestSave)HarvestSave=NewObject<UForestRegionHarvestSave>(this);
 UE_LOG(LogTemp,Log,TEXT("Element regions ready: %d anchors, %d swim surfaces, fire %.0fm / wind %.0fm / water %.0fm"),Regions.Num(),WaterSurfaces.Num(),FireRadius/100,WindRadius/100,WaterRadius/100);
 return true;
}
bool UForestElementRegions::IsInsideRegion(const FForestElementRegion& Region,const FVector& Point) const
{
 const float Distance=FVector::DistSquared2D(Region.Center,Point);
 if(Distance>FMath::Square(Region.Radius-100.f))return false;
 // Partition overlapping circles by distance to their stone, with stable ties.
 for(const auto& Other:Regions)
  if(Other.AnchorId!=Region.AnchorId && FVector::DistSquared2D(Other.Center,Point)<=FMath::Square(Other.Radius))
  {
   const float OtherDistance=FVector::DistSquared2D(Other.Center,Point);
   if(OtherDistance<Distance || (FMath::IsNearlyEqual(OtherDistance,Distance) && Other.AnchorId<Region.AnchorId))return false;
  }
 return true;
}
bool UForestElementRegions::FindWater(const FVector& Point,FVector& Surface) const
{
 bool Found=false;float Top=-MAX_flt;
 FCollisionQueryParams Query(SCENE_QUERY_STAT(RegionalWater),true);
 for(const auto& Weak:WaterSurfaces)if(auto* Mesh=Weak.Get())
 {
  const FBox Box=Mesh->Bounds.GetBox();
  if(Point.X<Box.Min.X || Point.X>Box.Max.X || Point.Y<Box.Min.Y || Point.Y>Box.Max.Y)continue;
  FHitResult Hit;
  if(Mesh->LineTraceComponent(Hit,FVector(Point.X,Point.Y,Box.Max.Z+500),FVector(Point.X,Point.Y,Box.Min.Z-500),Query) && Hit.ImpactNormal.Z>.25f && Hit.ImpactPoint.Z>Top)
  {Top=Hit.ImpactPoint.Z;Surface=Hit.ImpactPoint;Found=true;}
 }
 return Found;
}
bool UForestElementRegions::FindGround(const FVector& Point,FHitResult& Ground) const
{
 FCollisionQueryParams Query(SCENE_QUERY_STAT(RegionalGround),true);
 for(const auto& Weak:WaterSurfaces)if(auto* Mesh=Weak.Get())Query.AddIgnoredComponent(Mesh);
 FCollisionObjectQueryParams Objects;Objects.AddObjectTypesToQuery(ECC_WorldStatic);
 TArray<FHitResult> Hits;
 GetWorld()->LineTraceMultiByObjectType(Hits,Point+FVector(0,0,6000),Point-FVector(0,0,12000),Objects,Query);
 for(const auto& Hit:Hits)if(IsNaturalGround(Hit)){Ground=Hit;return true;}
 return false;
}
void UForestElementRegions::BeginRegrowth(int32 Day)
{
 for(const auto& Resource:Resources)if(IsValid(Resource))Resource->Destroy();Resources.Reset();
 CurrentDay=Day;if(auto* Zones=GetWorld()->GetSubsystem<UForestWorldZones>())Zones->BeginResourceDay(Day);BuildRegion=0;BuildAttempt=0;BuildCount=0;
 if(!Regions.IsEmpty())PlacementRandom.Initialize(static_cast<int32>(FCrc::StrCrc32(*Regions[0].AnchorId)));
}
void UForestElementRegions::GrowCandidates(int32 Budget)
{
 while(Budget-->0 && Regions.IsValidIndex(BuildRegion))
 {
  const auto& Region=Regions[BuildRegion];const bool Water=Region.Element==EAlchemyElement::Water;
  const int32 Goal=FMath::Clamp(Water?ShellsPerRegion:MushroomsPerRegion,1,Water?100:150);
  if(BuildCount>=Goal || BuildAttempt>=Goal*150)
  {
   UE_LOG(LogTemp,Log,TEXT("Element region %s: placed %d/%d, attempts=%d"),*Region.AnchorId,BuildCount,Goal,BuildAttempt);
   ++BuildRegion;BuildCount=0;BuildAttempt=0;
   if(Regions.IsValidIndex(BuildRegion))PlacementRandom.Initialize(static_cast<int32>(FCrc::StrCrc32(*Regions[BuildRegion].AnchorId)));
   continue;
  }
  ++BuildAttempt;
  const float Angle=PlacementRandom.FRand()*2*PI;
  const float Radius=FMath::Sqrt(PlacementRandom.FRandRange(.03f,.95f))*Region.Radius;
  FVector Point=Region.Center+FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius,0);
  if(!IsInsideRegion(Region,Point))continue;
  if(Resources.ContainsByPredicate([&](const auto& R){return IsValid(R) && FVector::DistSquared2D(R->RestLocation,Point)<FMath::Square(Water?300.f:230.f);}))continue;
  FVector WaterPoint;FHitResult Ground;const bool HasWater=FindWater(Point,WaterPoint);
  if(!FindGround(Point,Ground))continue;
  if(Water)
  {
   if(!HasWater || WaterPoint.Z-Ground.ImpactPoint.Z<180.f)continue;
   // Reject riverbanks: four neighbouring samples must also be open water.
   bool OpenWater=true;
   for(const FVector Offset:{FVector(300,0,0),FVector(-300,0,0),FVector(0,300,0),FVector(0,-300,0)})
   {FVector Edge;if(!FindWater(Point+Offset,Edge) || FMath::Abs(Edge.Z-WaterPoint.Z)>40.f){OpenWater=false;break;}}
   if(!OpenWater)continue;Point=WaterPoint+FVector(0,0,10);
  }
  else
  {
   if(HasWater && WaterPoint.Z>=Ground.ImpactPoint.Z-10.f)continue;
   Point=Ground.ImpactPoint+FVector(0,0,2);
   FCollisionQueryParams Clear(SCENE_QUERY_STAT(RegionalClearance),false);
   if(GetWorld()->OverlapBlockingTestByChannel(Point+FVector(0,0,100),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(40,96),Clear))continue;
  }
  UStaticMesh* Asset=Water?ShellMeshes[BuildCount%ShellMeshes.Num()].Get():Region.Element==EAlchemyElement::Fire?FireMushroom.Get():WindMushroom.Get();
  FActorSpawnParameters Spawn;Spawn.ObjectFlags|=RF_Transient;Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
  auto* Resource=GetWorld()->SpawnActor<AForestElementResource>(Point,FRotator(0,PlacementRandom.FRand()*360.f,0),Spawn);
  if(!Resource)continue;
  Resource->Mesh->SetStaticMesh(Asset);Resource->Element=Region.Element;Resource->bWaterShell=Water;
  const FBoxSphereBounds Bounds=Asset->GetBounds();
  const float Size=Water?FMath::Max(Bounds.BoxExtent.X,Bounds.BoxExtent.Y)*2.f:Bounds.BoxExtent.Z*2.f;
  const float Scale=FMath::Clamp((Water?75.f:85.f)/FMath::Max(Size,1.f),.02f,10.f);
  Resource->SetActorScale3D(FVector(Scale));
  Point.Z-=(Bounds.Origin.Z-Bounds.BoxExtent.Z)*Scale;
  Resource->SetActorLocation(Point);Resource->RestLocation=Point;
  Resource->FloatPhase=PlacementRandom.FRand()*2*PI;
  Resource->ResourceId=Region.AnchorId+FString::Printf(TEXT(":%d"),BuildCount++);
  Resource->bCollected=HarvestSave && HarvestSave->Collected.Contains(CollectionKey(Resource));
  Resource->SetActorHiddenInGame(Resource->bCollected);Resource->SetActorEnableCollision(!Resource->bCollected);
  Resource->SetActorTickEnabled(Water && !Resource->bCollected);
  Resources.Add(Resource);
  if(auto* Zones=GetWorld()->GetSubsystem<UForestWorldZones>())Zones->RegisterResource(Resource,Region.AnchorId,BuildCount-1);
 }
}
AForestElementResource* UForestElementRegions::Nearest(const ACharacter* Character,float Range) const
{
 if(!Character)return nullptr;
 AForestElementResource* Best=nullptr;float Distance=FMath::Square(Range);
 for(const auto& Resource:Resources)if(IsValid(Resource) && !Resource->bCollected && Resource->bZoneAvailable)
 {
  const float D=FVector::DistSquared(Character->GetActorLocation(),Resource->Mesh->Bounds.GetBox().GetClosestPointTo(Character->GetActorLocation()));
  if(D>=Distance)continue;
  FCollisionQueryParams Query(SCENE_QUERY_STAT(RegionalHarvestSight),false,Character);Query.AddIgnoredActor(Resource);
  for(const auto& Weak:WaterSurfaces)if(auto* Mesh=Weak.Get())Query.AddIgnoredComponent(Mesh);
  FHitResult Hit;
  if(GetWorld()->LineTraceSingleByChannel(Hit,Character->GetActorLocation()+FVector(0,0,50),Resource->Mesh->Bounds.Origin,ECC_Visibility,Query))continue;
  Best=Resource;Distance=D;
 }
 return Best;
}
FString UForestElementRegions::CollectionKey(const AForestElementResource* Resource) const{return FString::Printf(TEXT("%d:%s"),CurrentDay,*Resource->ResourceId);}
bool UForestElementRegions::CanHarvestResource(const ACharacter* Character,bool Swimming,const AForestElementResource* Resource) const
{
 if(!bInitialized || !HarvestSave || !Character || !IsValid(Resource) || Resource->bCollected || !Resource->bZoneAvailable || !Resources.Contains(Resource) || (Resource->bWaterShell && !Swimming))return false;
 if(FVector::DistSquared(Character->GetActorLocation(),Resource->Mesh->Bounds.GetBox().GetClosestPointTo(Character->GetActorLocation()))>FMath::Square(230.f))return false;
 FCollisionQueryParams Query(SCENE_QUERY_STAT(RegionalHarvestTarget),false,Character);Query.AddIgnoredActor(Resource);
 for(const auto& Weak:WaterSurfaces)if(auto* Mesh=Weak.Get())Query.AddIgnoredComponent(Mesh);
 FHitResult Hit;return !GetWorld()->LineTraceSingleByChannel(Hit,Character->GetActorLocation()+FVector(0,0,50),Resource->Mesh->Bounds.Origin,ECC_Visibility,Query);
}
bool UForestElementRegions::TryHarvest(ACharacter* Character,bool Swimming)
{
 auto* Resource=Nearest(Character,230.f);
 if(!bInitialized || !HarvestSave || !Resource || (Resource->bWaterShell && !Swimming))return false;
 auto* Actions=Character?Character->FindComponentByClass<UForestIsometricPlayer>():nullptr;
 return Actions && Actions->BeginGather(Resource);
}
bool UForestElementRegions::CollectResource(ACharacter* Character,bool Swimming,AForestElementResource* Resource)
{
 // Commit the original target only, after its animation. A moved/respawned target cancels.
 if(!CanHarvestResource(Character,Swimming,Resource))return false;
 if(!bInitialized || !HarvestSave || !Resource || (Resource->bWaterShell && !Swimming))return false;
 const FString Key=CollectionKey(Resource);
 if(HarvestSave->Collected.Contains(Key))return false;
 HarvestSave->Collected.Add(Key);
 if(!UGameplayStatics::SaveGameToSlot(HarvestSave,HarvestSlot,0)){HarvestSave->Collected.Remove(Key);return false;}
 auto* Harvest=GetWorld()->GetSubsystem<UForestHarvestMagic>();
 if(!Harvest || !Harvest->GrantRegionalMaterial(Resource->Element,MaterialPipValue,Resource->Mesh))
 {HarvestSave->Collected.Remove(Key);UGameplayStatics::SaveGameToSlot(HarvestSave,HarvestSlot,0);return false;}
 Resource->bCollected=true;Resource->SetActorHiddenInGame(true);Resource->SetActorEnableCollision(false);Resource->SetActorTickEnabled(false);
 return true;
}
FString UForestElementRegions::GetHarvestHint(const ACharacter* Character,bool Swimming) const
{
 if(!bInitialized)return TEXT("元素产区正在准备 · 请稍候");
 if(const auto* Resource=Nearest(Character,300.f))
 {
  if(Resource->bWaterShell && !Swimming)return TEXT("水灵贝壳 · 需要游泳靠近后按 E 采集");
  return FString::Printf(TEXT("E 采集%s · %s元素 %d"),Resource->bWaterShell?TEXT("漂浮贝壳"):Resource->Element==EAlchemyElement::Fire?TEXT("火纹蘑菇"):TEXT("风息蘑菇"),*UForestHarvestMagic::ElementText(Resource->Element),FMath::Clamp(MaterialPipValue,1,9));
 }
 if(Character)for(const auto& Region:Regions)if(FVector::DistSquared2D(Character->GetActorLocation(),Region.Center)<=FMath::Square(Region.Radius))
  return FString::Printf(TEXT("%s元素产区 · 半径 %.0f 米 · %s"),*UForestHarvestMagic::ElementText(Region.Element),Region.Radius/100,Region.Element==EAlchemyElement::Water?TEXT("游泳寻找水面漂浮贝壳"):TEXT("寻找符文石周围的元素蘑菇"));
 return TEXT("前往火 / 风符文石采集蘑菇，或到水符文石水域游泳采集贝壳");
}
void UForestElementRegions::Tick(float DT)
{
 if(!IsRegionalWorld())return;
 DiscoverDelay-=DT;
 if(!bInitialized)
 {
  if(DiscoverDelay>0)return;DiscoverDelay=3.f;bInitialized=Discover();
  if(!bInitialized)return;
  DiscoverDelay=0;
 }
 if(DiscoverDelay<=0)
 {
  DiscoverDelay=3.f;
  // Suppress the original scattered mushrooms, including late streamed actors.
  for(TActorIterator<AActor> It(GetWorld());It;++It)if(!It->IsA<AForestElementResource>() && !It->ActorHasTag(TEXT("ForestHarvestVisual")))
  {
   TArray<UStaticMeshComponent*> Meshes;It->GetComponents(Meshes);
   for(auto* Mesh:Meshes)if(Mesh->GetStaticMesh() && Mesh->GetStaticMesh()->GetName().StartsWith(TEXT("SM_Env_Mushroom")))
   {
    if(!SuppressedMeshes.Contains(Mesh))
    {FOriginalMesh Row;Row.Mesh=Mesh;Row.bVisible=Mesh->IsVisible();Row.Collision=Mesh->GetCollisionEnabled();OriginalMushrooms.Add(Row);SuppressedMeshes.Add(Mesh);}
    Mesh->SetVisibility(false,false);Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
   }
  }
 }
 auto* Daily=GetWorld()->GetGameInstance()->GetSubsystem<UDailyMushroomSubsystem>();const int32 Day=Daily?Daily->GetCurrentDay():1;
 if(Day!=CurrentDay)BeginRegrowth(Day);
 GrowCandidates(12);
}
TStatId UForestElementRegions::GetStatId() const{RETURN_QUICK_DECLARE_CYCLE_STAT(UForestElementRegions,STATGROUP_Tickables);}
void UForestElementRegions::Deinitialize()
{
 for(const auto& Resource:Resources)if(IsValid(Resource))Resource->Destroy();Resources.Reset();
 for(const auto& Row:OriginalMushrooms)if(auto* Mesh=Row.Mesh.Get()){Mesh->SetVisibility(Row.bVisible,false);Mesh->SetCollisionEnabled(Row.Collision);}
 OriginalMushrooms.Reset();SuppressedMeshes.Reset();Super::Deinitialize();
}
