#pragma once
#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Layout/SBorder.h"

/** Shared runtime Slate styles, aligned with the warm hand-painted woodland reference. */
namespace ForestTheme
{
 extern const FLinearColor Paper,HeaderPaper,InsetPaper,Edge,Ink,Muted,Moss,OnMoss,Fire,Wind,Water,SelectedPaper;
 FSlateFontInfo Font(int32 Size,bool Bold=false);
 const FSlateBrush* Fill();
 const FSlateBrush* PaperSheet();
 const FSlateBrush* ElementIcon(int32 Index);
 const FButtonStyle& PrimaryButton();
 const FButtonStyle& SecondaryButton();
 const FEditableTextBoxStyle& InputStyle();
 const FComboBoxStyle& ComboStyle();
 const FTableRowStyle& ComboRowStyle();
}

/** Textured paper container; the same asset is used in the offline preview. */
class SForestPaperBorder : public SBorder
{
public:
 using FArguments=SBorder::FArguments;
 void Construct(const FArguments& Args){SBorder::Construct(Args);SetBorderImage(ForestTheme::PaperSheet());SetBorderBackgroundColor(FLinearColor::White);}
 virtual int32 OnPaint(const FPaintArgs&,const FGeometry&,const FSlateRect&,FSlateWindowElementList&,int32,const FWidgetStyle&,bool) const override;
};
