#pragma once
#include "CoreMinimal.h"

namespace AlchemyPersonaPolicy
{
    constexpr int32 Revision=1;
    DIALOGUEMEMORYTABLE_API FName CanonicalId(FName Id);
    DIALOGUEMEMORYTABLE_API bool ContainsRetiredBinding(FName Id,const FString& Text);
    DIALOGUEMEMORYTABLE_API bool ContainsRetiredWorldBinding(const FString& Text);
    DIALOGUEMEMORYTABLE_API FString ContextText(FName Id,const FString& Text);
    DIALOGUEMEMORYTABLE_API FString CleanReply(FName Id,const FString& Text);
    DIALOGUEMEMORYTABLE_API FString FilterMemoryPatch(FName Id,const FString& Json);
}
