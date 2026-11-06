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
 FDialogueAICompletion Result;
 TArray<FDialogueMessage> Messages;
 FName PersonaId;
 FString PlayerText;
 FString PendingReply;
 bool bReviewing=false;
 int32 Repairs=0;
 void Send();
 void Finish(bool bSuccess,const FString& Text);
 void Reject();
 UFUNCTION() void Completed(bool bSuccess,const FString& Text);
};
