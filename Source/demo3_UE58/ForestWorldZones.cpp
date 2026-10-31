#include "ForestWorldZones.h"
#include "ForestZonePolicy.h"
#include "ForestElementRegions.h"
#include "DailyMushroomSubsystem.h"
#include "EngineUtils.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "JsonObjectConverter.h"
#include "Components/InstancedStaticMeshComponent.h"

namespace
{
 const TCHAR* ZoneSlot=TEXT("ForestWorldZones_v1");
 FName ChildId(const FString& Root,int Part){return FName(*(Root+(Part==0?TEXT("_entry_safe"):Part==1?TEXT("_gather_a"):TEXT("_gather_b"))));}
 bool InPolygon(FVector2D P,const TArray<FVector2D>& Points)
 {
  bool Inside=false;
  for(int32 I=0,J=Points.Num()-1;I<Points.Num();J=I++)
  {
   const auto A=Points[I],B=Points[J];
   if((A.Y>P.Y)!=(B.Y>P.Y) && P.X<(B.X-A.X)*(P.Y-A.Y)/(B.Y-A.Y)+A.X)Inside=!Inside;
  }
  return Inside;
 }
 bool SameState(const FForestZoneState& A,const FForestZoneState& B)
 {
  return A.StateId==B.StateId && A.ResourceAvailabilityScale==B.ResourceAvailabilityScale && A.MaxAvailableResources==B.MaxAvailableResources &&
   A.bHarvestEnabled==B.bHarvestEnabled && A.bHideUnavailableResources==B.bHideUnavailableResources && A.bHideTaggedActors==B.bHideTaggedActors &&
   A.bDisableTaggedActorCollision==B.bDisableTaggedActorCollision && A.NpcStateId==B.NpcStateId && A.MapStateId==B.MapStateId && A.SourceReceiptId==B.SourceReceiptId;
 }

 bool SafeActor(AActor* Actor)
 {
  if(!IsValid(Actor) || Actor->IsA<APawn>() || Actor->IsA<AController>() || Actor->IsA<AForestElementResource>() || Actor->ActorHasTag(TEXT("Forest.ZoneProtected")))return false;
  TArray<UInstancedStaticMeshComponent*> Instances;Actor->GetComponents(Instances);if(!Instances.IsEmpty())return false;
  const FString Type=Actor->GetClass()->GetName();
  return !Type.Contains(TEXT("Landscape")) && !Type.Contains(TEXT("NavMesh")) && !Type.Contains(TEXT("PlayerStart"));
 }
}
void UForestWorldZones::Initialize(FSubsystemCollectionBase& Collection)
{Super::Initialize(Collection);}
bool UForestWorldZones::ActiveWorld() const
{return GetWorld() && GetWorld()->IsGameWorld() && GetWorld()->GetMapName().Contains(TEXT("DemoMap"));}
int32 UForestWorldZones::CurrentDay() const
{
 if(auto* GI=GetWorld()?GetWorld()->GetGameInstance():nullptr)
  if(auto* Daily=GI->GetSubsystem<UDailyMushroomSubsystem>())return Daily->GetCurrentDay();
 return 1;
}
bool UForestWorldZones::ReloadLayout()
{
 FForestZoneLayout NewLayout;FString Text;
 const FString Path=FPaths::ProjectConfigDir()/TEXT("ForestZones.json");
 if(FFileHelper::LoadFileToString(Text,*Path))
 {
  if(!FJsonObjectConverter::JsonObjectStringToUStruct(Text,&NewLayout,0,0) || NewLayout.SchemaVersion!=1 ||
    !FMath::IsFinite(NewLayout.HomeProtectionRadius) || NewLayout.HomeProtectionRadius<=0 ||
    !FMath::IsFinite(NewLayout.EntryHalfAngleDegrees) || NewLayout.EntryHalfAngleDegrees<10 || NewLayout.EntryHalfAngleDegrees>85 || NewLayout.HomeCenter.ContainsNaN())
  {UE_LOG(LogTemp,Warning,TEXT("Invalid ForestZones.json; keeping previous zone layout."));return false;}
  for(const auto& Override:NewLayout.PolygonOverrides)
  {
   if(Override.ZoneId.IsNone() || Override.Points.Num()<3)return false;
   for(auto P:Override.Points)if(P.ContainsNaN())return false;
  }
 }
 NewLayout.ProtectedResourcesPerProductionRegion=FMath::Max(1,NewLayout.ProtectedResourcesPerProductionRegion);
 Layout=MoveTemp(NewLayout);
 if(!Save){Save=Cast<UForestWorldZoneSave>(UGameplayStatics::LoadGameFromSlot(ZoneSlot,0));if(!Save)Save=NewObject<UForestWorldZoneSave>(this);}
 bLoaded=true;DiscoverZones();ReassignResources();
 ApplyControls();return true;
}
bool UForestWorldZones::DiscoverZones()
{
 if(!ActiveWorld())return false;
 TArray<AActor*> Anchors;
 for(TActorIterator<AActor> It(GetWorld());It;++It)
 {
  const auto Name=It->GetName();
  if(It->ActorHasTag(TEXT("Forest.Region.Fire")) || It->ActorHasTag(TEXT("Forest.Region.Wind")) || It->ActorHasTag(TEXT("Forest.Region.Water")) ||
   Name==TEXT("SM_RuneStone_01_WIP4") || Name==TEXT("SM_RuneStone_01_Fire") || Name==TEXT("SM_RuneStone_01_WIP5") || Name==TEXT("SM_RuneStone_01_Water") || Name==TEXT("SM_RuneStone_01_WIP_2") || Name==TEXT("SM_RuneStone_01_Wind"))Anchors.Add(*It);
 }
 if(Anchors.IsEmpty())return false;
 Anchors.Sort([](const AActor& A,const AActor& B){return A.GetName()<B.GetName();});
 Zones.Reset();FForestWorldZone Home;Home.ZoneId=TEXT("home_safe");Home.Label=TEXT("小屋保护区");Home.Center=Layout.HomeCenter;Home.bProtected=true;Zones.Add(Home);
 for(auto* Anchor:Anchors)
 {
  const auto N=Anchor->GetName();
  const FString Root=Anchor->ActorHasTag(TEXT("Forest.Region.Fire")) || N==TEXT("SM_RuneStone_01_WIP4") || N==TEXT("SM_RuneStone_01_Fire")?TEXT("fire"):
   Anchor->ActorHasTag(TEXT("Forest.Region.Water")) || N==TEXT("SM_RuneStone_01_WIP5") || N==TEXT("SM_RuneStone_01_Water")?TEXT("water"):TEXT("wind");
  if(Zones.ContainsByPredicate([&](const auto& Z){return Z.ZoneId==ChildId(Root,0);}))continue;
  FVector2D Direction(Layout.HomeCenter-Anchor->GetActorLocation());Direction.Normalize();if(Direction.IsNearlyZero())Direction=FVector2D(1,0);
  for(int32 I=0;I<3;++I)
  {
   FForestWorldZone Z;Z.ZoneId=ChildId(Root,I);Z.ProductionAnchorId=Anchor->GetName();Z.Center=Anchor->GetActorLocation();Z.EntryDirection=Direction;Z.bProtected=I==0;
   Z.Label=(Root==TEXT("fire")?TEXT("火区"):Root==TEXT("wind")?TEXT("风区"):TEXT("水区"))+FString(I==0?TEXT("入口保底区"):I==1?TEXT("采集区 A"):TEXT("采集区 B"));Zones.Add(Z);
  }
 }
 return Zones.Num()>=4;
}
FName UForestWorldZones::Locate(FVector P,const FString& AnchorId) const
{
 if(Zones.IsEmpty())return NAME_None;
 if(FVector::DistSquared2D(P,Layout.HomeCenter)<=FMath::Square(Layout.HomeProtectionRadius))return TEXT("home_safe");
 for(const auto& Polygon:Layout.PolygonOverrides)
 {
  const auto* Zone=Zones.FindByPredicate([&](const auto& Z){return Z.ZoneId==Polygon.ZoneId && (AnchorId.IsEmpty() || Z.ProductionAnchorId==AnchorId);});
  if(Zone && InPolygon(FVector2D(P),Polygon.Points))return Zone->ZoneId;
 }
 const FForestWorldZone* Best=nullptr;double Distance=TNumericLimits<double>::Max();
 for(const auto& Z:Zones)if(Z.bProtected && !Z.ProductionAnchorId.IsEmpty() && (AnchorId.IsEmpty() || Z.ProductionAnchorId==AnchorId))
 {
  const double D=FVector::DistSquared2D(P,Z.Center);
  if(D<Distance || (D==Distance && Best && Z.ZoneId.LexicalLess(Best->ZoneId))){Best=&Z;Distance=D;}
 }
 if(!Best)return NAME_None;
 const FVector2D Delta(P-Best->Center),F=Best->EntryDirection;
 const auto Part=ForestZonePolicy::Classify(FVector2D::DotProduct(Delta,F),F.X*Delta.Y-F.Y*Delta.X,FMath::Cos(FMath::DegreesToRadians(Layout.EntryHalfAngleDegrees)));
 FString Root=Best->ZoneId.ToString();Root.RemoveFromEnd(TEXT("_entry_safe"));
 return ChildId(Root,Part==ForestZonePolicy::Part::Entry?0:Part==ForestZonePolicy::Part::A?1:2);
}
FName UForestWorldZones::GetZoneAtLocation(FVector P) const{return Locate(P);}
FName UForestWorldZones::GetActorZone(AActor* Actor) const{return IsValid(Actor)?Locate(Actor->GetActorLocation()):NAME_None;}
FForestZoneState UForestWorldZones::GetZoneState(FName Id,int32 Day) const
{
 FForestZoneState Result;
 const auto* Zone=Zones.FindByPredicate([&](const auto& Z){return Z.ZoneId==Id;});if(!Zone)return Result;
 if(Day<=0)Day=CurrentDay();
 for(const auto& Row:Layout.Controls)if(Row.bEnabled && Row.ZoneId==Id && Row.TargetDay==Day)Result=Row.State;
 if(Save)for(const auto& Row:Save->Controls)if(Row.bEnabled && Row.ZoneId==Id && Row.TargetDay==Day)Result=Row.State;
 // Defensive normalization also covers hand-edited config or old save values.
 Result.ResourceAvailabilityScale=FMath::IsFinite(Result.ResourceAvailabilityScale)?FMath::Clamp(Result.ResourceAvailabilityScale,0.f,1.f):1.f;
 Result.MaxAvailableResources=FMath::Max(-1,Result.MaxAvailableResources);
 if(Zone->bProtected){Result.ResourceAvailabilityScale=1;Result.MaxAvailableResources=-1;Result.bHarvestEnabled=true;Result.bHideTaggedActors=false;Result.bDisableTaggedActorCollision=false;}
 return Result;
}
bool UForestWorldZones::SetZoneState(FName Id,const FForestZoneState& State,int32 Day)
{
 if(!ActiveWorld())return false;if(!bLoaded && !ReloadLayout())return false;
 const auto* Z=Zones.FindByPredicate([&](const auto& X){return X.ZoneId==Id;});
 if(!Z || !Save || !FMath::IsFinite(State.ResourceAvailabilityScale) || Day<0)return false;
 if(Z->bProtected && (State.ResourceAvailabilityScale<1 || State.MaxAvailableResources>=0 || !State.bHarvestEnabled || State.bHideTaggedActors || State.bDisableTaggedActorCollision))return false;
 if(Day==0)Day=CurrentDay();if(Day<CurrentDay())return false;
 FForestZoneState Normalized=State;Normalized.ResourceAvailabilityScale=FMath::Clamp(State.ResourceAvailabilityScale,0.f,1.f);Normalized.MaxAvailableResources=FMath::Max(-1,State.MaxAvailableResources);
 if(Save->Controls.ContainsByPredicate([&](const auto& R){return R.ZoneId==Id && R.TargetDay==Day && R.bEnabled && SameState(R.State,Normalized);}))return true;
 const auto Before=Save->Controls;Save->Controls.RemoveAll([&](const auto& R){return R.ZoneId==Id && R.TargetDay==Day;});
 FForestZoneControlRow Row;Row.ZoneId=Id;Row.TargetDay=Day;Row.bEnabled=true;Row.State=State;Row.State.ResourceAvailabilityScale=FMath::Clamp(State.ResourceAvailabilityScale,0.f,1.f);Row.State.MaxAvailableResources=FMath::Max(-1,State.MaxAvailableResources);Save->Controls.Add(Row);
 if(!UGameplayStatics::SaveGameToSlot(Save,ZoneSlot,0)){Save->Controls=Before;return false;}
 if(Day==CurrentDay()){ApplyControls();OnZoneStateChanged.Broadcast(Id,GetZoneState(Id));}return true;
}
bool UForestWorldZones::ResetZone(FName Id,int32 Day){return SetZoneState(Id,FForestZoneState(),Day);}
void UForestWorldZones::BeginResourceDay(int32 Day)
{ResourceDay=Day;Resources.Reset();ZoneOrdinals.Reset();}
void UForestWorldZones::RegisterResource(AForestElementResource* R,const FString& AnchorId,int32 Ordinal)
{
 if(!IsValid(R))return;if(!bLoaded)ReloadLayout();if(Zones.Num()<10)DiscoverZones();
 R->ProductionAnchorId=AnchorId;R->ProductionOrdinal=Ordinal;R->ZoneId=Locate(R->RestLocation,AnchorId);
 R->ZoneOrdinal=ZoneOrdinals.FindOrAdd(R->ZoneId)++;R->bZoneReserve=Ordinal<Layout.ProtectedResourcesPerProductionRegion;Resources.Add(R);ApplyResource(R);
}
void UForestWorldZones::ReassignResources()
{
 ZoneOrdinals.Reset();
 for(const auto& Weak:Resources)if(auto* R=Weak.Get())
 {
  R->ZoneId=Locate(R->RestLocation,R->ProductionAnchorId);R->ZoneOrdinal=ZoneOrdinals.FindOrAdd(R->ZoneId)++;
  R->bZoneReserve=R->ProductionOrdinal<Layout.ProtectedResourcesPerProductionRegion;
 }
}
void UForestWorldZones::ApplyResource(AForestElementResource* R)
{
 if(!IsValid(R) || ResourceDay!=CurrentDay())return;
 const auto* Z=Zones.FindByPredicate([&](const auto& X){return X.ZoneId==R->ZoneId;});const auto State=GetZoneState(R->ZoneId);
 const bool Available=ForestZonePolicy::Available(R->bZoneReserve || (Z && Z->bProtected),State.bHarvestEnabled,State.ResourceAvailabilityScale,FCrc::StrCrc32(*R->ResourceId),R->ZoneOrdinal,State.MaxAvailableResources);
 R->ApplyZoneAvailability(Available,State.bHideUnavailableResources);
}
bool UForestWorldZones::HasActorControls() const
{
 for(const auto& Z:Zones){const auto S=GetZoneState(Z.ZoneId);if(S.bHideTaggedActors || S.bDisableTaggedActorCollision)return true;}return false;
}
void UForestWorldZones::RestoreActors()
{
 for(auto& Pair:ActorOriginals)if(auto* A=Pair.Value.Actor.Get()){A->SetActorHiddenInGame(Pair.Value.Hidden);A->SetActorEnableCollision(Pair.Value.Collision);}
 ActorOriginals.Reset();
}
void UForestWorldZones::ApplyControls()
{
 Resources.RemoveAll([](const auto& R){return !R.IsValid();});for(const auto& R:Resources)ApplyResource(R.Get());
 // No tagged actor scan or setter calls on the neutral path.
 if(!HasActorControls()){if(!ActorOriginals.IsEmpty())RestoreActors();return;}
 TSet<TWeakObjectPtr<AActor>> Active;
 for(TActorIterator<AActor> It(GetWorld());It;++It)if(SafeActor(*It) && It->ActorHasTag(TEXT("Forest.ZoneControlled")))
 {
  const auto State=GetZoneState(GetActorZone(*It));if(!State.bHideTaggedActors && !State.bDisableTaggedActorCollision)continue;
  const TWeakObjectPtr<AActor> Key(*It);Active.Add(Key);
  if(!ActorOriginals.Contains(Key)){FActorOriginal O;O.Actor=*It;O.Hidden=It->IsHidden();O.Collision=It->GetActorEnableCollision();ActorOriginals.Add(Key,O);}
  const auto& O=ActorOriginals[Key];It->SetActorHiddenInGame(O.Hidden || State.bHideTaggedActors);It->SetActorEnableCollision(O.Collision && !State.bDisableTaggedActorCollision);
 }
 for(auto It=ActorOriginals.CreateIterator();It;++It)if(!Active.Contains(It.Key()))
 {if(auto* A=It.Value().Actor.Get()){A->SetActorHiddenInGame(It.Value().Hidden);A->SetActorEnableCollision(It.Value().Collision);}It.RemoveCurrent();}
}
bool UForestWorldZones::SetActorZoneControlled(AActor* Actor,bool Controlled)
{
 if(!ActiveWorld() || !SafeActor(Actor))return false;
 if(Controlled)
 {
  const auto Id=GetActorZone(Actor);const auto* Zone=Zones.FindByPredicate([&](const auto& Z){return Z.ZoneId==Id;});if(!Zone || Zone->bProtected)return false;
  Actor->Tags.AddUnique(TEXT("Forest.ZoneControlled"));
 }
 else Actor->Tags.Remove(TEXT("Forest.ZoneControlled"));
 ApplyControls();return true;
}
void UForestWorldZones::Tick(float DT)
{
 if(!ActiveWorld())return;
 if(!bLoaded){ReloadLayout();return;}
 const int32 Day=CurrentDay();if(Day!=LastDay){LastDay=Day;ApplyControls();const auto Snapshot=Zones;for(const auto& Z:Snapshot)OnZoneStateChanged.Broadcast(Z.ZoneId,GetZoneState(Z.ZoneId));}
 ScanTime-=DT;if(ScanTime>0)return;ScanTime=2.f;
 if(Zones.Num()<10){const int32 Before=Zones.Num();if(DiscoverZones() && Zones.Num()!=Before){ReassignResources();ApplyControls();}}
 else if(HasActorControls())ApplyControls();
}
void UForestWorldZones::Deinitialize(){RestoreActors();Resources.Reset();Super::Deinitialize();}
TStatId UForestWorldZones::GetStatId() const{RETURN_QUICK_DECLARE_CYCLE_STAT(UForestWorldZones,STATGROUP_Tickables);}
