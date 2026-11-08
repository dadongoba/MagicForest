#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DialogueMemoryTypes.h"
#include "DialogueCompactMemoryPolicy.h"
#include "DialogueMemoryTableSubsystem.generated.h"

UCLASS()
class DIALOGUEMEMORYTABLE_API UDialogueMemoryTableSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Dialogue Memory|Persistence")
    void ConfigureSaveSlot(const FString& InSlotName, int32 InUserIndex);

    UFUNCTION(BlueprintCallable, Category = "Dialogue Memory|Persistence")
    bool LoadLedger(FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "Dialogue Memory|Persistence")
    bool SaveLedger(FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "Dialogue Memory|Patch")
    FDialogueMemoryApplyResult ApplyMemoryPatchJson(
        const FString& PatchJson,
        FName ExpectedNpcId,
        int64 ExpectedSessionId,
        int32 WorldDay,
        bool bSaveImmediately = true);

    UFUNCTION(BlueprintCallable, Category = "Dialogue Memory|Authority")
    bool SetAuthoritativeNpcProgress(
        FName NpcId,
        int32 Familiarity,
        int64 KnownSecretFlags,
        int32 WorldDay,
        bool bSaveImmediately,
        FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "Dialogue Memory|Authority")
    bool UpsertAuthoritativeWorldState(
        FName StateKey,
        EDialogueMemoryValueType ValueType,
        const FString& ValueText,
        int32 Importance,
        int32 WorldDay,
        int64 SourceSessionId,
        bool bSaveImmediately,
        FString& OutError);

    UFUNCTION(BlueprintPure, Category = "Dialogue Memory|Query")
    FDialogueMemoryLedgerSnapshot GetLedgerSnapshot() const;

    UFUNCTION(BlueprintCallable, Category = "Dialogue Memory|Debug")
    bool ExportLedgerToCsv(const FString& Directory, FString& OutError) const;

    bool ApplyCompactTurn(const DialogueCompactMemory::Delta& Delta,FString& OutError);
    bool RetryCompactSaves(FString& OutError);

private:
    bool MigrateCompactLedger(FString& OutError);
    TArray<DialogueCompactMemory::Delta> PendingCompactUpdates;

    UPROPERTY(Transient)
    TObjectPtr<UDialogueMemorySaveGame> Ledger;

    UPROPERTY(Transient)
    FString SaveSlotName = TEXT("DialogueMemoryLedger");

    UPROPERTY(Transient)
    int32 SaveUserIndex = 0;

    void EnsureLedger();
};
