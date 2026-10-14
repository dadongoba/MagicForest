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
  static const TSharedPtr<FSlateDynamicImageBrush> Brush=[]()->TSharedPtr<FSlateDynamicImageBrush>{
   const FString Path=FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()/TEXT("UI/Textures/WoodlandPaper.png"));
   if(!IFileManager::Get().FileExists(*Path))return TSharedPtr<FSlateDynamicImageBrush>();
   auto R=MakeShared<FSlateDynamicImageBrush>(FName(*Path),FVector2D(1536,1024));
   R->DrawAs=ESlateBrushDrawType::Box;R->Margin=FMargin(.13f,.15f);return R;
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

int32 SForestPaperBorder::OnPaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& Cull,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool Enabled) const
{
 return SBorder::OnPaint(Args,Geometry,Cull,Out,Layer,Style,Enabled);
}
