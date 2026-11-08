#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlchemyGameplaySubsystem.h"
#include "AlchemyDialogueTurn.h"
#include "DialogueAIServiceSubsystem.generated.h"

DECLARE_DYNAMIC_DELEGATE_TwoParams(FDialogueAICompletion, bool, bSuccess, const FString&, Text);

UCLASS()
class DIALOGUEMEMORYTABLE_API UDialogueAIServiceSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Dialogue AI|Gateway")
    void SetGatewayConfig(const FString& InBaseUrl, const FString& InApiKey, const FString& InModel);

    UFUNCTION(BlueprintPure, Category = "Dialogue AI|Gateway")
    FString GetGatewayBaseUrl() const { return GatewayBaseUrl; }

    UFUNCTION(BlueprintPure, Category = "Dialogue AI|Gateway")
    FString GetGatewayModel() const { return GatewayModel; }

    UFUNCTION(BlueprintPure, Category = "Dialogue AI|Gateway")
    bool HasApiKey() const { return !ApiKey.IsEmpty(); }
    // NPC callers may provide an already assembled, state-bound prompt.
    void RequestReviewedNpcMessages(FName PersonaId,const TArray<FDialogueMessage>& Messages,FDialogueAICompletion Callback,const FString& CommissionFacts=FString(),const FString& PurposeFallback=FString(),const TArray<FName>& Allowed={});
    void RequestScopedCompletion(const TArray<FDialogueMessage>& Messages,float Temperature,int32 MaxTokens,FDialogueAICompletion Callback,bool bJson,const TSharedPtr<FDialogueRequestScope>& Scope);
    void RequestNpcTurn(FName PersonaId,const TArray<FDialogueMessage>& Messages,const FString& Catalog,const TArray<FName>& Allowed,FName FallbackFact,const FString& FallbackText,FAlchemyNpcReplyCompletion Callback,const TSharedPtr<FDialogueRequestScope>& Scope,const FString& ProposedPlayerMemory=FString());
    // Own a stable reference: finishing a gate can synchronously reset its caller's scope.
    void CancelNpcTurns(TSharedPtr<FDialogueRequestScope> Scope);

    UFUNCTION(BlueprintCallable, Category = "Dialogue AI|Chat")
    void RequestChatCompletion(
        const TArray<FDialogueMessage>& Messages,
        float Temperature,
        int32 MaxTokens,
        FDialogueAICompletion Callback);

    UFUNCTION(BlueprintCallable, Category = "Dialogue AI|Chat")
    void RequestNpcDialogue(
        FName PersonaId,
        int32 Familiarity,
        int32 CommissionOrderIndex,
        const FString& CompactMemoryJson,
        const TArray<FDialogueMessage>& DialogueHistory,
        FDialogueAICompletion Callback);

    UFUNCTION(BlueprintCallable, Category = "Dialogue AI|World")
    void RequestWorldConsequence(
        int32 CommissionOrderIndex,
        const FAlchemyEvaluationResult& Evaluation,
        const FString& CompactMemoryJson,
        FDialogueAICompletion Callback);

private:
    friend class UAlchemyDialogueReplyGate;
    UPROPERTY(Transient) TArray<TObjectPtr<class UAlchemyDialogueReplyGate>> NpcReplyGates;
    void ReleaseNpcReplyGate(class UAlchemyDialogueReplyGate* Gate);
    UPROPERTY(Transient)
    FString GatewayBaseUrl;

    UPROPERTY(Transient)
    FString ApiKey;

    UPROPERTY(Transient)
    FString GatewayModel;

    void CompleteRequest(
        const TSharedPtr<class IHttpResponse, ESPMode::ThreadSafe>& Response,
        bool bWasSuccessful,
        FDialogueAICompletion Callback) const;
};
