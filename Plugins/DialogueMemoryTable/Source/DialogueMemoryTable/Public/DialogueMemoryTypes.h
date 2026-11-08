#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "DialogueMemoryTypes.generated.h"

UENUM(BlueprintType)
enum class EDialogueMemoryAttitude : uint8
{
    Guarded,
    Neutral,
    Warm,
    Hurt,
    Hostile
};

UENUM(BlueprintType)
enum class EDialogueMemoryVisibility : uint8
{
    NpcPrivate,
    PlayerKnown,
    WorldKnown
};

UENUM(BlueprintType)
enum class EDialogueMemoryFactStatus : uint8
{
    Active,
    Resolved,
    Contradicted
};

UENUM(BlueprintType)
enum class EDialogueMemoryAuthority : uint8
{
    Inference,
    NpcReport,
    NpcDisclosure,
    GameEvent
};

UENUM(BlueprintType)
enum class EDialogueMemoryValueType : uint8
{
    Bool,
    Int,
    Float,
    Name,
    Text
};

UENUM(BlueprintType)
enum class EDialogueMemoryLoopKind : uint8
{
    Promise,
    Question,
    Secret,
    Conflict,
    Goal
};

UENUM(BlueprintType)
enum class EDialogueMemoryLoopStatus : uint8
{
    Open,
    Resolved,
    Failed
};

USTRUCT(BlueprintType)
struct DIALOGUEMEMORYTABLE_API FDialogueMemoryNpcStateRow
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName NpcId = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    int32 Familiarity = 1;

    UPROPERTY(BlueprintReadOnly)
    int32 Trust = 0;

    UPROPERTY(BlueprintReadOnly)
    EDialogueMemoryAttitude Attitude = EDialogueMemoryAttitude::Neutral;

    UPROPERTY(BlueprintReadOnly)
    FString RelationshipSummary;

    UPROPERTY(BlueprintReadOnly)
    FString LastExchangeSummary;

    UPROPERTY(BlueprintReadOnly)
    FString LastQuote;

    UPROPERTY(BlueprintReadOnly)
    TArray<FName> TopicTags;

    UPROPERTY(BlueprintReadOnly)
    int64 KnownSecretFlags = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 LastWorldDay = 0;

    UPROPERTY(BlueprintReadOnly)
    int64 LastSessionId = 0;

    UPROPERTY()
    FString MemoryRevision;

    UPROPERTY()
    int32 MemoryDay = 0;
};

USTRUCT(BlueprintType)
struct DIALOGUEMEMORYTABLE_API FDialogueMemoryFactRow
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName FactKey = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    FName NpcId = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    FName Subject = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    FName Predicate = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    FString ObjectText;

    UPROPERTY()
    FName MemoryKind;

    UPROPERTY()
    FString Evidence;

    UPROPERTY(BlueprintReadOnly)
    EDialogueMemoryVisibility Visibility = EDialogueMemoryVisibility::NpcPrivate;

    UPROPERTY(BlueprintReadOnly)
    EDialogueMemoryFactStatus Status = EDialogueMemoryFactStatus::Active;

    UPROPERTY(BlueprintReadOnly)
    int32 Importance = 1;

    UPROPERTY(BlueprintReadOnly)
    int32 Confidence = 0;

    UPROPERTY(BlueprintReadOnly)
    EDialogueMemoryAuthority Authority = EDialogueMemoryAuthority::Inference;

    UPROPERTY(BlueprintReadOnly)
    int64 SourceSessionId = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 LastSeenDay = 0;
};

USTRUCT(BlueprintType)
struct DIALOGUEMEMORYTABLE_API FDialogueMemoryWorldStateRow
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName StateKey = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    EDialogueMemoryValueType ValueType = EDialogueMemoryValueType::Text;

    UPROPERTY(BlueprintReadOnly)
    FString ValueText;

    UPROPERTY(BlueprintReadOnly)
    EDialogueMemoryAuthority Authority = EDialogueMemoryAuthority::Inference;

    UPROPERTY(BlueprintReadOnly)
    int32 Confidence = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 Importance = 1;

    UPROPERTY(BlueprintReadOnly)
    int64 SourceSessionId = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 LastSeenDay = 0;
};

USTRUCT(BlueprintType)
struct DIALOGUEMEMORYTABLE_API FDialogueMemoryOpenLoopRow
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName LoopKey = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    FName NpcId = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    EDialogueMemoryLoopKind Kind = EDialogueMemoryLoopKind::Question;

    UPROPERTY(BlueprintReadOnly)
    FString Summary;

    UPROPERTY()
    FString Evidence;

    UPROPERTY()
    int32 CommissionId = 0;

    UPROPERTY()
    FName SourceRole;

    UPROPERTY(BlueprintReadOnly)
    EDialogueMemoryLoopStatus Status = EDialogueMemoryLoopStatus::Open;

    UPROPERTY(BlueprintReadOnly)
    int32 Priority = 1;

    UPROPERTY(BlueprintReadOnly)
    TArray<FName> TriggerTags;

    UPROPERTY(BlueprintReadOnly)
    int64 SourceSessionId = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 LastSeenDay = 0;
};

USTRUCT(BlueprintType)
struct DIALOGUEMEMORYTABLE_API FDialogueMemoryLedgerSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TArray<FDialogueMemoryNpcStateRow> NpcStates;

    UPROPERTY(BlueprintReadOnly)
    TArray<FDialogueMemoryFactRow> CharacterFacts;

    UPROPERTY(BlueprintReadOnly)
    TArray<FDialogueMemoryWorldStateRow> WorldStates;

    UPROPERTY(BlueprintReadOnly)
    TArray<FDialogueMemoryOpenLoopRow> OpenLoops;
};

USTRUCT(BlueprintType)
struct DIALOGUEMEMORYTABLE_API FDialogueMemoryApplyResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bSuccess = false;

    UPROPERTY(BlueprintReadOnly)
    bool bAlreadyApplied = false;

    UPROPERTY(BlueprintReadOnly)
    FName ErrorCode = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    FString ErrorMessage;

    UPROPERTY(BlueprintReadOnly)
    int32 FactsUpserted = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 WorldRowsUpserted = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 OpenLoopsUpserted = 0;
};

UCLASS(BlueprintType)
class DIALOGUEMEMORYTABLE_API UDialogueMemorySaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY()
    int32 SchemaVersion = 1;

    UPROPERTY()
    TArray<FDialogueMemoryNpcStateRow> NpcStates;

    UPROPERTY()
    TArray<FDialogueMemoryFactRow> CharacterFacts;

    UPROPERTY()
    TArray<FDialogueMemoryWorldStateRow> WorldStates;

    UPROPERTY()
    TArray<FDialogueMemoryOpenLoopRow> OpenLoops;
};
