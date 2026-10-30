#include "ForestUITheme.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Rendering/DrawElementTypes.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"

namespace ForestTheme
{
 static FLinearColor Hex(const TCHAR* Value){return FLinearColor::FromSRGBColor(FColor::FromHex(Value));}
 const FLinearColor Paper=Hex(TEXT("F4E8D1")),HeaderPaper=Hex(TEXT("E9D9B9")),InsetPaper=Hex(TEXT("EEDFC4"));
 const FLinearColor Edge=Hex(TEXT("BA9B70")),Ink=Hex(TEXT("57452F")),Muted=Hex(TEXT("87765B"));
 const FLinearColor Moss=Hex(TEXT("77804D")),OnMoss=Hex(TEXT("FFF7E5")),SelectedPaper=Hex(TEXT("DFE5C8"));
 const FLinearColor Fire=Hex(TEXT("BE6C4E")),Wind=Hex(TEXT("7D8C58")),Water=Hex(TEXT("688E97"));
 FSlateFontInfo Font(int32 Size,bool Bold){return FCoreStyle::GetDefaultFontStyle(Bold?TEXT("Bold"):TEXT("Regular"),Size+2);}
 const FSlateBrush* PaperSheet()
 {
  static const TSharedPtr<FSlateDynamicImageBrush> Brush=[]()->TSharedPtr<FSlateDynamicImageBrush>{
   const FString Path=FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()/TEXT("UI/Textures/WoodlandPaper.png"));
   if(!IFileManager::Get().FileExists(*Path))
   {
    UE_LOG(LogTemp,Warning,TEXT("Forest UI paper missing: %s; using plain fallback."),*Path);
    return TSharedPtr<FSlateDynamicImageBrush>();
   }
   return MakeShared<FSlateDynamicImageBrush>(FName(*Path),FVector2D(1536,1024));
  }();
  return Brush.IsValid()?Brush.Get():Fill();
 }
 const FSlateBrush* ElementIcon(int32 Index)
 {
  static const TArray<TSharedPtr<FSlateDynamicImageBrush>> Brushes=[](){
   TArray<TSharedPtr<FSlateDynamicImageBrush>> Result;
   const FString Path=FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()/TEXT("UI/Textures/ElementEmblems.png"));
   if(IFileManager::Get().FileExists(*Path))
   {
    for(int32 I=0;I<3;++I){
    auto B=MakeShared<FSlateDynamicImageBrush>(FName(*Path),FVector2D(60,60));
    B->SetUVRegion(FBox2f(FVector2f(I/3.f,0),FVector2f((I+1)/3.f,1)));Result.Add(B);
       }
   }
   return Result;
  }();
  return Brushes.IsValidIndex(Index)?Brushes[Index].Get():FCoreStyle::Get().GetBrush(TEXT("NoBrush"));
 }
 const FSlateBrush* Fill(){static const FSlateRoundedBoxBrush Brush(FLinearColor::White,8.f);return &Brush;}
 static FButtonStyle MakeButton(bool Primary)
 {
  FButtonStyle S;
  S.SetNormal(FSlateRoundedBoxBrush(Primary?Moss:Paper,7.f,Primary?Moss:Edge,1.f));
  S.SetHovered(FSlateRoundedBoxBrush(Primary?Hex(TEXT("88945D")):Hex(TEXT("FAEFD8")),7.f,Primary?Moss:Hex(TEXT("A88A60")),1.5f));
  S.SetPressed(FSlateRoundedBoxBrush(Primary?Hex(TEXT("65713F")):HeaderPaper,7.f,Edge,1.f));
  S.SetDisabled(FSlateRoundedBoxBrush(Hex(TEXT("DED4BE")),7.f,Hex(TEXT("C8BBA1")),1.f));
  S.SetNormalForeground(Primary?OnMoss:Ink).SetHoveredForeground(Primary?OnMoss:Ink).SetPressedForeground(Primary?OnMoss:Ink).SetDisabledForeground(Muted);
  S.SetNormalPadding(FMargin(12,9)).SetPressedPadding(FMargin(12,10,12,8));return S;
 }
 const FButtonStyle& PrimaryButton(){static const FButtonStyle S=MakeButton(true);return S;}
 const FButtonStyle& SecondaryButton(){static const FButtonStyle S=MakeButton(false);return S;}
 const FEditableTextBoxStyle& InputStyle()
 {
  static const FEditableTextBoxStyle S=[](){FEditableTextBoxStyle R;
   R.SetBackgroundImageNormal(FSlateRoundedBoxBrush(Paper,6.f,Edge,1.f));
   R.SetBackgroundImageHovered(FSlateRoundedBoxBrush(Paper,6.f,Moss,1.f));
   R.SetBackgroundImageFocused(FSlateRoundedBoxBrush(Paper,6.f,Moss,2.f));
   R.SetBackgroundImageReadOnly(FSlateRoundedBoxBrush(InsetPaper,6.f,Edge,1.f));
   R.SetForegroundColor(Ink).SetReadOnlyForegroundColor(Muted).SetPadding(FMargin(12,10));return R;}();return S;
 }
 const FComboBoxStyle& ComboStyle()
 {
  static const FComboBoxStyle S=[](){FComboBoxStyle R=FCoreStyle::Get().GetWidgetStyle<FComboBoxStyle>(TEXT("ComboBox"));
   FComboButtonStyle B=R.ComboButtonStyle;B.SetButtonStyle(SecondaryButton());B.SetMenuBorderBrush(FSlateRoundedBoxBrush(Paper,6.f,Edge,1.f));B.SetMenuBorderPadding(FMargin(6));R.SetComboButtonStyle(B);return R;}();return S;
 }
 const FTableRowStyle& ComboRowStyle()
 {
  static const FTableRowStyle S=[](){FTableRowStyle R;
   R.SetEvenRowBackgroundBrush(FSlateRoundedBoxBrush(Paper,4.f)).SetOddRowBackgroundBrush(FSlateRoundedBoxBrush(Paper,4.f));
   R.SetEvenRowBackgroundHoveredBrush(FSlateRoundedBoxBrush(HeaderPaper,4.f)).SetOddRowBackgroundHoveredBrush(FSlateRoundedBoxBrush(HeaderPaper,4.f));
   R.SetActiveBrush(FSlateRoundedBoxBrush(SelectedPaper,4.f)).SetInactiveBrush(FSlateRoundedBoxBrush(SelectedPaper,4.f));
   R.SetActiveHoveredBrush(FSlateRoundedBoxBrush(SelectedPaper,4.f)).SetInactiveHoveredBrush(FSlateRoundedBoxBrush(SelectedPaper,4.f));
   R.SetSelectorFocusedBrush(FSlateRoundedBoxBrush(FLinearColor::Transparent,4.f,Moss,1.f));R.SetTextColor(Ink).SetSelectedTextColor(Ink);return R;}();return S;
 }

}

namespace
{
 // Original artwork remains byte-for-byte unchanged. These source-pixel points
 // follow its paper silhouette, excluding the photographed brown rectangle.
 // The contour is star-shaped about (768,512), so a triangle fan is sufficient.
 const TArray<FVector2f>& PaperContour()
 {
  static const TArray<FVector2f> Points={
   {28,108},{60,77},{88,52},{120,60},{159,56},{193,64},{276,58},
   {388,61},{505,54},{636,57},{752,52},{889,56},{1020,52},{1135,57},
   {1260,54},{1370,50},{1403,58},{1432,54},{1458,68},{1482,76},
   {1502,94},{1501,158},{1508,215},{1502,284},{1507,356},{1499,422},
   {1506,470},{1501,544},{1506,615},{1500,690},{1507,751},{1500,822},
   {1487,872},{1465,902},{1454,929},{1422,947},{1376,944},{1324,955},
   {1240,948},{1125,957},{1000,950},{880,955},{751,950},{630,957},
   {502,950},{390,955},{280,950},{193,955},{128,946},{84,932},
   {47,917},{29,890},{33,818},{28,744},{33,671},{28,598},
   {34,524},{29,455},{33,383},{29,310},{33,238},{28,169}
  };
  return Points;
 }

 // Clip each source triangle to a single stretch cell. Geometry and UVs use
 // the same piecewise-affine transform; no corner is stretched across a seam.
 TArray<FVector2f> ClipPaper(TArray<FVector2f> P,int Axis,float Limit,bool KeepGreater)
 {
  TArray<FVector2f> Out;
  if(P.IsEmpty())return Out;
  auto Value=[Axis](const FVector2f& V){return Axis==0?V.X:V.Y;};
  FVector2f A=P.Last();float DA=Value(A)-Limit;
  for(const FVector2f B:P)
  {
   const float DB=Value(B)-Limit;
   const bool InA=KeepGreater?DA>=0:DA<=0,InB=KeepGreater?DB>=0:DB<=0;
   if(InA!=InB)Out.Add(A+(B-A)*(DA/(DA-DB)));
   if(InB)Out.Add(B);
   A=B;DA=DB;
  }
  return Out;
 }

 // Shared with the offline layout preview by extraction; coordinates are source
 // pixels, target borders are Slate units. The entire flower is inside cell 8.
 constexpr float PaperSourceX[]={0,320,1216,1536};
 constexpr float PaperSourceY[]={0,160,704,1024};
 constexpr float PaperCorner=128.f;
 constexpr float PaperFlowerMinWidth=480.f;
 constexpr float PaperFlowerMinHeight=240.f;
}

void SForestPaperBorder::Construct(const FArguments& Args)
{
 SBorder::Construct(Args);SetPadding(Args._Padding);
 SetBorderImage(ForestTheme::PaperSheet());SetBorderBackgroundColor(FLinearColor::White);
 SetClipping(EWidgetClipping::ClipToBounds);
}
void SForestPaperBorder::SetPadding(TAttribute<FMargin> InPadding)
{
 // Reserve room for the source edge and floral corner. The fixed right gutter
 // also protects descriptions that extend to the bottom of a large panel.
 // No previous-frame geometry is used, avoiding first-frame/resize oscillation.
 SBorder::SetPadding(TAttribute<FMargin>::CreateLambda([InPadding](){
  const FMargin P=InPadding.Get();
  return FMargin(FMath::Max(20.f,P.Left),FMath::Max(28.f,P.Top),FMath::Max(104.f,P.Right),FMath::Max(32.f,P.Bottom));
 }));
}
int32 SForestPaperBorder::OnPaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& Cull,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool Enabled) const
{
 const FVector2f Size(Geometry.GetLocalSize());const float W=Size.X,H=Size.Y;
 if(W<=0 || H<=0)return Layer;
 const bool bEnabled=ShouldBeEnabled(Enabled);
 const auto Effect=GetShowDisabledEffect() && !bEnabled?ESlateDrawEffect::DisabledEffect:ESlateDrawEffect::None;
 const FSlateBrush* Brush=ForestTheme::PaperSheet();
 if(Brush==ForestTheme::Fill())
 {
  static const FSlateRoundedBoxBrush Fallback(ForestTheme::Paper,6.f,ForestTheme::Edge,1.f);
  FSlateDrawElement::MakeBox(Out,Layer,Geometry.ToPaintGeometry(),&Fallback,Effect,Style.GetColorAndOpacityTint());
 }
 else
 {
  // Uniformly scale the corner patches, independently of the stretchable paper
  // centre. At very small heights the paper becomes a strip without flowers.
  const float Scale=FMath::Min(PaperCorner/320.f,FMath::Min(W/800.f,H/600.f));
  const float S=FMath::Max(.001f,Scale);
  const float DX[]={0,320*S,W-320*S,W};
  const float DY[]={0,160*S,H-320*S,H};
  const bool bFlower=W>=PaperFlowerMinWidth && H>=PaperFlowerMinHeight;
  if(CachedPaperSize!=Size)
  {
   CachedPaperSize=Size;PaperPositions.Reset();PaperUVs.Reset();PaperIndices.Reset();
   const auto& Contour=PaperContour();
   for(int32 Row=0;Row<3;++Row)for(int32 Col=0;Col<3;++Col)
   {
    // Short notices replace the flower corner with a mirrored plain corner.
    const bool Mirror=Row==2 && Col==2 && !bFlower;
    const int32 SourceCol=Mirror?0:Col;
    for(int32 I=0;I<Contour.Num();++I)
    {
     TArray<FVector2f> Poly={FVector2f(768,512),Contour[I],Contour[(I+1)%Contour.Num()]};
     Poly=ClipPaper(MoveTemp(Poly),0,PaperSourceX[SourceCol],true);
     Poly=ClipPaper(MoveTemp(Poly),0,PaperSourceX[SourceCol+1],false);
     Poly=ClipPaper(MoveTemp(Poly),1,PaperSourceY[Row],true);
     Poly=ClipPaper(MoveTemp(Poly),1,PaperSourceY[Row+1],false);
     if(Poly.Num()<3)continue;
     const SlateIndex Base=PaperPositions.Num();
     for(const FVector2f P:Poly)
     {
      float U=(P.X-PaperSourceX[SourceCol])/(PaperSourceX[SourceCol+1]-PaperSourceX[SourceCol]);
      if(Mirror)U=1-U;
      const float V=(P.Y-PaperSourceY[Row])/(PaperSourceY[Row+1]-PaperSourceY[Row]);
      const FVector2f Pos(FMath::Lerp(DX[Col],DX[Col+1],U),FMath::Lerp(DY[Row],DY[Row+1],V));
      PaperPositions.Add(Pos);PaperUVs.Add(FVector2f(P.X/1536.f,P.Y/1024.f));
     }
     for(int32 J=1;J+1<Poly.Num();++J){PaperIndices.Add(Base);PaperIndices.Add(Base+J);PaperIndices.Add(Base+J+1);}
    }
   }
  }
  TArray<FSlateVertex> Vertices;Vertices.Reserve(PaperPositions.Num());
  const FColor Color=Style.GetColorAndOpacityTint().ToFColor(true);
  for(int32 I=0;I<PaperPositions.Num();++I)
   Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Geometry.GetAccumulatedRenderTransform(),PaperPositions[I],PaperUVs[I],Color));
  const auto Handle=FSlateApplication::Get().GetRenderer()->GetResourceHandle(*Brush);
  FSlateDrawElement::MakeCustomVerts(Out,Layer,Handle,Vertices,PaperIndices,nullptr,0,0,Effect);
 }
 return SCompoundWidget::OnPaint(Args,Geometry,Cull,Out,Layer+1,Style,bEnabled);
}
