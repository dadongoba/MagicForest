#pragma once
#include "CoreMinimal.h"
#include "AlchemyDialogueTurn.generated.h"

class IHttpRequest;
struct DIALOGUEMEMORYTABLE_API FDialogueRequestScope
{
 bool bCancelled=false;
 double Deadline=0;
 int32 Calls=0;
 int32 MaxCalls=6;
 TArray<TSharedPtr<IHttpRequest,ESPMode::ThreadSafe>> Requests;
 bool IsActive() const;
 void Cancel();
};

USTRUCT()
struct DIALOGUEMEMORYTABLE_API FAlchemyDialogueTurnResult
{
 GENERATED_BODY()
 UPROPERTY() bool bSuccess=false;
 UPROPERTY() bool bFallback=false;
 UPROPERTY() FString Text;
 UPROPERTY() FString Error;
 UPROPERTY() FString ReviewCode;
 UPROPERTY() TArray<FName> DeliveredFacts;
 UPROPERTY() FString MemoryPatchJson;
};
DECLARE_DYNAMIC_DELEGATE_OneParam(FAlchemyNpcReplyCompletion,const FAlchemyDialogueTurnResult&,Reply);

struct FAlchemyDialogueFact
{
 FName Id;
 FString Topic;
 FString Text;
 int32 MinimumLayer=1;
 bool bCharacter=false;
};
