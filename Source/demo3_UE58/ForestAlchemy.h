#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameFramework/SaveGame.h"
#include "Blueprint/UserWidget.h"
#include "AlchemyGameplayTypes.h"
#include "ForestAlchemy.generated.h"
class UInputComponent;class APlayerController;class UStaticMeshComponent;class UForestAlchemy;
class STextBlock;class SVerticalBox;

/** Future restrictions are data, not an LLM decision. Empty allowlists mean unrestricted. */
UCLASS(Config=ForestAlchemy,DefaultConfig)
class DEMO3_UE58_API UForestAlchemyRules : public UObject
{
 GENERATED_BODY()
public:
 UPROPERTY(Config,EditAnywhere,Category="Daily draw") int32 MaxDailyDraws=0;
 UPROPERTY(Config,EditAnywhere,Category="Daily draw") TArray<FName> AllowedMajorIds;
 UPROPERTY(Config,EditAnywhere,Category="Daily draw") TArray<FName> RequiredBottleTags;
 UPROPERTY(Config,EditAnywhere,Category="Daily draw") float InteractionDistance=280;
};
USTRUCT(BlueprintType)
struct FForestDailyMajor
{
 GENERATED_BODY()
 UPROPERTY(BlueprintReadOnly) FString InstanceId;
 UPROPERTY(BlueprintReadOnly) FName CardId;
 UPROPERTY(BlueprintReadOnly) FString SourceBottle;
 UPROPERTY(BlueprintReadOnly) int32 Day=1;
};
USTRUCT(BlueprintType)
struct FForestBrewRecord
{
 GENERATED_BODY()
 UPROPERTY(BlueprintReadOnly) int32 Day=1;
 UPROPERTY(BlueprintReadOnly) int32 CommissionId=0;
 UPROPERTY(BlueprintReadOnly) FAlchemyEvaluationResult Result;
 UPROPERTY(BlueprintReadOnly) TArray<FAlchemyCardSequenceItem> Sequence;
};
UCLASS()
class DEMO3_UE58_API UForestAlchemySave : public USaveGame
{
 GENERATED_BODY()
public:
 UPROPERTY() int32 Day=0;
 UPROPERTY() int32 Seed=0;
 UPROPERTY() TArray<FString> ClaimedBottles;
 UPROPERTY() TArray<FForestDailyMajor> Majors;
 UPROPERTY() TArray<FForestBrewRecord> Brews;
 UPROPERTY() int32 PendingAdvanceFromDay=0;
};
struct FForestBrewChoice
{
 FString Key,Name,Description;
 FAlchemyCardSequenceItem Card;
};
UCLASS()
class DEMO3_UE58_API UForestBrewWidget : public UUserWidget
{
 GENERATED_BODY()
public:
 TWeakObjectPtr<UForestAlchemy> Owner;
 void Refresh();
protected:
 virtual TSharedRef<SWidget> RebuildWidget() override;
 virtual FReply NativeOnPreviewKeyDown(const FGeometry&,const FKeyEvent&) override;
private:
 TSharedPtr<SVerticalBox> Body;
};
UCLASS()
class DEMO3_UE58_API UForestArcanaNotice : public UUserWidget
{
 GENERATED_BODY()
public:
 TSharedPtr<STextBlock> Text;
protected:
 virtual TSharedRef<SWidget> RebuildWidget() override;
};
/** Daily card inventory, bottle interaction, indoor day start and the evening workbench. */
UCLASS()
class DEMO3_UE58_API UForestAlchemy : public UTickableWorldSubsystem
{
 GENERATED_BODY()
public:
 virtual void Tick(float DeltaTime) override;
 virtual void Deinitialize() override;
 virtual TStatId GetStatId() const override;
 virtual bool IsTickableInEditor() const override{return false;}
 UFUNCTION(BlueprintPure,Category="Alchemy|Daily") TArray<FForestDailyMajor> GetTodayMajors() const;
 UFUNCTION(BlueprintPure,Category="Alchemy|Daily") bool GetMajorDefinition(FName Id,FAlchemyCardDefinition& Out) const;
 UFUNCTION(BlueprintPure,Category="Alchemy|Daily") int32 GetDay() const;
 UFUNCTION(BlueprintPure,Category="Alchemy|Brews") TArray<FForestBrewRecord> GetBrewRecords() const;
 UFUNCTION(BlueprintCallable,Category="Alchemy|Daily") bool TakeBottle(AActor* Bottle);
 UFUNCTION(BlueprintCallable,Category="Alchemy|Night") void OpenWorkbench();
 UFUNCTION(BlueprintCallable,Category="Alchemy|Night") void CloseWorkbench();
 bool IsWorkbenchOpen() const;
 TArray<FForestBrewChoice> Choices() const;
 TArray<FAlchemyCommissionDefinition> Commissions() const;
 bool CurrentCommission(FAlchemyCommissionDefinition& Out) const;
 void ChooseCommission(int32 Id);
 void AddChoice(const FForestBrewChoice& Choice);
 void RemoveSlot();void MoveSlot(int32 Direction);void ClearSequence();
 bool Evaluate(FAlchemyEvaluationResult& Out) const;
 void ConfirmBrew();
 void RefreshUI();
 TArray<FForestBrewChoice> Sequence;
 int32 SelectedSlot=INDEX_NONE;
 FString Status;
 bool bConfirming=false;
private:
 UPROPERTY() TObjectPtr<UForestAlchemySave> Save;
 UPROPERTY() TObjectPtr<UInputComponent> Input;
 UPROPERTY() TObjectPtr<UForestBrewWidget> Workbench;
 UPROPERTY() TObjectPtr<UForestArcanaNotice> Notice;
 TWeakObjectPtr<APlayerController> Player;
 struct FBottle {TWeakObjectPtr<AActor> Actor;TWeakObjectPtr<UStaticMeshComponent> Mesh;FString Key;FTransform Home;bool bHidden=false,bCollision=true;};
 TArray<FBottle> Bottles;
 TArray<TWeakObjectPtr<AActor>> Cauldrons;
 TWeakObjectPtr<AActor> Focus;
 bool bReady=false,bNeedsCabinSpawn=true,bOldCursor=false;
 int32 CommissionId=0;
 float ScanTime=1,SpawnTime=0,ToastUntil=0;
 FString Toast;
 void Setup();bool Persist();bool SyncDay();void Discover();void StartInCabin();void Interact();
 bool OtherMenuOpen() const;
 AActor* FindFocus() const;
 TMap<FString,int32> ReadBag() const;
 bool WriteBag(const TMap<FString,int32>& Bag);
 void Tell(const FString& Message);
};
