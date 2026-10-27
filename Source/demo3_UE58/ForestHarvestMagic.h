#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameFramework/SaveGame.h"
#include "Blueprint/UserWidget.h"
#include "AlchemyGameplayTypes.h"
#include "ForestHarvestMagic.generated.h"

class UForestHarvestMagic;
class UInputComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UNiagaraComponent;
class STextBlock;
class SVerticalBox;
class SWrapBox;

USTRUCT(BlueprintType)
struct DEMO3_UE58_API FForestMagicItem
{
 GENERATED_BODY()
 UPROPERTY(BlueprintReadOnly) FString Id;
 UPROPERTY(BlueprintReadOnly) FString InventoryKey;
 UPROPERTY(BlueprintReadOnly) FString Name;
 UPROPERTY(BlueprintReadOnly) FString Description;
 UPROPERTY(BlueprintReadOnly) FString SourceMushroom;
 UPROPERTY(BlueprintReadOnly) EAlchemyElement Element=EAlchemyElement::Fire;
 UPROPERTY(BlueprintReadOnly) int32 PipValue=1;
 UPROPERTY(BlueprintReadOnly) int32 HarvestDay=1;
 UPROPERTY(BlueprintReadOnly) FAlchemyVector Value;
 UPROPERTY(BlueprintReadOnly) bool bGeneratedByAI=false;
};
UCLASS()
class DEMO3_UE58_API UForestMagicItemSave : public USaveGame
{
 GENERATED_BODY()
public:
 UPROPERTY() TArray<FForestMagicItem> Items;
};
UCLASS()
class DEMO3_UE58_API UForestHarvestRequest : public UObject
{
 GENERATED_BODY()
public:
 TWeakObjectPtr<UForestHarvestMagic> Owner;
 FString ItemId;
 UFUNCTION() void Completed(bool bSuccess,const FString& Text);
};
UCLASS()
class DEMO3_UE58_API UForestHarvestNoticeWidget : public UUserWidget
{
 GENERATED_BODY()
public:
 TSharedPtr<STextBlock> Text;
protected:
 virtual TSharedRef<SWidget> RebuildWidget() override;
};
UCLASS()
class DEMO3_UE58_API UForestFieldHUDWidget : public UUserWidget
{
 GENERATED_BODY()
public:
 TSharedPtr<STextBlock> Counts;
 TSharedPtr<STextBlock> Action;
protected:
 virtual TSharedRef<SWidget> RebuildWidget() override;
};
UCLASS()
class DEMO3_UE58_API UForestMagicInventoryWidget : public UUserWidget
{
 GENERATED_BODY()
public:
 TWeakObjectPtr<UForestHarvestMagic> Owner;
 virtual TSharedRef<SWidget> RebuildWidget() override;
 virtual FReply NativeOnPreviewKeyDown(const FGeometry&,const FKeyEvent&) override;
 void Refresh();
 void SelectMajor(const FString& InstanceId);
 void Select(const FString& Key);
 UFUNCTION() void Close();
private:
 TSharedPtr<SWrapBox> List;
 TSharedPtr<STextBlock> Detail;
 FString Selected;
 FString SelectedMajor;
 bool bMajorsTab=false;
};

/** Bridges confirmed legacy Blueprint harvests to named, immutable magic items. */
UCLASS()
class DEMO3_UE58_API UForestHarvestMagic : public UTickableWorldSubsystem
{
 GENERATED_BODY()
public:
 virtual void Tick(float DeltaTime) override;
 virtual TStatId GetStatId() const override;
 virtual bool IsTickableInEditor() const override {return false;}
 virtual void Deinitialize() override;
 UFUNCTION(BlueprintCallable) void ToggleInventory();
 UFUNCTION(BlueprintPure) bool FindItem(const FString& Key,FForestMagicItem& Item) const;
 TMap<FString,int32> ReadBag() const;
 bool IsInventoryOpen() const{return Inventory && Inventory->IsInViewport();}
 void HandleResponse(UForestHarvestRequest*,bool,const FString&);
 static FString ElementText(EAlchemyElement Element);
private:
 UPROPERTY() TObjectPtr<UForestMagicItemSave> Ledger;
 UPROPERTY() TArray<TObjectPtr<UForestHarvestRequest>> Requests;
 UPROPERTY() TObjectPtr<UForestMagicInventoryWidget> Inventory;
 UPROPERTY() TObjectPtr<UUserWidget> NoticeWidget;
 UPROPERTY() TObjectPtr<UForestFieldHUDWidget> FieldHUD;
 TSharedPtr<STextBlock> Notice;
 UPROPERTY() TObjectPtr<UInputComponent> Input;
 TWeakObjectPtr<class APlayerController> Player;
 TWeakObjectPtr<class APawn> BagOwner;
 TMap<FString,int32> Previous;
 bool bReady=false;
 bool bOldCursor=false;
 float NoticeUntil=0;
 float PollTime=0;
 float FieldUITime=0;
 struct FVisual
 {
  TWeakObjectPtr<AActor> Actor;
  TWeakObjectPtr<UStaticMeshComponent> Mushroom;
  TWeakObjectPtr<USkeletalMeshComponent> Book;
  TWeakObjectPtr<UNiagaraComponent> Portal;
  FVector Origin;
  FVector Scale;
  EAlchemyElement Element;
  float Age=0;
  bool bBurst=false;
 };
 TArray<FVisual> Visuals;
 TMap<TWeakObjectPtr<UStaticMeshComponent>,bool> MushroomVisibility;
 TMap<FString,TWeakObjectPtr<UStaticMeshComponent>> JustHarvested;
 void Setup();
 void RefreshFieldUI();
 void Harvest(const FString& Source,int32 Count);
 void Adjust(const FString& Key,int32 Delta);
 void RefreshLegacyHUD();
 void Persist();
 void ShowResult(const FForestMagicItem& Item);
 void BeginVisual(const FString& Source,EAlchemyElement Element);
 void UpdateVisuals(float DeltaTime);
 static FString Prompt(const FForestMagicItem& Item);
};
