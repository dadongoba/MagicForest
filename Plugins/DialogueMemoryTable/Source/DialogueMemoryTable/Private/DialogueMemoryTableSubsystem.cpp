#include "DialogueMemoryTableSubsystem.h"
#include "AlchemyGameplaySubsystem.h"
#include "DialogueCompactMemory.h"

#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace DialogueMemoryTable
{
    constexpr int32 MaxFactsPerNpc = 24;
    constexpr int32 MaxWorldRows = 64;
    constexpr int32 MaxOpenLoopsPerNpc = 8;

    FDialogueMemoryApplyResult ErrorResult(const FName Code, const FString& Message)
    {
        FDialogueMemoryApplyResult Result;
        Result.ErrorCode = Code;
        Result.ErrorMessage = Message;
        return Result;
    }

    bool HasOnlyFields(
        const TSharedPtr<FJsonObject>& Object,
        const TArray<FString>& AllowedFields,
        FString& OutError)
    {
        if (!Object.IsValid())
        {
            OutError = TEXT("Expected a JSON object.");
            return false;
        }

        for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Object->Values)
        {
            if (!AllowedFields.Contains(Pair.Key))
            {
                OutError = FString::Printf(TEXT("Unexpected field: %s"), *Pair.Key);
                return false;
            }
        }

        return true;
    }

    bool TryRequiredString(
        const TSharedPtr<FJsonObject>& Object,
        const TCHAR* Field,
        const int32 MaxLength,
        FString& OutValue,
        FString& OutError)
    {
        if (!Object.IsValid() || !Object->TryGetStringField(Field, OutValue))
        {
            OutError = FString::Printf(TEXT("Missing or invalid string field: %s"), Field);
            return false;
        }

        if (OutValue.Len() > MaxLength)
        {
            OutError = FString::Printf(TEXT("Field %s exceeds %d characters"), Field, MaxLength);
            return false;
        }

        return true;
    }

    bool TryOptionalString(
        const TSharedPtr<FJsonObject>& Object,
        const TCHAR* Field,
        const int32 MaxLength,
        FString& OutValue,
        FString& OutError)
    {
        OutValue.Reset();
        if (!Object.IsValid() || !Object->HasField(Field))
        {
            return true;
        }

        return TryRequiredString(Object, Field, MaxLength, OutValue, OutError);
    }

    bool TryRequiredInteger(
        const TSharedPtr<FJsonObject>& Object,
        const TCHAR* Field,
        int64& OutValue,
        FString& OutError)
    {
        double Number = 0.0;
        if (!Object.IsValid() || !Object->TryGetNumberField(Field, Number))
        {
            OutError = FString::Printf(TEXT("Missing or invalid integer field: %s"), Field);
            return false;
        }

        const int64 Integer = static_cast<int64>(Number);
        if (!FMath::IsNearlyEqual(Number, static_cast<double>(Integer), 0.000001))
        {
            OutError = FString::Printf(TEXT("Field %s must be an integer"), Field);
            return false;
        }

        OutValue = Integer;
        return true;
    }

    bool TryOptionalInteger(
        const TSharedPtr<FJsonObject>& Object,
        const TCHAR* Field,
        const int64 DefaultValue,
        int64& OutValue,
        FString& OutError)
    {
        if (!Object.IsValid() || !Object->HasField(Field))
        {
            OutValue = DefaultValue;
            return true;
        }

        return TryRequiredInteger(Object, Field, OutValue, OutError);
    }

    bool TryRequiredObject(
        const TSharedPtr<FJsonObject>& Object,
        const TCHAR* Field,
        TSharedPtr<FJsonObject>& OutObject,
        FString& OutError)
    {
        if (!Object.IsValid() || !Object->HasTypedField<EJson::Object>(Field))
        {
            OutError = FString::Printf(TEXT("Missing or invalid object field: %s"), Field);
            return false;
        }

        OutObject = Object->GetObjectField(Field);
        return OutObject.IsValid();
    }

    bool TryRequiredArray(
        const TSharedPtr<FJsonObject>& Object,
        const TCHAR* Field,
        const TArray<TSharedPtr<FJsonValue>>*& OutArray,
        FString& OutError)
    {
        OutArray = nullptr;
        if (!Object.IsValid() || !Object->TryGetArrayField(Field, OutArray) || OutArray == nullptr)
        {
            OutError = FString::Printf(TEXT("Missing or invalid array field: %s"), Field);
            return false;
        }

        return true;
    }

    bool IsStableKey(const FString& Value)
    {
        if (Value.IsEmpty() || Value.Len() > 64)
        {
            return false;
        }

        for (const TCHAR Character : Value)
        {
            const bool bAllowed =
                (Character >= TEXT('a') && Character <= TEXT('z')) ||
                (Character >= TEXT('0') && Character <= TEXT('9')) ||
                Character == TEXT('.') ||
                Character == TEXT('_') ||
                Character == TEXT('-');

            if (!bAllowed)
            {
                return false;
            }
        }

        return true;
    }

    bool TryNameArray(
        const TSharedPtr<FJsonObject>& Object,
        const TCHAR* Field,
        const int32 MaxItems,
        const int32 MaxItemLength,
        TArray<FName>& OutValues,
        FString& OutError)
    {
        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (!TryRequiredArray(Object, Field, Values, OutError))
        {
            return false;
        }

        if (Values->Num() > MaxItems)
        {
            OutError = FString::Printf(TEXT("Array %s exceeds %d items"), Field, MaxItems);
            return false;
        }

        TSet<FName> Seen;
        OutValues.Reset();
        for (const TSharedPtr<FJsonValue>& Value : *Values)
        {
            if (!Value.IsValid() || Value->Type != EJson::String)
            {
                OutError = FString::Printf(TEXT("Array %s must contain strings"), Field);
                return false;
            }

            const FString Text = Value->AsString();
            if (Text.IsEmpty() || Text.Len() > MaxItemLength)
            {
                OutError = FString::Printf(TEXT("Array %s contains an invalid string"), Field);
                return false;
            }

            const FName Name(*Text);
            if (!Seen.Contains(Name))
            {
                Seen.Add(Name);
                OutValues.Add(Name);
            }
        }

        return true;
    }

    bool ParseAttitude(const FString& Value, EDialogueMemoryAttitude& Out)
    {
        if (Value == TEXT("guarded")) { Out = EDialogueMemoryAttitude::Guarded; return true; }
        if (Value == TEXT("neutral")) { Out = EDialogueMemoryAttitude::Neutral; return true; }
        if (Value == TEXT("warm")) { Out = EDialogueMemoryAttitude::Warm; return true; }
        if (Value == TEXT("hurt")) { Out = EDialogueMemoryAttitude::Hurt; return true; }
        if (Value == TEXT("hostile")) { Out = EDialogueMemoryAttitude::Hostile; return true; }
        return false;
    }

    bool ParseVisibility(const FString& Value, EDialogueMemoryVisibility& Out)
    {
        if (Value == TEXT("npc_private")) { Out = EDialogueMemoryVisibility::NpcPrivate; return true; }
        if (Value == TEXT("player_known")) { Out = EDialogueMemoryVisibility::PlayerKnown; return true; }
        if (Value == TEXT("world_known")) { Out = EDialogueMemoryVisibility::WorldKnown; return true; }
        return false;
    }

    bool ParseFactStatus(const FString& Value, EDialogueMemoryFactStatus& Out)
    {
        if (Value == TEXT("active")) { Out = EDialogueMemoryFactStatus::Active; return true; }
        if (Value == TEXT("resolved")) { Out = EDialogueMemoryFactStatus::Resolved; return true; }
        if (Value == TEXT("contradicted")) { Out = EDialogueMemoryFactStatus::Contradicted; return true; }
        return false;
    }

    bool ParseAiAuthority(const FString& Value, EDialogueMemoryAuthority& Out)
    {
        if (Value == TEXT("npc_disclosure")) { Out = EDialogueMemoryAuthority::NpcDisclosure; return true; }
        if (Value == TEXT("npc_report")) { Out = EDialogueMemoryAuthority::NpcReport; return true; }
        if (Value == TEXT("inference")) { Out = EDialogueMemoryAuthority::Inference; return true; }
        return false;
    }

    bool ParseValueType(const FString& Value, EDialogueMemoryValueType& Out)
    {
        if (Value == TEXT("bool")) { Out = EDialogueMemoryValueType::Bool; return true; }
        if (Value == TEXT("int")) { Out = EDialogueMemoryValueType::Int; return true; }
        if (Value == TEXT("float")) { Out = EDialogueMemoryValueType::Float; return true; }
        if (Value == TEXT("name")) { Out = EDialogueMemoryValueType::Name; return true; }
        if (Value == TEXT("text")) { Out = EDialogueMemoryValueType::Text; return true; }
        return false;
    }

    bool ParseLoopKind(const FString& Value, EDialogueMemoryLoopKind& Out)
    {
        if (Value == TEXT("promise")) { Out = EDialogueMemoryLoopKind::Promise; return true; }
        if (Value == TEXT("question")) { Out = EDialogueMemoryLoopKind::Question; return true; }
        if (Value == TEXT("secret")) { Out = EDialogueMemoryLoopKind::Secret; return true; }
        if (Value == TEXT("conflict")) { Out = EDialogueMemoryLoopKind::Conflict; return true; }
        if (Value == TEXT("goal")) { Out = EDialogueMemoryLoopKind::Goal; return true; }
        return false;
    }

    bool ParseLoopStatus(const FString& Value, EDialogueMemoryLoopStatus& Out)
    {
        if (Value == TEXT("open")) { Out = EDialogueMemoryLoopStatus::Open; return true; }
        if (Value == TEXT("resolved")) { Out = EDialogueMemoryLoopStatus::Resolved; return true; }
        if (Value == TEXT("failed")) { Out = EDialogueMemoryLoopStatus::Failed; return true; }
        return false;
    }

    int32 AuthorityRank(const EDialogueMemoryAuthority Authority)
    {
        switch (Authority)
        {
        case EDialogueMemoryAuthority::GameEvent: return 4;
        case EDialogueMemoryAuthority::NpcDisclosure: return 3;
        case EDialogueMemoryAuthority::NpcReport: return 2;
        default: return 1;
        }
    }

    bool PruneFacts(TArray<FDialogueMemoryFactRow>& Rows, const FName NpcId)
    {
        auto CountNpcRows = [&Rows, NpcId]()
        {
            int32 Count = 0;
            for (const FDialogueMemoryFactRow& Row : Rows)
            {
                Count += Row.NpcId == NpcId ? 1 : 0;
            }
            return Count;
        };

        while (CountNpcRows() > MaxFactsPerNpc)
        {
            int32 Candidate = INDEX_NONE;
            int64 CandidateScore = MAX_int64;

            for (int32 Index = 0; Index < Rows.Num(); ++Index)
            {
                const FDialogueMemoryFactRow& Row = Rows[Index];
                if (Row.NpcId != NpcId || Row.Importance >= 5)
                {
                    continue;
                }

                const int64 ActivePenalty = Row.Status == EDialogueMemoryFactStatus::Active ? 1000000000LL : 0LL;
                const int64 Score = ActivePenalty + static_cast<int64>(Row.Importance) * 1000000LL + Row.LastSeenDay;
                if (Score < CandidateScore)
                {
                    Candidate = Index;
                    CandidateScore = Score;
                }
            }

            if (Candidate == INDEX_NONE)
            {
                return false;
            }

            Rows.RemoveAt(Candidate);
        }

        return true;
    }

    bool PruneWorldRows(TArray<FDialogueMemoryWorldStateRow>& Rows)
    {
        while (Rows.Num() > MaxWorldRows)
        {
            int32 Candidate = INDEX_NONE;
            int64 CandidateScore = MAX_int64;

            for (int32 Index = 0; Index < Rows.Num(); ++Index)
            {
                const FDialogueMemoryWorldStateRow& Row = Rows[Index];
                if (Row.Authority == EDialogueMemoryAuthority::GameEvent || Row.Importance >= 5)
                {
                    continue;
                }

                const int64 Score = static_cast<int64>(Row.Importance) * 1000000LL + Row.LastSeenDay;
                if (Score < CandidateScore)
                {
                    Candidate = Index;
                    CandidateScore = Score;
                }
            }

            if (Candidate == INDEX_NONE)
            {
                return false;
            }

            Rows.RemoveAt(Candidate);
        }

        return true;
    }

    bool PruneOpenLoops(TArray<FDialogueMemoryOpenLoopRow>& Rows, const FName NpcId)
    {
        auto CountNpcRows = [&Rows, NpcId]()
        {
            int32 Count = 0;
            for (const FDialogueMemoryOpenLoopRow& Row : Rows)
            {
                Count += Row.NpcId == NpcId ? 1 : 0;
            }
            return Count;
        };

        while (CountNpcRows() > MaxOpenLoopsPerNpc)
        {
            int32 Candidate = INDEX_NONE;
            int64 CandidateScore = MAX_int64;

            for (int32 Index = 0; Index < Rows.Num(); ++Index)
            {
                const FDialogueMemoryOpenLoopRow& Row = Rows[Index];
                if (Row.NpcId != NpcId)
                {
                    continue;
                }

                const bool bProtected =
                    Row.Status == EDialogueMemoryLoopStatus::Open &&
                    (Row.Kind == EDialogueMemoryLoopKind::Promise || Row.Priority >= 5);
                if (bProtected)
                {
                    continue;
                }

                const int64 OpenPenalty = Row.Status == EDialogueMemoryLoopStatus::Open ? 1000000000LL : 0LL;
                const int64 Score = OpenPenalty + static_cast<int64>(Row.Priority) * 1000000LL + Row.LastSeenDay;
                if (Score < CandidateScore)
                {
                    Candidate = Index;
                    CandidateScore = Score;
                }
            }

            if (Candidate == INDEX_NONE)
            {
                return false;
            }

            Rows.RemoveAt(Candidate);
        }

        return true;
    }

    FString CsvEscape(const FString& Value)
    {
        FString Escaped = Value;
        Escaped.ReplaceInline(TEXT("\""), TEXT("\"\""));
        return FString::Printf(TEXT("\"%s\""), *Escaped);
    }

    FString JoinNames(const TArray<FName>& Values)
    {
        TArray<FString> Strings;
        Strings.Reserve(Values.Num());
        for (const FName Value : Values)
        {
            Strings.Add(Value.ToString());
        }
        return FString::Join(Strings, TEXT("|"));
    }

    FString AttitudeString(const EDialogueMemoryAttitude Value)
    {
        switch (Value)
        {
        case EDialogueMemoryAttitude::Guarded: return TEXT("guarded");
        case EDialogueMemoryAttitude::Warm: return TEXT("warm");
        case EDialogueMemoryAttitude::Hurt: return TEXT("hurt");
        case EDialogueMemoryAttitude::Hostile: return TEXT("hostile");
        default: return TEXT("neutral");
        }
    }

    FString VisibilityString(const EDialogueMemoryVisibility Value)
    {
        switch (Value)
        {
        case EDialogueMemoryVisibility::PlayerKnown: return TEXT("player_known");
        case EDialogueMemoryVisibility::WorldKnown: return TEXT("world_known");
        default: return TEXT("npc_private");
        }
    }

    FString FactStatusString(const EDialogueMemoryFactStatus Value)
    {
        switch (Value)
        {
        case EDialogueMemoryFactStatus::Resolved: return TEXT("resolved");
        case EDialogueMemoryFactStatus::Contradicted: return TEXT("contradicted");
        default: return TEXT("active");
        }
    }

    FString AuthorityString(const EDialogueMemoryAuthority Value)
    {
        switch (Value)
        {
        case EDialogueMemoryAuthority::GameEvent: return TEXT("game_event");
        case EDialogueMemoryAuthority::NpcDisclosure: return TEXT("npc_disclosure");
        case EDialogueMemoryAuthority::NpcReport: return TEXT("npc_report");
        default: return TEXT("inference");
        }
    }

    FString ValueTypeString(const EDialogueMemoryValueType Value)
    {
        switch (Value)
        {
        case EDialogueMemoryValueType::Bool: return TEXT("bool");
        case EDialogueMemoryValueType::Int: return TEXT("int");
        case EDialogueMemoryValueType::Float: return TEXT("float");
        case EDialogueMemoryValueType::Name: return TEXT("name");
        default: return TEXT("text");
        }
    }

    FString LoopKindString(const EDialogueMemoryLoopKind Value)
    {
        switch (Value)
        {
        case EDialogueMemoryLoopKind::Promise: return TEXT("promise");
        case EDialogueMemoryLoopKind::Secret: return TEXT("secret");
        case EDialogueMemoryLoopKind::Conflict: return TEXT("conflict");
        case EDialogueMemoryLoopKind::Goal: return TEXT("goal");
        default: return TEXT("question");
        }
    }

    FString LoopStatusString(const EDialogueMemoryLoopStatus Value)
    {
        switch (Value)
        {
        case EDialogueMemoryLoopStatus::Resolved: return TEXT("resolved");
        case EDialogueMemoryLoopStatus::Failed: return TEXT("failed");
        default: return TEXT("open");
        }
    }
}

void UDialogueMemoryTableSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    FString IgnoredError;
    LoadLedger(IgnoredError);
}

void UDialogueMemoryTableSubsystem::ConfigureSaveSlot(const FString& InSlotName, const int32 InUserIndex)
{
    if (!InSlotName.TrimStartAndEnd().IsEmpty())
    {
        SaveSlotName = InSlotName.TrimStartAndEnd();
    }
    SaveUserIndex = FMath::Max(0, InUserIndex);
}

void UDialogueMemoryTableSubsystem::EnsureLedger()
{
    if (!Ledger)
    {
        Ledger = Cast<UDialogueMemorySaveGame>(
            UGameplayStatics::CreateSaveGameObject(UDialogueMemorySaveGame::StaticClass()));
    }
}

bool UDialogueMemoryTableSubsystem::LoadLedger(FString& OutError)
{
    OutError.Reset();
    PendingCompactUpdates.Reset();

    if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex))
    {
        USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex);
        Ledger = Cast<UDialogueMemorySaveGame>(Loaded);
        if (!Ledger)
        {
            OutError = TEXT("The save slot exists but is not a DialogueMemorySaveGame.");
            return false;
        }
        return MigrateCompactLedger(OutError);
    }

    Ledger = Cast<UDialogueMemorySaveGame>(
        UGameplayStatics::CreateSaveGameObject(UDialogueMemorySaveGame::StaticClass()));
    if (!Ledger)
    {
        OutError = TEXT("Unable to create DialogueMemorySaveGame.");
        return false;
    }

    return MigrateCompactLedger(OutError);
}

bool UDialogueMemoryTableSubsystem::SaveLedger(FString& OutError)
{
    OutError.Reset();
    EnsureLedger();

    if (!Ledger || !UGameplayStatics::SaveGameToSlot(Ledger, SaveSlotName, SaveUserIndex))
    {
        OutError = TEXT("SaveGameToSlot failed.");
        return false;
    }

    return true;
}

FDialogueMemoryApplyResult UDialogueMemoryTableSubsystem::ApplyMemoryPatchJson(
    const FString& PatchJson,
    const FName ExpectedNpcId,
    const int64 ExpectedSessionId,
    const int32 WorldDay,
    const bool bSaveImmediately)
{
    using namespace DialogueMemoryTable;
    EnsureLedger();
    if(Ledger && Ledger->SchemaVersion>=2)return ErrorResult(TEXT("legacy_patch_retired"),TEXT("Use validated compact turn updates; raw summary patches are retired."));

    if (ExpectedNpcId.IsNone() || ExpectedSessionId < 1)
    {
        return ErrorResult(TEXT("invalid_expected_identity"), TEXT("Expected NPC and session ID are required."));
    }

    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(PatchJson);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        return ErrorResult(TEXT("invalid_json"), TEXT("Patch is not a valid JSON object."));
    }

    FString Error;
    if (!HasOnlyFields(
            Root,
            {
                TEXT("schema_version"),
                TEXT("npc_id"),
                TEXT("session_id"),
                TEXT("npc_patch"),
                TEXT("fact_upserts"),
                TEXT("world_suggestions"),
                TEXT("open_loop_upserts")
            },
            Error))
    {
        return ErrorResult(TEXT("unexpected_root_field"), Error);
    }

    int64 SchemaVersion = 0;
    if (!TryRequiredInteger(Root, TEXT("schema_version"), SchemaVersion, Error) || SchemaVersion != 1)
    {
        return ErrorResult(TEXT("invalid_schema_version"), Error.IsEmpty() ? TEXT("schema_version must be 1.") : Error);
    }

    FString NpcIdString;
    if (!TryRequiredString(Root, TEXT("npc_id"), 32, NpcIdString, Error))
    {
        return ErrorResult(TEXT("invalid_npc_id"), Error);
    }

    const FName PatchNpcId(*NpcIdString);
    if (PatchNpcId != ExpectedNpcId)
    {
        return ErrorResult(TEXT("npc_mismatch"), TEXT("Patch npc_id does not match the active conversation."));
    }

    const auto* Personas=GetGameInstance()->GetSubsystem<UAlchemyGameplaySubsystem>();
    FDialoguePersonaProfile Profile;
    if (!Personas || !Personas->GetPersonaProfile(PatchNpcId,Profile))
    {
        return ErrorResult(TEXT("unknown_npc"), TEXT("npc_id is not in the supported persona set."));
    }

    int64 SessionId = 0;
    if (!TryRequiredInteger(Root, TEXT("session_id"), SessionId, Error) || SessionId != ExpectedSessionId)
    {
        return ErrorResult(TEXT("session_mismatch"), Error.IsEmpty() ? TEXT("Patch session_id does not match.") : Error);
    }

    EnsureLedger();
    if (!Ledger)
    {
        return ErrorResult(TEXT("ledger_unavailable"), TEXT("Dialogue memory ledger is unavailable."));
    }

    if (const FDialogueMemoryNpcStateRow* ExistingState = Ledger->NpcStates.FindByPredicate(
        [ExpectedNpcId](const FDialogueMemoryNpcStateRow& Row) { return Row.NpcId == ExpectedNpcId; }))
    {
        if (ExistingState->LastSessionId == ExpectedSessionId)
        {
            FDialogueMemoryApplyResult Result;
            Result.bSuccess = true;
            Result.bAlreadyApplied = true;
            return Result;
        }

        if (ExistingState->LastSessionId > ExpectedSessionId)
        {
            return ErrorResult(TEXT("stale_session"), TEXT("Patch session_id is older than the stored session."));
        }
    }

    TSharedPtr<FJsonObject> NpcPatchObject;
    if (!TryRequiredObject(Root, TEXT("npc_patch"), NpcPatchObject, Error))
    {
        return ErrorResult(TEXT("invalid_npc_patch"), Error);
    }
    if (!HasOnlyFields(
            NpcPatchObject,
            {
                TEXT("attitude"),
                TEXT("trust_delta_suggested"),
                TEXT("relationship_summary"),
                TEXT("last_exchange_summary"),
                TEXT("last_quote"),
                TEXT("topic_tags")
            },
            Error))
    {
        return ErrorResult(TEXT("unexpected_npc_patch_field"), Error);
    }

    FDialogueMemoryNpcStateRow ParsedNpcPatch;
    ParsedNpcPatch.NpcId = ExpectedNpcId;

    FString EnumString;
    if (!TryRequiredString(NpcPatchObject, TEXT("attitude"), 16, EnumString, Error) ||
        !ParseAttitude(EnumString, ParsedNpcPatch.Attitude))
    {
        return ErrorResult(TEXT("invalid_attitude"), Error.IsEmpty() ? TEXT("Invalid attitude enum.") : Error);
    }

    if (!TryRequiredString(NpcPatchObject, TEXT("relationship_summary"), 120, ParsedNpcPatch.RelationshipSummary, Error) ||
        !TryRequiredString(NpcPatchObject, TEXT("last_exchange_summary"), 160, ParsedNpcPatch.LastExchangeSummary, Error) ||
        !TryOptionalString(NpcPatchObject, TEXT("last_quote"), 48, ParsedNpcPatch.LastQuote, Error) ||
        !TryNameArray(NpcPatchObject, TEXT("topic_tags"), 8, 32, ParsedNpcPatch.TopicTags, Error))
    {
        return ErrorResult(TEXT("invalid_npc_patch"), Error);
    }

    int64 TrustDelta = 0;
    if (!TryOptionalInteger(NpcPatchObject, TEXT("trust_delta_suggested"), 0, TrustDelta, Error) ||
        TrustDelta < -10 || TrustDelta > 10)
    {
        return ErrorResult(TEXT("invalid_trust_delta"), Error.IsEmpty() ? TEXT("trust_delta_suggested must be -10..10.") : Error);
    }

    const TArray<TSharedPtr<FJsonValue>>* FactValues = nullptr;
    if (!TryRequiredArray(Root, TEXT("fact_upserts"), FactValues, Error) || FactValues->Num() > 6)
    {
        return ErrorResult(TEXT("invalid_fact_upserts"), Error.IsEmpty() ? TEXT("fact_upserts exceeds 6 rows.") : Error);
    }

    TArray<FDialogueMemoryFactRow> ParsedFacts;
    TSet<FName> FactKeys;
    for (const TSharedPtr<FJsonValue>& Value : *FactValues)
    {
        const TSharedPtr<FJsonObject> Object =
            Value.IsValid() && Value->Type == EJson::Object ? Value->AsObject() : nullptr;
        if (!Object.IsValid())
        {
            return ErrorResult(TEXT("invalid_fact_row"), TEXT("fact_upserts must contain objects."));
        }
        if (!HasOnlyFields(
                Object,
                {
                    TEXT("fact_key"),
                    TEXT("subject"),
                    TEXT("predicate"),
                    TEXT("object_text"),
                    TEXT("visibility"),
                    TEXT("status"),
                    TEXT("importance"),
                    TEXT("confidence"),
                    TEXT("authority")
                },
                Error))
        {
            return ErrorResult(TEXT("unexpected_fact_field"), Error);
        }

        FDialogueMemoryFactRow Row;
        Row.NpcId = ExpectedNpcId;
        Row.SourceSessionId = SessionId;
        Row.LastSeenDay = WorldDay;

        FString Key;
        FString Subject;
        FString Predicate;
        FString Visibility;
        FString Status;
        FString Authority;
        int64 Importance = 0;
        int64 Confidence = 0;

        if (!TryRequiredString(Object, TEXT("fact_key"), 64, Key, Error) || !IsStableKey(Key) ||
            !TryRequiredString(Object, TEXT("subject"), 48, Subject, Error) ||
            !TryRequiredString(Object, TEXT("predicate"), 32, Predicate, Error) ||
            !TryRequiredString(Object, TEXT("object_text"), 80, Row.ObjectText, Error) ||
            !TryRequiredString(Object, TEXT("visibility"), 24, Visibility, Error) ||
            !TryRequiredString(Object, TEXT("status"), 16, Status, Error) ||
            !TryRequiredInteger(Object, TEXT("importance"), Importance, Error) ||
            !TryRequiredInteger(Object, TEXT("confidence"), Confidence, Error) ||
            !TryRequiredString(Object, TEXT("authority"), 24, Authority, Error))
        {
            return ErrorResult(TEXT("invalid_fact_row"), Error.IsEmpty() ? TEXT("Invalid fact key.") : Error);
        }

        Row.FactKey = FName(*Key);
        Row.Subject = FName(*Subject);
        Row.Predicate = FName(*Predicate);
        Row.Importance = static_cast<int32>(Importance);
        Row.Confidence = static_cast<int32>(Confidence);

        if (FactKeys.Contains(Row.FactKey) ||
            Row.Importance < 1 || Row.Importance > 5 ||
            Row.Confidence < 0 || Row.Confidence > 100 ||
            !ParseVisibility(Visibility, Row.Visibility) ||
            !ParseFactStatus(Status, Row.Status) ||
            !ParseAiAuthority(Authority, Row.Authority))
        {
            return ErrorResult(TEXT("invalid_fact_row"), TEXT("Fact row contains a duplicate key, range error, or invalid enum."));
        }

        FactKeys.Add(Row.FactKey);
        ParsedFacts.Add(Row);
    }

    const TArray<TSharedPtr<FJsonValue>>* WorldValues = nullptr;
    if (!TryRequiredArray(Root, TEXT("world_suggestions"), WorldValues, Error) || WorldValues->Num() > 3)
    {
        return ErrorResult(TEXT("invalid_world_suggestions"), Error.IsEmpty() ? TEXT("world_suggestions exceeds 3 rows.") : Error);
    }

    TArray<FDialogueMemoryWorldStateRow> ParsedWorldRows;
    TSet<FName> WorldKeys;
    for (const TSharedPtr<FJsonValue>& Value : *WorldValues)
    {
        const TSharedPtr<FJsonObject> Object =
            Value.IsValid() && Value->Type == EJson::Object ? Value->AsObject() : nullptr;
        if (!Object.IsValid())
        {
            return ErrorResult(TEXT("invalid_world_row"), TEXT("world_suggestions must contain objects."));
        }
        if (!HasOnlyFields(
                Object,
                {
                    TEXT("state_key"),
                    TEXT("value_type"),
                    TEXT("value_text"),
                    TEXT("authority"),
                    TEXT("confidence"),
                    TEXT("importance")
                },
                Error))
        {
            return ErrorResult(TEXT("unexpected_world_field"), Error);
        }

        FDialogueMemoryWorldStateRow Row;
        Row.SourceSessionId = SessionId;
        Row.LastSeenDay = WorldDay;

        FString Key;
        FString ValueType;
        FString Authority;
        int64 Importance = 0;
        int64 Confidence = 0;

        if (!TryRequiredString(Object, TEXT("state_key"), 64, Key, Error) || !IsStableKey(Key) ||
            !TryRequiredString(Object, TEXT("value_type"), 16, ValueType, Error) ||
            !TryRequiredString(Object, TEXT("value_text"), 64, Row.ValueText, Error) ||
            !TryRequiredString(Object, TEXT("authority"), 24, Authority, Error) ||
            !TryRequiredInteger(Object, TEXT("confidence"), Confidence, Error) ||
            !TryRequiredInteger(Object, TEXT("importance"), Importance, Error))
        {
            return ErrorResult(TEXT("invalid_world_row"), Error.IsEmpty() ? TEXT("Invalid world key.") : Error);
        }

        Row.StateKey = FName(*Key);
        Row.Confidence = static_cast<int32>(Confidence);
        Row.Importance = static_cast<int32>(Importance);

        if (WorldKeys.Contains(Row.StateKey) ||
            Row.Confidence < 0 || Row.Confidence > 100 ||
            Row.Importance < 1 || Row.Importance > 5 ||
            !ParseValueType(ValueType, Row.ValueType) ||
            !ParseAiAuthority(Authority, Row.Authority) ||
            Row.Authority == EDialogueMemoryAuthority::NpcDisclosure)
        {
            return ErrorResult(TEXT("invalid_world_row"), TEXT("World row contains a duplicate key, range error, or invalid enum."));
        }

        WorldKeys.Add(Row.StateKey);
        ParsedWorldRows.Add(Row);
    }

    const TArray<TSharedPtr<FJsonValue>>* LoopValues = nullptr;
    if (!TryRequiredArray(Root, TEXT("open_loop_upserts"), LoopValues, Error) || LoopValues->Num() > 3)
    {
        return ErrorResult(TEXT("invalid_open_loop_upserts"), Error.IsEmpty() ? TEXT("open_loop_upserts exceeds 3 rows.") : Error);
    }

    TArray<FDialogueMemoryOpenLoopRow> ParsedLoops;
    TSet<FName> LoopKeys;
    for (const TSharedPtr<FJsonValue>& Value : *LoopValues)
    {
        const TSharedPtr<FJsonObject> Object =
            Value.IsValid() && Value->Type == EJson::Object ? Value->AsObject() : nullptr;
        if (!Object.IsValid())
        {
            return ErrorResult(TEXT("invalid_open_loop_row"), TEXT("open_loop_upserts must contain objects."));
        }
        if (!HasOnlyFields(
                Object,
                {
                    TEXT("loop_key"),
                    TEXT("kind"),
                    TEXT("summary"),
                    TEXT("status"),
                    TEXT("priority"),
                    TEXT("trigger_tags")
                },
                Error))
        {
            return ErrorResult(TEXT("unexpected_open_loop_field"), Error);
        }

        FDialogueMemoryOpenLoopRow Row;
        Row.NpcId = ExpectedNpcId;
        Row.SourceSessionId = SessionId;
        Row.LastSeenDay = WorldDay;

        FString Key;
        FString Kind;
        FString Status;
        int64 Priority = 0;

        if (!TryRequiredString(Object, TEXT("loop_key"), 64, Key, Error) || !IsStableKey(Key) ||
            !TryRequiredString(Object, TEXT("kind"), 16, Kind, Error) ||
            !TryRequiredString(Object, TEXT("summary"), 96, Row.Summary, Error) ||
            !TryRequiredString(Object, TEXT("status"), 16, Status, Error) ||
            !TryRequiredInteger(Object, TEXT("priority"), Priority, Error) ||
            !TryNameArray(Object, TEXT("trigger_tags"), 5, 32, Row.TriggerTags, Error))
        {
            return ErrorResult(TEXT("invalid_open_loop_row"), Error.IsEmpty() ? TEXT("Invalid loop key.") : Error);
        }

        Row.LoopKey = FName(*Key);
        Row.Priority = static_cast<int32>(Priority);

        if (LoopKeys.Contains(Row.LoopKey) ||
            Row.Priority < 1 || Row.Priority > 5 ||
            !ParseLoopKind(Kind, Row.Kind) ||
            !ParseLoopStatus(Status, Row.Status))
        {
            return ErrorResult(TEXT("invalid_open_loop_row"), TEXT("Open-loop row contains a duplicate key, range error, or invalid enum."));
        }

        LoopKeys.Add(Row.LoopKey);
        ParsedLoops.Add(Row);
    }

    UDialogueMemorySaveGame* Working = NewObject<UDialogueMemorySaveGame>(this);
    Working->SchemaVersion = Ledger->SchemaVersion;
    Working->NpcStates = Ledger->NpcStates;
    Working->CharacterFacts = Ledger->CharacterFacts;
    Working->WorldStates = Ledger->WorldStates;
    Working->OpenLoops = Ledger->OpenLoops;

    FDialogueMemoryNpcStateRow* State = Working->NpcStates.FindByPredicate(
        [ExpectedNpcId](const FDialogueMemoryNpcStateRow& Row) { return Row.NpcId == ExpectedNpcId; });
    if (!State)
    {
        FDialogueMemoryNpcStateRow NewState;
        NewState.NpcId = ExpectedNpcId;
        Working->NpcStates.Add(NewState);
        State = &Working->NpcStates.Last();
    }

    State->Trust = FMath::Clamp(State->Trust + static_cast<int32>(TrustDelta), -100, 100);
    State->Attitude = ParsedNpcPatch.Attitude;
    State->RelationshipSummary = ParsedNpcPatch.RelationshipSummary;
    State->LastExchangeSummary = ParsedNpcPatch.LastExchangeSummary;
    State->LastQuote = ParsedNpcPatch.LastQuote;
    State->TopicTags = ParsedNpcPatch.TopicTags;
    State->LastWorldDay = WorldDay;
    State->LastSessionId = SessionId;

    for (const FDialogueMemoryFactRow& Incoming : ParsedFacts)
    {
        FDialogueMemoryFactRow* Existing = Working->CharacterFacts.FindByPredicate(
            [&Incoming](const FDialogueMemoryFactRow& Row) { return Row.NpcId == Incoming.NpcId && Row.FactKey == Incoming.FactKey; });

        if (!Existing)
        {
            Working->CharacterFacts.Add(Incoming);
        }
        else if (AuthorityRank(Incoming.Authority) >= AuthorityRank(Existing->Authority))
        {
            *Existing = Incoming;
        }
        else
        {
            Existing->LastSeenDay = FMath::Max(Existing->LastSeenDay, Incoming.LastSeenDay);
        }
    }

    for (const FDialogueMemoryWorldStateRow& Incoming : ParsedWorldRows)
    {
        FDialogueMemoryWorldStateRow* Existing = Working->WorldStates.FindByPredicate(
            [&Incoming](const FDialogueMemoryWorldStateRow& Row) { return Row.StateKey == Incoming.StateKey; });

        if (!Existing)
        {
            Working->WorldStates.Add(Incoming);
        }
        else if (AuthorityRank(Incoming.Authority) >= AuthorityRank(Existing->Authority))
        {
            *Existing = Incoming;
        }
        else
        {
            Existing->LastSeenDay = FMath::Max(Existing->LastSeenDay, Incoming.LastSeenDay);
        }
    }

    for (const FDialogueMemoryOpenLoopRow& Incoming : ParsedLoops)
    {
        FDialogueMemoryOpenLoopRow* Existing = Working->OpenLoops.FindByPredicate(
            [&Incoming](const FDialogueMemoryOpenLoopRow& Row) { return Row.NpcId == Incoming.NpcId && Row.LoopKey == Incoming.LoopKey; });

        if (Existing)
        {
            *Existing = Incoming;
        }
        else
        {
            Working->OpenLoops.Add(Incoming);
        }
    }

    if (!PruneFacts(Working->CharacterFacts, ExpectedNpcId) ||
        !PruneWorldRows(Working->WorldStates) ||
        !PruneOpenLoops(Working->OpenLoops, ExpectedNpcId))
    {
        return ErrorResult(TEXT("protected_capacity_exceeded"), TEXT("The table cap is full of protected rows; no data was changed."));
    }

    UDialogueMemorySaveGame* PreviousLedger = Ledger;
    Ledger = Working;

    if (bSaveImmediately)
    {
        FString SaveError;
        if (!SaveLedger(SaveError))
        {
            Ledger = PreviousLedger;
            return ErrorResult(TEXT("save_failed"), SaveError);
        }
    }

    FDialogueMemoryApplyResult Result;
    Result.bSuccess = true;
    Result.FactsUpserted = ParsedFacts.Num();
    Result.WorldRowsUpserted = ParsedWorldRows.Num();
    Result.OpenLoopsUpserted = ParsedLoops.Num();
    return Result;
}

bool UDialogueMemoryTableSubsystem::SetAuthoritativeNpcProgress(
    const FName NpcId,
    const int32 Familiarity,
    const int64 KnownSecretFlags,
    const int32 WorldDay,
    const bool bSaveImmediately,
    FString& OutError)
{
    OutError.Reset();
    if (NpcId.IsNone())
    {
        OutError = TEXT("NpcId is required.");
        return false;
    }

    EnsureLedger();
    UDialogueMemorySaveGame* Working = NewObject<UDialogueMemorySaveGame>(this);
    Working->SchemaVersion = Ledger->SchemaVersion;
    Working->NpcStates = Ledger->NpcStates;
    Working->CharacterFacts = Ledger->CharacterFacts;
    Working->WorldStates = Ledger->WorldStates;
    Working->OpenLoops = Ledger->OpenLoops;

    FDialogueMemoryNpcStateRow* State = Working->NpcStates.FindByPredicate(
        [NpcId](const FDialogueMemoryNpcStateRow& Row) { return Row.NpcId == NpcId; });
    if (!State)
    {
        FDialogueMemoryNpcStateRow NewState;
        NewState.NpcId = NpcId;
        Working->NpcStates.Add(NewState);
        State = &Working->NpcStates.Last();
    }

    State->Familiarity = FMath::Max(State->Familiarity,FMath::Clamp(Familiarity, 1, 3));
    State->KnownSecretFlags = KnownSecretFlags;
    State->LastWorldDay = FMath::Max(0, WorldDay);

    UDialogueMemorySaveGame* PreviousLedger = Ledger;
    Ledger = Working;
    if (bSaveImmediately && !SaveLedger(OutError))
    {
        Ledger = PreviousLedger;
        return false;
    }

    return true;
}

bool UDialogueMemoryTableSubsystem::UpsertAuthoritativeWorldState(
    const FName StateKey,
    const EDialogueMemoryValueType ValueType,
    const FString& ValueText,
    const int32 Importance,
    const int32 WorldDay,
    const int64 SourceSessionId,
    const bool bSaveImmediately,
    FString& OutError)
{
    using namespace DialogueMemoryTable;

    OutError.Reset();
    if (!IsStableKey(StateKey.ToString()))
    {
        OutError = TEXT("StateKey must use lowercase ASCII, digits, dot, underscore, or hyphen.");
        return false;
    }

    if (ValueText.Len() > 64 || Importance < 1 || Importance > 5)
    {
        OutError = TEXT("Authoritative world row exceeds its value or importance limits.");
        return false;
    }

    EnsureLedger();
    UDialogueMemorySaveGame* Working = NewObject<UDialogueMemorySaveGame>(this);
    Working->SchemaVersion = Ledger->SchemaVersion;
    Working->NpcStates = Ledger->NpcStates;
    Working->CharacterFacts = Ledger->CharacterFacts;
    Working->WorldStates = Ledger->WorldStates;
    Working->OpenLoops = Ledger->OpenLoops;

    FDialogueMemoryWorldStateRow Row;
    Row.StateKey = StateKey;
    Row.ValueType = ValueType;
    Row.ValueText = ValueText;
    Row.Authority = EDialogueMemoryAuthority::GameEvent;
    Row.Confidence = 100;
    Row.Importance = Importance;
    Row.SourceSessionId = SourceSessionId;
    Row.LastSeenDay = WorldDay;

    FDialogueMemoryWorldStateRow* Existing = Working->WorldStates.FindByPredicate(
        [StateKey](const FDialogueMemoryWorldStateRow& Item) { return Item.StateKey == StateKey; });
    if (Existing)
    {
        *Existing = Row;
    }
    else
    {
        Working->WorldStates.Add(Row);
    }

    if (!PruneWorldRows(Working->WorldStates))
    {
        OutError = TEXT("World table is full of protected rows.");
        return false;
    }

    UDialogueMemorySaveGame* PreviousLedger = Ledger;
    Ledger = Working;
    if (bSaveImmediately && !SaveLedger(OutError))
    {
        Ledger = PreviousLedger;
        return false;
    }

    return true;
}

FDialogueMemoryLedgerSnapshot UDialogueMemoryTableSubsystem::GetLedgerSnapshot() const
{
    FDialogueMemoryLedgerSnapshot Snapshot;
    if (Ledger)
    {
        Snapshot.NpcStates = Ledger->NpcStates;
        Snapshot.CharacterFacts = Ledger->CharacterFacts;
        Snapshot.WorldStates = Ledger->WorldStates;
        Snapshot.OpenLoops = Ledger->OpenLoops;
    }
    return Snapshot;
}

bool UDialogueMemoryTableSubsystem::ExportLedgerToCsv(const FString& Directory, FString& OutError) const
{
    using namespace DialogueMemoryTable;

    OutError.Reset();
    if (!Ledger)
    {
        OutError = TEXT("Dialogue memory ledger is unavailable.");
        return false;
    }

    const FString OutputDirectory = Directory.TrimStartAndEnd().IsEmpty()
        ? FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("DialogueMemoryTable"))
        : Directory;

    if (!IFileManager::Get().MakeDirectory(*OutputDirectory, true))
    {
        OutError = FString::Printf(TEXT("Unable to create CSV directory: %s"), *OutputDirectory);
        return false;
    }

    FString NpcCsv = TEXT("npc_id,familiarity,trust,attitude,relationship_summary,last_exchange_summary,last_quote,topic_tags,known_secret_flags,last_world_day,last_session_id\n");
    for (const FDialogueMemoryNpcStateRow& Row : Ledger->NpcStates)
    {
        NpcCsv += FString::Printf(
            TEXT("%s,%d,%d,%s,%s,%s,%s,%s,%lld,%d,%lld\n"),
            *CsvEscape(Row.NpcId.ToString()),
            Row.Familiarity,
            Row.Trust,
            *CsvEscape(AttitudeString(Row.Attitude)),
            *CsvEscape(Row.RelationshipSummary),
            *CsvEscape(Row.LastExchangeSummary),
            *CsvEscape(Row.LastQuote),
            *CsvEscape(JoinNames(Row.TopicTags)),
            static_cast<long long>(Row.KnownSecretFlags),
            Row.LastWorldDay,
            static_cast<long long>(Row.LastSessionId));
    }

    FString FactCsv = TEXT("fact_key,npc_id,subject,predicate,object_text,visibility,status,importance,confidence,authority,source_session_id,last_seen_day\n");
    for (const FDialogueMemoryFactRow& Row : Ledger->CharacterFacts)
    {
        FactCsv += FString::Printf(
            TEXT("%s,%s,%s,%s,%s,%s,%s,%d,%d,%s,%lld,%d\n"),
            *CsvEscape(Row.FactKey.ToString()),
            *CsvEscape(Row.NpcId.ToString()),
            *CsvEscape(Row.Subject.ToString()),
            *CsvEscape(Row.Predicate.ToString()),
            *CsvEscape(Row.ObjectText),
            *CsvEscape(VisibilityString(Row.Visibility)),
            *CsvEscape(FactStatusString(Row.Status)),
            Row.Importance,
            Row.Confidence,
            *CsvEscape(AuthorityString(Row.Authority)),
            static_cast<long long>(Row.SourceSessionId),
            Row.LastSeenDay);
    }

    FString WorldCsv = TEXT("state_key,value_type,value_text,authority,confidence,importance,source_session_id,last_seen_day\n");
    for (const FDialogueMemoryWorldStateRow& Row : Ledger->WorldStates)
    {
        WorldCsv += FString::Printf(
            TEXT("%s,%s,%s,%s,%d,%d,%lld,%d\n"),
            *CsvEscape(Row.StateKey.ToString()),
            *CsvEscape(ValueTypeString(Row.ValueType)),
            *CsvEscape(Row.ValueText),
            *CsvEscape(AuthorityString(Row.Authority)),
            Row.Confidence,
            Row.Importance,
            static_cast<long long>(Row.SourceSessionId),
            Row.LastSeenDay);
    }

    FString LoopCsv = TEXT("loop_key,npc_id,kind,summary,status,priority,trigger_tags,source_session_id,last_seen_day\n");
    for (const FDialogueMemoryOpenLoopRow& Row : Ledger->OpenLoops)
    {
        LoopCsv += FString::Printf(
            TEXT("%s,%s,%s,%s,%s,%d,%s,%lld,%d\n"),
            *CsvEscape(Row.LoopKey.ToString()),
            *CsvEscape(Row.NpcId.ToString()),
            *CsvEscape(LoopKindString(Row.Kind)),
            *CsvEscape(Row.Summary),
            *CsvEscape(LoopStatusString(Row.Status)),
            Row.Priority,
            *CsvEscape(JoinNames(Row.TriggerTags)),
            static_cast<long long>(Row.SourceSessionId),
            Row.LastSeenDay);
    }

    const struct
    {
        const TCHAR* Filename;
        const FString* Contents;
    } Files[] =
    {
        { TEXT("npc_state.csv"), &NpcCsv },
        { TEXT("character_facts.csv"), &FactCsv },
        { TEXT("world_state.csv"), &WorldCsv },
        { TEXT("open_loops.csv"), &LoopCsv }
    };

    for (const auto& File : Files)
    {
        const FString Path = FPaths::Combine(OutputDirectory, File.Filename);
        if (!FFileHelper::SaveStringToFile(*File.Contents, *Path, FFileHelper::EEncodingOptions::ForceUTF8))
        {
            OutError = FString::Printf(TEXT("Unable to write CSV file: %s"), *Path);
            return false;
        }
    }

    return true;
}


bool UDialogueMemoryTableSubsystem::MigrateCompactLedger(FString& Error)
{
 if(!Ledger){Error=TEXT("ledger_unavailable");return false;}
 if(Ledger->SchemaVersion==2)return true;
 if(Ledger->SchemaVersion>2){Error=TEXT("unsupported_memory_schema");return false;}
 auto* Before=Ledger.Get();auto* Work=DuplicateObject<UDialogueMemorySaveGame>(Before,this);
 // Old model summaries have no verifiable source. Preserve authority/progress,
 // not guessed free-form history. Existing delivered IDs live in the conversation save.
 Work->CharacterFacts.RemoveAll([](const auto& F){return F.Authority!=EDialogueMemoryAuthority::GameEvent;});
 Work->WorldStates.RemoveAll([](const auto& W){return W.Authority!=EDialogueMemoryAuthority::GameEvent;});
 Work->OpenLoops.Reset();
 for(auto& N:Work->NpcStates){N.RelationshipSummary.Reset();N.LastExchangeSummary.Reset();N.LastQuote.Reset();N.TopicTags.Reset();N.MemoryRevision.Reset();}
 Work->SchemaVersion=2;Ledger=Work;
 if(!SaveLedger(Error)){Ledger=Before;return false;}return true;
}
bool UDialogueMemoryTableSubsystem::ApplyCompactTurn(const DialogueCompactMemory::Delta& Delta,FString& Error)
{
 EnsureLedger();if(!Ledger||Ledger->SchemaVersion!=2){Error=TEXT("compact_memory_not_ready");return false;}
 auto Store=DialogueCompactMemoryRuntime::Read(GetLedgerSnapshot());
 for(const auto& Pending:PendingCompactUpdates)DialogueCompactMemory::Apply(Store,Pending);
 DialogueCompactMemory::Apply(Store,Delta);
 auto* Before=Ledger.Get();auto* Work=DuplicateObject<UDialogueMemorySaveGame>(Before,this);
 DialogueCompactMemoryRuntime::Write(Store,*Work);Ledger=Work;
 if(SaveLedger(Error)){PendingCompactUpdates.Reset();return true;}
 Ledger=Before;
 if(!PendingCompactUpdates.ContainsByPredicate([&](const auto& D){return D.owner==Delta.owner&&D.turn==Delta.turn;}))PendingCompactUpdates.Add(Delta);
 // Only structured patches are retained in RAM, never raw chat transcripts.
 int32 Count=0;for(int32 I=PendingCompactUpdates.Num()-1;I>=0;--I)if(PendingCompactUpdates[I].owner==Delta.owner&&++Count>3)PendingCompactUpdates.RemoveAt(I);
 return false;
}
bool UDialogueMemoryTableSubsystem::RetryCompactSaves(FString& Error)
{
 if(PendingCompactUpdates.IsEmpty())return true;
 const auto Last=PendingCompactUpdates.Last();return ApplyCompactTurn(Last,Error);
}
