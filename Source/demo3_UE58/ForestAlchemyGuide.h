#pragma once
#include "CoreMinimal.h"
class UAlchemyGameplaySubsystem;
struct FForestMajorGuide
{
 FString HowToUse,Reverse,Caution,Example;
};
/** Read-only help. Examples use the same target-free operation evaluator as a brew. */
namespace ForestAlchemyGuide
{
 bool Major(const UAlchemyGameplaySubsystem* Gameplay,FName Id,FForestMajorGuide& Out);
 FString Introduction();
 FString Notes();
 FString VectorText(int32 Fire,int32 Wind,int32 Water);
}
