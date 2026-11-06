#pragma once
#include "CoreMinimal.h"
#include "AlchemyGameplayTypes.h"
#include "AlchemyDialogueTurn.h"

// Narrative knowledge is optional and reviewed separately from gameplay math.
// Target values never leave this local boundary in a dialogue request.
namespace AlchemyDialogueKnowledge
{
 constexpr int32 Revision=3;
 DIALOGUEMEMORYTABLE_API bool ApplyReviewedDefinitions(TArray<FAlchemyCommissionDefinition>& Commissions);
 DIALOGUEMEMORYTABLE_API bool HasNumericDisclosure(const FString& Text);
 DIALOGUEMEMORYTABLE_API bool IsSafeReply(const FString& Text);
 DIALOGUEMEMORYTABLE_API bool IsSafeReplyForPlayer(const FString& Text,const FString& PlayerText);
 DIALOGUEMEMORYTABLE_API FString SafeContext(const FString& Text);
 DIALOGUEMEMORYTABLE_API FString SafeMemoryJson(const FString& Json);
 DIALOGUEMEMORYTABLE_API bool IsMemoryFieldAllowed(const FString& Key);
 DIALOGUEMEMORYTABLE_API FString Rules();
 DIALOGUEMEMORYTABLE_API int32 TopicBit(const FString& Topic);
 DIALOGUEMEMORYTABLE_API int32 LimitTopics(int32 Mask);
 DIALOGUEMEMORYTABLE_API bool HasReviewedEntry(const FAlchemyCommissionDefinition& Commission);
 DIALOGUEMEMORYTABLE_API FString Intro(const FAlchemyCommissionDefinition& Commission);
 DIALOGUEMEMORYTABLE_API FString Guidance(const FAlchemyCommissionDefinition& Commission,int32 Topics,int32 AlreadyDisclosed);
 DIALOGUEMEMORYTABLE_API TArray<FAlchemyDialogueFact> Facts(const FAlchemyCommissionDefinition& Commission,int32 Layer);
 DIALOGUEMEMORYTABLE_API FString Catalog(const FAlchemyCommissionDefinition& Commission,int32 Layer);
 DIALOGUEMEMORYTABLE_API TArray<FName> AllowedFacts(const FAlchemyCommissionDefinition& Commission,int32 Layer,const TArray<FName>& Selected,const TArray<FName>& Delivered);
 DIALOGUEMEMORYTABLE_API FString FactGuidance(const FAlchemyCommissionDefinition& Commission,int32 Layer,const TArray<FName>& Allowed,const TArray<FName>& Delivered);
 DIALOGUEMEMORYTABLE_API FString FactText(const FAlchemyCommissionDefinition& Commission,int32 Layer,FName Id);
 DIALOGUEMEMORYTABLE_API FString AnalysisRules();
 DIALOGUEMEMORYTABLE_API FString ReviewRules();
 DIALOGUEMEMORYTABLE_API FString GenerationRules();
 DIALOGUEMEMORYTABLE_API FString Fingerprint();
 DIALOGUEMEMORYTABLE_API FString Reveal(const FAlchemyCommissionDefinition& Commission);
}
