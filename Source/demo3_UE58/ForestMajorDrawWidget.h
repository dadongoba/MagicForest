#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ForestMajorDrawWidget.generated.h"

class UForestAlchemy;
class SBox;
class SVerticalBox;

/** Full-card reveal after a successful daily draw; the same art is used by the bag and codex. */
UCLASS()
class DEMO3_UE58_API UForestMajorDrawWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    TWeakObjectPtr<UForestAlchemy> Owner;

    void Configure(FName CardId, const FString& Name, const FString& ShortEffect,
        int32 Cost, int32 Day, int32 Drawn, int32 Limit, bool bFirstDraw);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

private:
    void Refresh();
    void CloseReveal();

    TSharedPtr<SBox> Frame;
    TSharedPtr<SBox> CardMotion;
    TSharedPtr<SVerticalBox> Body;
    FName ShownCardId = NAME_None;
    FString ShownName;
    FString ShownEffect;
    int32 ShownCost = 0;
    int32 ShownDay = 1;
    int32 ShownDrawn = 0;
    int32 ShownLimit = 3;
    bool bShowFirstDrawGuide = false;
    bool bCompact = false;
    float RevealAge = 0.0f;
};
