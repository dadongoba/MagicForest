#pragma once
#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Layout/SBorder.h"
#include "Rendering/RenderingCommon.h"

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

/** Original paper artwork with silhouette clipping and independent corner scaling. */
class SForestPaperBorder : public SBorder
{
public:
 using FArguments=SBorder::FArguments;
 void Construct(const FArguments& Args);
 void SetPadding(TAttribute<FMargin> InPadding);
 virtual int32 OnPaint(const FPaintArgs&,const FGeometry&,const FSlateRect&,FSlateWindowElementList&,int32,const FWidgetStyle&,bool) const override;
private:
 // Rebuild clipping only on resize; transforms and tint remain live each paint.
 mutable FVector2f CachedPaperSize=FVector2f(-1,-1);
 mutable TArray<FVector2f> PaperPositions,PaperUVs;
 mutable TArray<SlateIndex> PaperIndices;
};
