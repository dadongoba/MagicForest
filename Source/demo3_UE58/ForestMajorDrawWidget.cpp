#include "ForestMajorDrawWidget.h"

#include "ForestAlchemy.h"
#include "ForestMajorArt.h"
#include "ForestUITheme.h"
#include "InputCoreTypes.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
    TSharedRef<STextBlock> DrawText(const FString& Value, int32 Size, bool bBold = false,
        FLinearColor Color = ForestTheme::Ink)
    {
        return SNew(STextBlock)
            .Text(FText::FromString(Value))
            .Font(ForestTheme::Font(Size, bBold))
            .ColorAndOpacity(Color)
            .AutoWrapText(true);
    }
}

void UForestMajorDrawWidget::Configure(FName CardId, const FString& Name,
    const FString& ShortEffect, int32 Cost, int32 Day, int32 Drawn, int32 Limit,
    bool bFirstDraw)
{
    ShownCardId = CardId;
    ShownName = Name;
    ShownEffect = ShortEffect;
    ShownCost = Cost;
    ShownDay = Day;
    ShownDrawn = Drawn;
    ShownLimit = Limit;
    bShowFirstDrawGuide = bFirstDraw;
    RevealAge = 0.0f;
    Refresh();
}

TSharedRef<SWidget> UForestMajorDrawWidget::RebuildWidget()
{
    SetIsFocusable(true);
    TSharedRef<SWidget> Root = SNew(SOverlay)
        + SOverlay::Slot()
        [SNew(SBorder)
            .BorderImage(ForestTheme::Fill())
            .BorderBackgroundColor(FLinearColor(0.04f, 0.035f, 0.027f, 0.72f))]
        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(16.0f)
        [SAssignNew(Frame, SBox).WidthOverride(920.0f).HeightOverride(640.0f)
            [SNew(SForestPaperBorder).Padding(FMargin(30.0f, 24.0f, 28.0f, 28.0f))
                [SAssignNew(Body, SVerticalBox)]]];
    Refresh();
    return Root;
}

void UForestMajorDrawWidget::Refresh()
{
    if (!Body.IsValid()) return;
    Body->ClearChildren();
    CardMotion.Reset();

    const int32 SafeLimit = FMath::Max(1, ShownLimit);
    const int32 Left = FMath::Max(0, SafeLimit - ShownDrawn);
    Body->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)
        [DrawText(FString::Printf(TEXT("第 %d 天 · 药瓶里显出一张日运牌"), ShownDay), 18, true, ForestTheme::Moss)];

    if (bShowFirstDrawGuide)
    {
        Body->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 14.0f)
            [DrawText(TEXT("蘑菇提炼出的火、风、水是药剂的材料；「日运」是今天从药瓶借来的调药手法，会改变材料入锅后的结果。白天听委托、采元素，夜里再决定怎么用这张牌。"),
                18, false, ForestTheme::Ink)];
    }

    const float CardHeight = bCompact ? 340.0f : 450.0f;
    const float CardWidth = CardHeight * ForestMajorArt::AspectRatio;
    TSharedRef<SWidget> Card = ForestMajorArt::BrushFor(ShownCardId)
        ? StaticCastSharedRef<SWidget>(SNew(SBorder)
            .BorderImage(ForestTheme::Fill())
            .BorderBackgroundColor(ForestTheme::Edge)
            .Padding(3.0f)
            [SNew(SBox).WidthOverride(CardWidth).HeightOverride(CardHeight)
                [SNew(SImage).Image(ForestMajorArt::BrushFor(ShownCardId))]])
        : StaticCastSharedRef<SWidget>(SNew(SBorder)
            .BorderImage(ForestTheme::Fill())
            .BorderBackgroundColor(ForestTheme::HeaderPaper)
            .Padding(12.0f)
            [SNew(SBox).WidthOverride(CardWidth).HeightOverride(CardHeight)
                .HAlign(HAlign_Center).VAlign(VAlign_Center)
                [DrawText(ShownCardId.ToString() + TEXT("\n") + ShownName +
                    TEXT("\n\n牌图暂不可用"), 16, true)]]);

    TSharedRef<SWidget> Art = SAssignNew(CardMotion, SBox)
        .HAlign(HAlign_Center).VAlign(VAlign_Center)[Card];
    CardMotion->SetRenderOpacity(FMath::Clamp(RevealAge / 0.6f, 0.0f, 1.0f));
    CardMotion->SetRenderTransform(FSlateRenderTransform(
        FVector2D(0.0f, 26.0f * (1.0f - FMath::Clamp(RevealAge / 0.6f, 0.0f, 1.0f)))));

    TSharedRef<SVerticalBox> Details = SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)
        [DrawText(ShownCardId.ToString() + TEXT(" · ") + ShownName, 27, true)]
        + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)
        [DrawText(TEXT("这张牌会怎样改变锅里的药性"), 16, true, ForestTheme::Moss)]
        + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 16.0f)
        [DrawText(ShownEffect.IsEmpty() ? TEXT("用法请在魔法书中查看。") : ShownEffect, 19)]
        + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 16.0f)
        [DrawText(FString::Printf(TEXT("夜间使用时占 1 个槽位、花 %d 点额度；每份配方最多可花 5 点。"),
            ShownCost), 16)]
        + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)
        [DrawText(FString::Printf(TEXT("今天已抽 %d / %d 张 · 还可抽 %d 张"),
            ShownDrawn, SafeLimit, Left), 17, true, ForestTheme::Moss)]
        + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 13.0f)
        [DrawText(TEXT("牌已收进背包「今日大牌」。按 B 看牌图，按 J 翻魔法书查用法。"), 16)]
        + SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)
        [DrawText(TEXT("今天结束后，没用过的牌也会失效。"), 16, true, ForestTheme::Fire)];

    if (bShowFirstDrawGuide)
    {
        Details->AddSlot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 8.0f)
            [DrawText(TEXT("有的牌让锅里的火更旺，有的把一种元素换成另一种。每张牌的作用不同；把元素小物和牌按顺序放上合成台，放在哪里也会影响结果。"), 17)];
        Details->AddSlot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 8.0f)
            [DrawText(FString::Printf(TEXT("每天最多抽 %d 张，供当天三份委托共用。确认封瓶才会用掉牌；用过不补，今天结束后未用的也会失效。大牌不是每份药都必需。"),
                SafeLimit), 17)];
    }

    TSharedRef<SWidget> Main = bCompact
        ? StaticCastSharedRef<SWidget>(SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 15.0f)[Art]
            + SVerticalBox::Slot().AutoHeight()[Details])
        : StaticCastSharedRef<SWidget>(SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 28.0f, 0.0f)[Art]
            + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)[Details]);

    Body->AddSlot().FillHeight(1.0f)
        [SNew(SScrollBox).ScrollBarAlwaysVisible(true)
            + SScrollBox::Slot()[Main]];
    Body->AddSlot().AutoHeight().HAlign(HAlign_Right).Padding(0.0f, 15.0f, 0.0f, 0.0f)
        [SNew(SButton).ButtonStyle(&ForestTheme::PrimaryButton())
            .ContentPadding(FMargin(22.0f, 10.0f))
            .OnClicked_Lambda([this]() { CloseReveal(); return FReply::Handled(); })
            [DrawText(TEXT("收下日运  ·  Esc 关闭"), 16, true, ForestTheme::OnMoss)]];
}

void UForestMajorDrawWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    const FVector2D Available = Geometry.GetLocalSize();
    if (Frame.IsValid() && Available.X > 0.0 && Available.Y > 0.0)
    {
        const float Width = FMath::Max(1.0f, FMath::Min(920.0f, static_cast<float>(Available.X) - 32.0f));
        const float Height = FMath::Max(1.0f, FMath::Min(640.0f, static_cast<float>(Available.Y) - 32.0f));
        Frame->SetWidthOverride(Width);
        Frame->SetHeightOverride(Height);
        const bool bShouldCompact = Width < 680.0f;
        if (bCompact != bShouldCompact)
        {
            bCompact = bShouldCompact;
            Refresh();
        }
    }

    RevealAge = FMath::Min(0.6f, RevealAge + FMath::Max(0.0f, DeltaTime));
    if (CardMotion.IsValid())
    {
        const float Phase = FMath::Clamp(RevealAge / 0.6f, 0.0f, 1.0f);
        const float Eased = Phase * Phase * (3.0f - 2.0f * Phase);
        CardMotion->SetRenderOpacity(Eased);
        CardMotion->SetRenderTransform(FSlateRenderTransform(FVector2D(0.0f, 26.0f * (1.0f - Eased))));
    }
}

FReply UForestMajorDrawWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,
    const FKeyEvent& Event)
{
    if (Event.GetKey() == EKeys::Escape)
    {
        CloseReveal();
        return FReply::Handled();
    }
    return Super::NativeOnPreviewKeyDown(Geometry, Event);
}

void UForestMajorDrawWidget::CloseReveal()
{
    if (Owner.IsValid()) Owner->CloseMajorDrawReveal();
    else RemoveFromParent();
}
