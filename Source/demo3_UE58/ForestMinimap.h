#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ForestMinimap.generated.h"

class ACharacter;
class UTexture2D;

struct FForestMapMarker
{
 FVector Location=FVector::ZeroVector;
 FString Label;
 FLinearColor Color=FLinearColor::White;
};

/** North-up map of DemoMap. The captured terrain is static; actors stay live. */
UCLASS()
class DEMO3_UE58_API UForestMinimapWidget : public UUserWidget
{
 GENERATED_BODY()
public:
 UForestMinimapWidget(const FObjectInitializer& Initializer);
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Map") TObjectPtr<UTexture2D> MapTexture;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Map") FVector2D WorldCenter=FVector2D(45900,45900);
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Map") float WorldWidth=96000;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Map") float LocalWidth=16000;
 UPROPERTY(BlueprintReadOnly,Category="Map") bool bOverview=false;
 UPROPERTY(BlueprintReadOnly,Category="Map") FVector PlayerLocation=FVector::ZeroVector;
 UPROPERTY(BlueprintReadOnly,Category="Map") FVector HomeLocation=FVector::ZeroVector;
 UPROPERTY(BlueprintReadOnly,Category="Map") float PlayerHeading=0;
 UFUNCTION(BlueprintCallable,Category="Map") void ToggleOverview();
 UFUNCTION(BlueprintPure,Category="Map") FVector2D WorldToMapUV(FVector WorldLocation) const;
 UFUNCTION(BlueprintPure,Category="Map") FVector2D GetViewCenter() const;
 void UpdateMap(float DeltaTime);
 TArray<FForestMapMarker> Markers;
 bool bHasHome=false;
protected:
 virtual TSharedRef<SWidget> RebuildWidget() override;
private:
 float ActorScanDelay=0;
 TArray<TWeakObjectPtr<ACharacter>> NPCs;
};
