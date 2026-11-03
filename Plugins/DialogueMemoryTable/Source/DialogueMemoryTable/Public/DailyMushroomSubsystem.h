#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/SaveGame.h"
#include "AlchemyGameplayTypes.h"
#include "DailyMushroomSubsystem.generated.h"

USTRUCT(BlueprintType)
struct DIALOGUEMEMORYTABLE_API FDailyMushroomCardRow
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName MushroomId = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    FString DisplayName;

    UPROPERTY(BlueprintReadOnly)
    int32 Day = 1;

    UPROPERTY(BlueprintReadOnly)
    bool bAvailable = false;

    UPROPERTY(BlueprintReadOnly)
    FName CardId = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    EAlchemyElement Element = EAlchemyElement::Fire;

    UPROPERTY(BlueprintReadOnly)
    int32 PipValue = 1;

    UPROPERTY(BlueprintReadOnly)
    FAlchemyVector Value;

    UPROPERTY(BlueprintReadOnly)
    int32 Weight = 1;

    UPROPERTY(BlueprintReadOnly)
    FString Tags;
};

/**
 * Next-day element consequence row. Conditions and numeric thresholds are
 * intentionally placeholders until the scoring system is defined.
 */
USTRUCT(BlueprintType)
struct DIALOGUEMEMORYTABLE_API FDailyElementModifierRow
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FName RuleId = NAME_None;

    UPROPERTY(BlueprintReadWrite)
    FName ConditionId = NAME_None;

    UPROPERTY(BlueprintReadWrite)
    FString ConditionPlaceholder;

    UPROPERTY(BlueprintReadWrite)
    FString EffectPlaceholder;

    UPROPERTY(BlueprintReadWrite)
    int32 SourceDay = 0;

    UPROPERTY(BlueprintReadWrite)
    int32 TargetDay = 0;

    UPROPERTY(BlueprintReadWrite)
    FName SourceCharacterId = NAME_None;

    UPROPERTY(BlueprintReadWrite)
    EAlchemyElement AffectedElement = EAlchemyElement::Fire;

    UPROPERTY(BlueprintReadWrite)
    bool bBlocked = false;

    UPROPERTY(BlueprintReadWrite)
    float AvailabilityScale = 1.0f;

    UPROPERTY(BlueprintReadWrite)
    int32 MaxAvailableCount = -1;

    UPROPERTY(BlueprintReadWrite)
    float PipValueScale = 1.0f;

    UPROPERTY(BlueprintReadWrite)
    int32 PipValueDelta = 0;

    UPROPERTY(BlueprintReadWrite)
    FString Tags;
};

DECLARE_DELEGATE_RetVal_OneParam(bool, FAlchemyDayCompletionGate, int32);

UCLASS()
class DIALOGUEMEMORYTABLE_API UDailyMushroomSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY()
    int32 CurrentDay = 1;

    UPROPERTY()
    int32 LastConfirmedSynthesisDay = 0;

    UPROPERTY()
    bool bCampaignCompleted = false;

    UPROPERTY()
    TArray<FDailyElementModifierRow> ElementModifiers;
};

/** Shared world-day state and deterministic daily mushroom availability/values. */
UCLASS()
class DIALOGUEMEMORYTABLE_API UDailyMushroomSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintPure, Category = "World|Day")
    int32 GetCurrentDay() const { return CurrentDay; }

    UFUNCTION(BlueprintCallable, Category = "World|Day")
    int32 AdvanceDay();

    UFUNCTION(BlueprintCallable, Category = "World|Day")
    bool ConfirmDryPotSynthesis();

    UFUNCTION(BlueprintPure, Category = "World|Day")
    bool IsCampaignCompleted() const { return Save && Save->bCampaignCompleted; }

    UFUNCTION(BlueprintCallable, Category = "World|Day")
    bool ConfirmSynthesisForDay(int32 ReceiptDay);

    // Native authority supplied by the receipt owner; absent authority cannot advance time.
    FAlchemyDayCompletionGate DayCompletionGate;

    UFUNCTION(BlueprintCallable, Category = "World|Day")
    bool SetCurrentDay(int32 NewDay);

    UFUNCTION(BlueprintPure, Category = "World|Mushrooms")
    TArray<FDailyMushroomCardRow> GetDailyMushrooms() const;

    UFUNCTION(BlueprintPure, Category = "World|Mushrooms")
    bool GetDailyMushroom(FName MushroomId, FDailyMushroomCardRow& OutDefinition) const;

    UFUNCTION(BlueprintPure, Category = "World|Mushrooms")
    bool IsMushroomAvailable(FName MushroomId) const;

    UFUNCTION(BlueprintPure, Category = "World|Mushrooms")
    bool GetMushroomValue(FName MushroomId, FAlchemyVector& OutValue) const;

    UFUNCTION(BlueprintPure, Category = "World|Mushrooms")
    TArray<FDailyElementModifierRow> GetDailyElementModifierRuleTemplates() const;

    UFUNCTION(BlueprintPure, Category = "World|Mushrooms")
    TArray<FDailyElementModifierRow> GetElementModifiersForDay(int32 TargetDay) const;

    UFUNCTION(BlueprintCallable, Category = "World|Mushrooms")
    bool AddElementModifier(const FDailyElementModifierRow& Modifier);

    UFUNCTION(BlueprintCallable, Category = "World|Mushrooms")
    bool ClearElementModifiersForDay(int32 TargetDay);

    UFUNCTION(BlueprintPure, Category = "World|Day")
    FString GetDayStatusText() const;

    UFUNCTION(BlueprintCallable, Category = "World|Day")
    bool ConfigureSaveSlot(const FString& InSlotName, int32 InUserIndex = 0);

private:
    UPROPERTY(Transient)
    TObjectPtr<UDailyMushroomSaveGame> Save;

    UPROPERTY(Transient)
    int32 CurrentDay = 1;

    UPROPERTY(Transient)
    FString SaveSlotName = TEXT("WorldDayState");

    UPROPERTY(Transient)
    int32 SaveUserIndex = 0;

    bool Persist();
    static const TArray<FName>& MushroomCatalog();
    static EAlchemyElement MushroomElementForIndex(int32 Index);
    static int32 DailySeed(int32 Day, int32 Index);
    static FDailyMushroomCardRow MakeDefinition(int32 Day, int32 Index);
};
