#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlchemyGameplayTypes.h"
#include "AlchemyGameplaySubsystem.generated.h"

UCLASS()
class DIALOGUEMEMORYTABLE_API UAlchemyGameplaySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Alchemy|Personas")
    bool RegisterPersonaProfile(const FDialoguePersonaProfile& Profile);
    UFUNCTION(BlueprintPure, Category = "Alchemy|Personas")
    TArray<FName> GetPersonaIds() const;

    UFUNCTION(BlueprintPure, Category = "Alchemy|Personas")
    bool GetPersonaProfile(FName PersonaId, FDialoguePersonaProfile& OutProfile) const;

    UFUNCTION(BlueprintCallable, Category = "Alchemy|Personas")
    int32 EvaluatePersonaTrigger(
        FName PersonaId,
        const FString& PlayerText,
        int32 CurrentFamiliarity,
        int32& OutTriggeredLayer,
        FString& OutReason) const;

    UFUNCTION(BlueprintCallable, Category = "Alchemy|Personas")
    bool AdvancePersonaFromPlayerText(
        FName PersonaId,
        const FString& PlayerText,
        int64 KnownSecretFlags,
        int32 WorldDay,
        bool bSaveImmediately,
        int32& OutNewFamiliarity,
        int32& OutTriggeredLayer,
        FString& OutError) const;

    UFUNCTION(BlueprintPure, Category = "Alchemy|Personas")
    FString BuildPersonaSystemPrompt(
        FName PersonaId,
        int32 Familiarity,
        int32 CommissionOrderIndex,
        const FString& CompactMemoryJson,
        const TArray<FDialogueMessage>& DialogueHistory) const;

    UFUNCTION(BlueprintPure, Category = "Alchemy|Cards")
    TArray<FAlchemyCardDefinition> GetMajorCards() const;

    UFUNCTION(BlueprintPure, Category = "Alchemy|Cards")
    TArray<FAlchemyCardDefinition> GetCourtCards() const;

    UFUNCTION(BlueprintPure, Category = "Alchemy|Commissions")
    TArray<FAlchemyCommissionDefinition> GetCommissions() const;

    UFUNCTION(BlueprintPure, Category = "Alchemy|Commissions")
    bool GetCommission(int32 OrderIndex, FAlchemyCommissionDefinition& OutCommission) const;

    /** Read-only operation preview. Does not read a commission or reveal its target/score. */
    UFUNCTION(BlueprintPure, Category = "Alchemy|Cards")
    bool PreviewAlchemySequence(const TArray<FAlchemyCardSequenceItem>& Sequence, FAlchemyVector& OutVector, int32& OutCost) const;

    UFUNCTION(BlueprintCallable, Category = "Alchemy|Commissions")
    bool EvaluateAlchemySequence(
        int32 OrderIndex,
        const TArray<FAlchemyCardSequenceItem>& Sequence,
        FAlchemyEvaluationResult& OutResult) const;

    UFUNCTION(BlueprintPure, Category = "Alchemy|Commissions")
    FString DescribeAlchemyVector(const FAlchemyVector& Vector) const;

    UFUNCTION(BlueprintPure, Category = "Alchemy|World Consequence")
    FString BuildWorldConsequencePrompt(
        int32 OrderIndex,
        const FAlchemyEvaluationResult& Evaluation,
        const FString& CompactMemoryJson) const;

private:
    static void ComputeAlchemySequence(const TArray<FAlchemyCardSequenceItem>& Sequence, FAlchemyVector& OutVector, int32& OutCost, TSet<FName>& OutPipIds);
    UPROPERTY(Transient) TMap<FName, FDialoguePersonaProfile> CustomPersonas;
    static FDialoguePersonaProfile MakeBlacksmithProfile();
    static FDialoguePersonaProfile MakeOperatorProfile();
    static FDialoguePersonaProfile MakeNovelistProfile();
    static TArray<FAlchemyCardDefinition> MakeMajorCards();
    static TArray<FAlchemyCardDefinition> MakeCourtCards();
    static TArray<FAlchemyCommissionDefinition> MakeCommissions();

    static FString ElementName(EAlchemyElement Element);
    static FString CardTypeName(EAlchemyCardType Type);
    static int32 GetAxis(const FAlchemyVector& Vector, EAlchemyElement Element);
    static void SetAxis(FAlchemyVector& Vector, EAlchemyElement Element, int32 Value);
    static int32 AbsMaxAxis(const FAlchemyVector& Vector);
    static int32 AbsMinNonZeroAxis(const FAlchemyVector& Vector);
    static void AddVectors(FAlchemyVector& Target, const FAlchemyVector& Source, int32 Factor);
    static FAlchemyCardSequenceItem MakePip(FName Id, EAlchemyElement Element, int32 Value);
    static FAlchemyCardDefinition MakeCard(
        FName Id,
        const FString& Name,
        const FString& Latin,
        EAlchemyCardType Type,
        int32 Cost,
        const FString& ShortExpression,
        const FString& Description,
        EAlchemyElement Element = EAlchemyElement::Fire,
        int32 PipValue = 0,
        int32 BracketLength = 0);
};
