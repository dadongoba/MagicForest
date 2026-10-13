#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlchemyGameplayTypes.h"
#include "ForestTravelNarrativeSubsystem.generated.h"

class UForestTravelNarrativeSubsystem;

/** Frozen facts used to narrate one paid NPC-finder journey. */
USTRUCT(BlueprintType)
struct DEMO3_UE58_API FForestTravelNarrativeInput
{
 GENERATED_BODY()

 UPROPERTY(BlueprintReadWrite) int64 RequestId=0;
 UPROPERTY(BlueprintReadWrite) FString ConsumedItemId;
 UPROPERTY(BlueprintReadWrite) FString ConsumedItemName;
 UPROPERTY(BlueprintReadWrite) FString ConsumedItemDescription;
 UPROPERTY(BlueprintReadWrite) bool bHasElementValue=false;
 UPROPERTY(BlueprintReadWrite) EAlchemyElement Element=EAlchemyElement::Fire;
 UPROPERTY(BlueprintReadWrite) int32 PipValue=0;
 UPROPERTY(BlueprintReadWrite) FAlchemyVector Value;
 UPROPERTY(BlueprintReadWrite) FName TargetNpcId=NAME_None;
 UPROPERTY(BlueprintReadWrite) FString TargetNpcName;
 UPROPERTY(BlueprintReadWrite) bool bUsesPlaceholderValue=true;
 UPROPERTY(BlueprintReadWrite) FString ValueSource=TEXT("unresolved_placeholder");
};

/** Structured output. The emergent name is narrative-only and is not inventory. */
USTRUCT(BlueprintType)
struct DEMO3_UE58_API FForestTravelNarrativeResult
{
 GENERATED_BODY()

 UPROPERTY(BlueprintReadOnly) int64 RequestId=0;
 UPROPERTY(BlueprintReadOnly) FString EmergentItemName;
 UPROPERTY(BlueprintReadOnly) FString ShortNarrative;
 UPROPERTY(BlueprintReadOnly) bool bGeneratedByAI=false;
 UPROPERTY(BlueprintReadOnly) bool bUsedPlaceholderValue=true;
 UPROPERTY(BlueprintReadOnly) FString Error;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FForestTravelNarrativeCompletion,const FForestTravelNarrativeResult&,Result);

UCLASS()
class DEMO3_UE58_API UForestTravelNarrativeRequestContext : public UObject
{
 GENERATED_BODY()
public:
 TWeakObjectPtr<UForestTravelNarrativeSubsystem> Owner;
 FForestTravelNarrativeInput Input;
 FForestTravelNarrativeCompletion Completion;
 UFUNCTION() void Completed(bool bSuccess,const FString& Text);
};

/** Prompt/parse/fallback layer that reuses the shared dialogue AI gateway. */
UCLASS()
class DEMO3_UE58_API UForestTravelNarrativeSubsystem : public UGameInstanceSubsystem
{
 GENERATED_BODY()
public:
 virtual void Initialize(FSubsystemCollectionBase& Collection) override;
 virtual void Deinitialize() override;

 UFUNCTION(BlueprintCallable,Category="NPC Finder|Narrative")
 void GenerateTravelNarrative(const FForestTravelNarrativeInput& Input,FForestTravelNarrativeCompletion Completion);

 UFUNCTION(BlueprintPure,Category="NPC Finder|Narrative")
 FForestTravelNarrativeResult BuildFallback(const FForestTravelNarrativeInput& Input,const FString& Error=TEXT("")) const;

 UFUNCTION(BlueprintPure,Category="NPC Finder|Narrative")
 FForestTravelNarrativeResult GetLastResult() const{return LastResult;}

 void HandleGatewayResponse(UForestTravelNarrativeRequestContext* Context,bool bSuccess,const FString& Text);

private:
 UPROPERTY() TArray<TObjectPtr<UForestTravelNarrativeRequestContext>> Requests;
 UPROPERTY(Transient) FForestTravelNarrativeResult LastResult;
 int64 LatestRequestedId=0;
 static FString ElementName(EAlchemyElement Element);
 static FString CleanJson(FString Text);
 static bool ParseResult(const FString& Text,const FForestTravelNarrativeInput& Input,FForestTravelNarrativeResult& OutResult);
 static FString BuildPrompt(const FForestTravelNarrativeInput& Input);
};
