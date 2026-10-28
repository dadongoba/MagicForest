#include "ForestUITheme.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Rendering/DrawElementTypes.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

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
  // No raster margins or intrinsic image size can influence the panel layout.
  static const FSlateRoundedBoxBrush Brush(Paper,6.f,Edge,1.f);return &Brush;
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

void SForestPaperBorder::Construct(const FArguments& Args)
{
 SBorder::Construct(Args);SetPadding(Args._Padding);
 SetBorderImage(ForestTheme::PaperSheet());SetBorderBackgroundColor(FLinearColor::White);
 SetClipping(EWidgetClipping::ClipToBounds);
}
void SForestPaperBorder::SetPadding(TAttribute<FMargin> InPadding)
{
 // Keep the border and botanical marks outside the content, including when a
 // caller updates padding after a resize. Preserve larger caller-provided gaps.
 SBorder::SetPadding(TAttribute<FMargin>::CreateLambda([InPadding](){
  const FMargin P=InPadding.Get();return FMargin(FMath::Max(16.f,P.Left),FMath::Max(14.f,P.Top),FMath::Max(16.f,P.Right),FMath::Max(14.f,P.Bottom));
 }));
}
int32 SForestPaperBorder::OnPaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& Cull,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool Enabled) const
{
 const FVector2f Size(Geometry.GetLocalSize());const float W=Size.X,H=Size.Y;
 if(W<=0 || H<=0)return Layer;
 const bool bEnabled=ShouldBeEnabled(Enabled);
 const auto Effect=GetShowDisabledEffect() && !bEnabled?ESlateDrawEffect::DisabledEffect:ESlateDrawEffect::None;
 const auto Tint=Style.GetColorAndOpacityTint();
 auto Paint=[&](float X,float Y,float Width,float Height){return Geometry.ToPaintGeometry(FVector2f(FMath::Max(0.f,Width),FMath::Max(0.f,Height)),FSlateLayoutTransform(FVector2f(X,Y)));};
 static const FSlateRoundedBoxBrush Shadow(FLinearColor::White,6.f);
 FSlateDrawElement::MakeBox(Out,Layer,Paint(1,2,W-2,H-2),&Shadow,Effect,FLinearColor(.19f,.12f,.055f,.24f)*Tint);
 // Redesign the sheet at its actual allotted size. A narrow HUD strip and a
 // full inventory use the same small edge width, rather than scaled PNG corners.
 const float R=FMath::Min(6.f,FMath::Min(W,H)*.2f);
 TArray<FSlateGradientStop> Stops;
 Stops.Emplace(FVector2f(0,0),ForestTheme::HeaderPaper*Tint);
 Stops.Emplace(FVector2f(0,H*.28f),ForestTheme::Paper*Tint);
 Stops.Emplace(FVector2f(0,H*.72f),FLinearColor(.96f,.90f,.78f,1)*Tint);
 Stops.Emplace(FVector2f(0,H),ForestTheme::HeaderPaper*Tint);
 FSlateDrawElement::MakeGradient(Out,Layer+1,Paint(1,1,W-2,H-3),MoveTemp(Stops),Orient_Vertical,Effect,FVector4f(R));
 auto Line=[&](TArray<FVector2f> Points,FLinearColor Color,float Thickness=1.f){
  FSlateDrawElement::MakeLines(Out,Layer+2,Geometry.ToPaintGeometry(),MoveTemp(Points),Effect,Color*Tint,true,Thickness);
 };
 // A gently uneven ink edge follows all four sides. Every point stays inside
 // the widget bounds; line thickness and content gutter are in Slate units.
 const float Inset=FMath::Min(4.f,FMath::Min(W,H)*.15f);
 TArray<FVector2f> Edge;
 const int32 NX=FMath::Clamp(FMath::CeilToInt(W/70.f),2,24),NY=FMath::Clamp(FMath::CeilToInt(H/70.f),2,18);
 for(int32 I=0;I<=NX;++I)Edge.Emplace(FMath::Lerp(Inset,W-Inset,I/float(NX)),Inset+.65f*FMath::Sin(I*2.1f));
 for(int32 I=0;I<=NY;++I)Edge.Emplace(W-Inset+.6f*FMath::Sin(I*2.3f),FMath::Lerp(Inset,H-Inset,I/float(NY)));
 for(int32 I=NX;I>=0;--I)Edge.Emplace(FMath::Lerp(Inset,W-Inset,I/float(NX)),H-Inset+.65f*FMath::Sin(I*1.9f));
 for(int32 I=NY;I>=0;--I)Edge.Emplace(Inset+.6f*FMath::Sin(I*2.4f),FMath::Lerp(Inset,H-Inset,I/float(NY)));
 // TArray forbids adding a reference into its own storage (CheckAddress).
 // Copy the closing vertex before Add can reallocate or validate the array.
 const FVector2f ClosingPoint=Edge[0];
 Edge.Add(ClosingPoint);Line(MoveTemp(Edge),ForestTheme::Edge.CopyWithNewOpacity(.7f));
 if(W>=240 && H>=100)
 {
  // Small olive sprig entirely in the right gutter, never below story text.
  const float X=W-9.f,Bottom=H-15.f,Length=FMath::Min(58.f,H*.2f);
  Line({FVector2f(X,Bottom),FVector2f(X-1,Bottom-Length)},ForestTheme::Moss.CopyWithNewOpacity(.45f));
  for(int32 I=1;I<=4;++I)
  {
   const float Y=Bottom-Length*I/5.f,Side=I%2?1.f:-1.f;
   Line({FVector2f(X,Y+3),FVector2f(X+Side*3.5f,Y),FVector2f(X+Side*3.f,Y-5),FVector2f(X,Y-1)},ForestTheme::Moss.CopyWithNewOpacity(.4f));
  }
 }
 return SCompoundWidget::OnPaint(Args,Geometry,Cull,Out,Layer+3,Style,bEnabled);
}
