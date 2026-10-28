#include "ForestMinimap.h"
#include "ForestUITheme.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/SLeafWidget.h"

namespace
{
 const FLinearColor Gold(.84f,.69f,.37f),Teal(.18f,.95f,.79f),Cream(.96f,.91f,.74f);

 class SForestMinimap : public SLeafWidget
 {
 public:
  SLATE_BEGIN_ARGS(SForestMinimap){} SLATE_ARGUMENT(UForestMinimapWidget*,Owner) SLATE_END_ARGS()
  void Construct(const FArguments& Args){Owner=Args._Owner;SetCanTick(false);}
  virtual FVector2D ComputeDesiredSize(float) const override{return FVector2D(800,600);}
  virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool) const override
  {
   const auto* Map=Owner.Get();if(!Map)return Layer;
   const FVector2D Screen=G.GetLocalSize();
   const double Side=Map->bOverview?FMath::Max(120.,FMath::Min3(Screen.Y-140.,Screen.X-100.,680.)):FMath::Clamp(Screen.Y*.32,150.,260.);
   const FVector2D PanelSize(Side+24,Side+108);
   const FVector2D Panel=Map->bOverview?(Screen-PanelSize)*.5:FVector2D(Screen.X-PanelSize.X-22,22);
   const FVector2D Origin=Panel+FVector2D(12,38);
   const FSlateBrush* White=FCoreStyle::Get().GetBrush("WhiteBrush");
   auto Box=[&](FVector2D P,FVector2D Size,FLinearColor Color,int32 L){FSlateDrawElement::MakeBox(Out,L,G.ToPaintGeometry(Size,FSlateLayoutTransform(P)),White,ESlateDrawEffect::None,Color);};
   auto Text=[&](const FString& Value,FVector2D P,int32 Size,FLinearColor Color,int32 L){FSlateDrawElement::MakeText(Out,L,G.ToPaintGeometry(FVector2D(1,1),FSlateLayoutTransform(P)),Value,ForestTheme::Font(Size,true),ESlateDrawEffect::None,Color);};
   auto Line=[&](TArray<FVector2f> Points,FLinearColor Color,float Thickness,int32 L){FSlateDrawElement::MakeLines(Out,L,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,Color,true,Thickness);};
   if(Map->bOverview)Box(FVector2D::ZeroVector,Screen,FLinearColor(.015f,.025f,.025f,.65f),Layer);
   Box(Panel+FVector2D(4,5),PanelSize,FLinearColor(0,0,0,.4f),Layer+1);
   Box(Panel,PanelSize,Gold,Layer+2);
   Box(Panel+FVector2D(2,2),PanelSize-FVector2D(4,4),FLinearColor(.027f,.065f,.052f,.97f),Layer+3);
   Text(Map->bOverview?TEXT("梦境森林 · 世界地图"):TEXT("林间方位"),Panel+FVector2D(13,9),12,Cream,Layer+4);
   Text(TEXT("N  北 ↑"),Panel+FVector2D(Side-50,11),10,Gold,Layer+4);
   Box(Origin-FVector2D(1,1),FVector2D(Side+2,Side+2),Gold,Layer+4);
   Box(Origin,FVector2D(Side,Side),FLinearColor(.07f,.13f,.1f),Layer+5);
   const double Width=Map->bOverview?Map->WorldWidth:Map->LocalWidth;
   const FVector2D Center=Map->GetViewCenter();
   const FVector2D UV=Map->WorldToMapUV(FVector(Center.X,Center.Y,0));
   const double HalfUV=Width/(Map->WorldWidth*2.);
   if(Map->MapTexture)
   {
    // Keep the brush owned by this widget: Slate retains its resource until render.
    Image.SetResourceObject(Map->MapTexture);Image.ImageSize=FVector2D(2048,2048);Image.DrawAs=ESlateBrushDrawType::Image;
    Image.SetUVRegion(FBox2d(UV-FVector2D(HalfUV,HalfUV),UV+FVector2D(HalfUV,HalfUV)));
    FSlateDrawElement::MakeBox(Out,Layer+6,G.ToPaintGeometry(FVector2D(Side,Side),FSlateLayoutTransform(Origin)),&Image,ESlateDrawEffect::None,FLinearColor(.85f,.94f,.85f));
   }
   auto Project=[&](const FVector& P){return Origin+FVector2D(.5+(P.Y-Center.Y)/Width,.5-(P.X-Center.X)/Width)*Side;};
   const double Step=Map->bOverview?10000.:2000.;
   for(double Y=FMath::CeilToDouble((Center.Y-Width*.5)/Step)*Step;Y<Center.Y+Width*.5;Y+=Step)
   {const double X=Project(FVector(Center.X,Y,0)).X;Line({FVector2f(X,Origin.Y),FVector2f(X,Origin.Y+Side)},FLinearColor(1,1,1,.09f),1,Layer+7);}
   for(double X=FMath::CeilToDouble((Center.X-Width*.5)/Step)*Step;X<Center.X+Width*.5;X+=Step)
   {const double Y=Project(FVector(X,Center.Y,0)).Y;Line({FVector2f(Origin.X,Y),FVector2f(Origin.X+Side,Y)},FLinearColor(1,1,1,.09f),1,Layer+7);}
   auto Marker=[&](const FVector& World,const FString& Label,FLinearColor Color,bool Home)
   {
    FVector2D P=Project(World);const bool Outside=P.X<Origin.X+8 || P.X>Origin.X+Side-8 || P.Y<Origin.Y+8 || P.Y>Origin.Y+Side-8;
    P.X=FMath::Clamp(P.X,Origin.X+8,Origin.X+Side-8);P.Y=FMath::Clamp(P.Y,Origin.Y+8,Origin.Y+Side-8);
    Box(P-FVector2D(5,5),FVector2D(10,10),FLinearColor(.025f,.04f,.03f),Layer+8);
    if(Home)Line({FVector2f(P.X-6,P.Y),FVector2f(P.X,P.Y-6),FVector2f(P.X+6,P.Y),FVector2f(P.X,P.Y+6),FVector2f(P.X-6,P.Y)},Color,3,Layer+9);
    else Box(P-FVector2D(3,3),FVector2D(6,6),Color,Layer+9);
    FString Name=Label;if(Outside)Name+=FString::Printf(TEXT(" %.0fm"),FVector::Dist2D(Map->PlayerLocation,World)/100.);
    const double LabelOffset=Home?10:Label==TEXT("Fawnia")?-5:Label==TEXT("女巫")?-30:-17;
    const FVector2D LabelPos(FMath::Clamp(P.X+9,Origin.X+4,Origin.X+Side-65),FMath::Clamp(P.Y+LabelOffset,Origin.Y+3,Origin.Y+Side-15));
    Text(Name,LabelPos+FVector2D(1,1),9,FLinearColor::Black,Layer+10);Text(Name,LabelPos,9,Color,Layer+11);
   };
   if(Map->bHasHome)Marker(Map->HomeLocation,TEXT("小屋"),Gold,true);
   for(const auto& M:Map->Markers)Marker(M.Location,M.Label,M.Color,false);
   FVector2D Player=Project(Map->PlayerLocation);Player.X=FMath::Clamp(Player.X,Origin.X+12,Origin.X+Side-12);Player.Y=FMath::Clamp(Player.Y,Origin.Y+12,Origin.Y+Side-12);
   const double Heading=FMath::DegreesToRadians(Map->PlayerHeading);const FVector2D Forward(FMath::Sin(Heading),-FMath::Cos(Heading)),Right(-Forward.Y,Forward.X);
   const FVector2D Tip=Player+Forward*11,Left=Player-Forward*6-Right*6,R=Player-Forward*6+Right*6;
   const TArray<FVector2f> Arrow={FVector2f(Tip),FVector2f(Left),FVector2f(R),FVector2f(Tip)};
   Line(Arrow,FLinearColor(.01f,.03f,.02f),6,Layer+12);Line(Arrow,Teal,3,Layer+13);Box(Player-FVector2D(1.5,1.5),FVector2D(3,3),Cream,Layer+14);
   const double ScaleWorld=Map->bOverview?20000.:5000.,Bar=ScaleWorld/Width*Side;
   const FVector2D ScaleOrigin=Origin+FVector2D(10,Side-12);
   Line({FVector2f(ScaleOrigin.X,ScaleOrigin.Y-4),FVector2f(ScaleOrigin),FVector2f(ScaleOrigin.X+Bar,ScaleOrigin.Y),FVector2f(ScaleOrigin.X+Bar,ScaleOrigin.Y-4)},Cream,2,Layer+15);
   Text(Map->bOverview?TEXT("200m"):TEXT("50m"),ScaleOrigin+FVector2D(0,-21),9,Cream,Layer+15);
   const FVector2D Footer=Origin+FVector2D(0,Side+8);
   const FString Position=FString::Printf(TEXT("X %.0fm  ·  Y %.0fm"),Map->PlayerLocation.X/100.,Map->PlayerLocation.Y/100.);
   Text(Position,Footer,10,Cream,Layer+16);
   const FString Home=Map->bHasHome?FString::Printf(TEXT("距小屋 %.0fm"),FVector::Dist2D(Map->PlayerLocation,Map->HomeLocation)/100.):TEXT("跟随主角 · 北向固定");
   Text(Home,Footer+FVector2D(0,17),9,Gold,Layer+16);
   Text(Map->bOverview?TEXT("M 收起全图   ▲ 你  ◆ 小屋  ■ NPC"):TEXT("M 查看全图   ▲ 你  ◆ 小屋  ■ NPC"),Footer+FVector2D(0,34),9,Cream,Layer+16);
   return Layer+16;
  }
 private:
  TWeakObjectPtr<UForestMinimapWidget> Owner;
  mutable FSlateBrush Image;
 };
}

UForestMinimapWidget::UForestMinimapWidget(const FObjectInitializer& Initializer):Super(Initializer)
{
 static ConstructorHelpers::FObjectFinder<UTexture2D> Texture(TEXT("/Game/ForestMigration/UI/T_DemoMap_Minimap.T_DemoMap_Minimap"));
 MapTexture=Texture.Object;
}
TSharedRef<SWidget> UForestMinimapWidget::RebuildWidget(){return SNew(SForestMinimap).Owner(this);}
FVector2D UForestMinimapWidget::WorldToMapUV(FVector P) const{return FVector2D(.5+(P.Y-WorldCenter.Y)/WorldWidth,.5-(P.X-WorldCenter.X)/WorldWidth);}
FVector2D UForestMinimapWidget::GetViewCenter() const
{
 if(bOverview)return WorldCenter;
 const double Limit=FMath::Max(0.,(WorldWidth-LocalWidth)*.5);
 return FVector2D(FMath::Clamp(PlayerLocation.X,WorldCenter.X-Limit,WorldCenter.X+Limit),FMath::Clamp(PlayerLocation.Y,WorldCenter.Y-Limit,WorldCenter.Y+Limit));
}
void UForestMinimapWidget::ToggleOverview(){bOverview=!bOverview;InvalidateLayoutAndVolatility();}
void UForestMinimapWidget::UpdateMap(float DT)
{
 UWorld* W=GetWorld();auto* P=Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(W,0));if(!W || !P)return;
 PlayerLocation=P->GetActorLocation();PlayerHeading=P->GetMesh()->GetComponentRotation().Yaw+90;
 ActorScanDelay-=DT;
 if(ActorScanDelay<=0)
 {
  ActorScanDelay=1;NPCs.Reset();
  for(TActorIterator<ACharacter> It(W);It;++It)if(*It!=P && It->GetName().StartsWith(TEXT("BP_NPC_")))NPCs.Add(*It);
  bHasHome=false;for(TActorIterator<APlayerStart> It(W);It;++It)if(It->ActorHasTag(TEXT("Forest.CabinSpawn"))){HomeLocation=It->GetActorLocation();bHasHome=true;break;}
 }
 Markers.Reset();
 for(auto& Weak:NPCs)if(auto* NPC=Weak.Get())
 {
  FForestMapMarker M;M.Location=NPC->GetActorLocation();const FString N=NPC->GetName();
  M.Label=N.Contains(TEXT("Natta"))?TEXT("Natta"):N.Contains(TEXT("Fawnia"))?TEXT("Fawnia"):TEXT("女巫");
  M.Color=N.Contains(TEXT("Natta"))?FLinearColor(.55f,.8f,1):N.Contains(TEXT("Fawnia"))?FLinearColor(1,.65f,.5f):FLinearColor(.83f,.64f,1);Markers.Add(M);
 }
 if(auto Widget=GetCachedWidget())Widget->Invalidate(EInvalidateWidgetReason::Paint);
}
