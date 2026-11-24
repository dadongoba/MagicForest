#pragma once
#include "CoreMinimal.h"
#include "ForestWorldOutcome.h"
namespace ForestTerminationNotice
{
 FString GeneratePrompt(const FForestWorldOutcome& Outcome,const FAlchemyEvaluationResult& Result);
 FString ReviewPrompt(const FForestWorldOutcome& Outcome,const FAlchemyEvaluationResult& Result,const FString& Candidate);
 FString Fallback(const FForestWorldOutcome& Outcome);
 bool AcceptReview(const FString& Review);
}
