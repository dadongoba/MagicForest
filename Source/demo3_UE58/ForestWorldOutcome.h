#pragma once
#include "CoreMinimal.h"
#include "AlchemyGameplayTypes.h"
#include "ForestOutcomePolicy.h"
#include "ForestWorldOutcome.generated.h"

/** Immutable authoritative facts, not an LLM summary or a second dialogue transcript. */
USTRUCT()
struct FForestWorldOutcome
{
 GENERATED_BODY()
 UPROPERTY() int32 Revision=0;
 UPROPERTY() int32 Grade=0;
 UPROPERTY() int32 PreviousState=0;
 UPROPERTY() int32 NextState=0;
 UPROPERTY() bool bDisconnected=false;
 UPROPERTY() FName Persona;
 UPROPERTY() FName Region;
 UPROPERTY() FName Visual;
 UPROPERTY() float AvailabilityA=1;
 UPROPERTY() float AvailabilityB=1;
 UPROPERTY() TArray<FName> Tags;
 UPROPERTY() FString TaskFact;
 UPROPERTY() FString NpcFact;
 UPROPERTY() FString Continuity;
 UPROPERTY() FString Fallback;
};
namespace ForestOutcome
{
 bool Ready(FString& Error);
 bool Rate(FAlchemyEvaluationResult& Result,int32& Grade);
 bool Build(int32 CommissionId,const FAlchemyEvaluationResult& Result,int32 PreviousState,FForestWorldOutcome& Out);
 FName Canonical(FName Id);
 FName PersonaForCommission(int32 Id);
 FString GradeName(int32 Grade);
 FString StateName(int32 State);
 FString Summary(const FForestWorldOutcome& Outcome,bool FinalNight,const FString& PlayerNotice=FString());
 FString Memory(const FForestWorldOutcome& Outcome);
}
