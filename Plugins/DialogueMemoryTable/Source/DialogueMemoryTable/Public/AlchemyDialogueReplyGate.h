#pragma once
#include "CoreMinimal.h"
#include "DialogueAIServiceSubsystem.h"
#include "AlchemyDialogueReplyGate.generated.h"

// NPC-only wrapper. Other text-generation features keep their numeric content.
UCLASS()
class DIALOGUEMEMORYTABLE_API UAlchemyDialogueReplyGate : public UObject
{
 GENERATED_BODY()
public:
 TWeakObjectPtr<UDialogueAIServiceSubsystem> Service;
 FDialogueAICompletion LegacyResult;
 FAlchemyNpcReplyCompletion Result;
 TArray<FDialogueMessage> Messages;
 FName PersonaId;
 FString PlayerText;
 FString PendingReply;
 FString MemoryPatchJson;
 FString ProposedPlayerMemory;
 FString CommissionFacts;
 FString PurposeFallback;
 FName FallbackFact;
 TArray<FName> AllowedFacts,ClaimedFacts;
 TSharedPtr<FDialogueRequestScope> Scope;
 FString LastReviewCode,LastReviewReason,LastReviewSpan;
 bool bFinished=false,bStructured=false;
 bool bReviewing=false;
 int32 Repairs=0;
 int32 FormatRepairs=0;
 void Send();
 void SendReview();
 void Finish(bool bSuccess,const FString& Text,bool bFallback=false,const TArray<FName>& Delivered={});
 void Reject();
 UFUNCTION() void Completed(bool bSuccess,const FString& Text);
};
