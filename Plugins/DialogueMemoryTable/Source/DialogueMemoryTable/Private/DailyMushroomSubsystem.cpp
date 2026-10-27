#include "DailyMushroomSubsystem.h"

#include "Kismet/GameplayStatics.h"

void UDailyMushroomSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ConfigureSaveSlot(SaveSlotName, SaveUserIndex);
}

bool UDailyMushroomSubsystem::ConfigureSaveSlot(const FString& InSlotName, const int32 InUserIndex)
{
    if (InSlotName.IsEmpty())
    {
        return false;
    }

    SaveSlotName = InSlotName;
    SaveUserIndex = FMath::Max(0, InUserIndex);
    Save = nullptr;

    if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex))
    {
        Save = Cast<UDailyMushroomSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex));
    }

    if (!Save)
    {
        Save = Cast<UDailyMushroomSaveGame>(UGameplayStatics::CreateSaveGameObject(UDailyMushroomSaveGame::StaticClass()));
    }

    CurrentDay = Save ? FMath::Max(1, Save->CurrentDay) : 1;
    if (Save)
    {
        Save->CurrentDay = CurrentDay;
        Persist();
    }
    return Save != nullptr;
}

bool UDailyMushroomSubsystem::Persist()
{
    if (!Save)
    {
        return false;
    }
    Save->CurrentDay = CurrentDay;
    return UGameplayStatics::SaveGameToSlot(Save, SaveSlotName, SaveUserIndex);
}

int32 UDailyMushroomSubsystem::AdvanceDay()
{
    CurrentDay = FMath::Max(1, CurrentDay + 1);
    Persist();
    return CurrentDay;
}

bool UDailyMushroomSubsystem::ConfirmDryPotSynthesis()
{
    if (!Save)
    {
        return false;
    }

    const int32 PreviousDay = CurrentDay;
    const int32 PreviousConfirmed = Save->LastConfirmedSynthesisDay;
    Save->LastConfirmedSynthesisDay = CurrentDay;
    CurrentDay = FMath::Max(1, CurrentDay + 1);
    if (Persist()) return true;
    CurrentDay = PreviousDay;
    Save->CurrentDay = PreviousDay;
    Save->LastConfirmedSynthesisDay = PreviousConfirmed;
    return false;
}

bool UDailyMushroomSubsystem::SetCurrentDay(const int32 NewDay)
{
    if (NewDay < 1)
    {
        return false;
    }
    CurrentDay = NewDay;
    return Persist();
}

const TArray<FName>& UDailyMushroomSubsystem::MushroomCatalog()
{
    static const TArray<FName> Catalog = {
        TEXT("SM_Env_Mushroom_01"), TEXT("SM_Env_Mushroom_02"), TEXT("SM_Env_Mushroom_03"),
        TEXT("SM_Env_Mushroom_04"), TEXT("SM_Env_Mushroom_05"), TEXT("SM_Env_Mushroom_Cap_01"),
        TEXT("SM_Env_Mushroom_Cap_02"), TEXT("SM_Env_Mushroom_Cap_03"), TEXT("SM_Env_Mushroom_Cap_04"),
        TEXT("SM_Env_Mushroom_Small_01"), TEXT("SM_Env_Mushroom_Small_02"), TEXT("SM_Env_Mushroom_Small_03"),
        TEXT("SM_Env_Mushroom_Small_04"), TEXT("SM_Env_Mushroom_Small_05"), TEXT("SM_Env_Mushroom_Small_06"),
        TEXT("SM_Env_Mushroom_Small_07"), TEXT("SM_Env_Mushroom_Small_08"), TEXT("SM_Env_Mushroom_Small_09"),
        TEXT("SM_Env_Mushroom_Small_10")
    };
    return Catalog;
}

int32 UDailyMushroomSubsystem::DailySeed(const int32 Day, const int32 Index)
{
    return FMath::Abs(Day * 92821 + Index * 68917 + 17);
}

EAlchemyElement UDailyMushroomSubsystem::MushroomElementForIndex(const int32 Index)
{
    // Element is a property of the mushroom type. Day changes availability and pip value only.
    static const TArray<EAlchemyElement> Elements = {
        EAlchemyElement::Fire, EAlchemyElement::Wind, EAlchemyElement::Water,
        EAlchemyElement::Fire, EAlchemyElement::Wind, EAlchemyElement::Water,
        EAlchemyElement::Fire, EAlchemyElement::Wind, EAlchemyElement::Water,
        EAlchemyElement::Fire, EAlchemyElement::Wind, EAlchemyElement::Water,
        EAlchemyElement::Fire, EAlchemyElement::Wind, EAlchemyElement::Water,
        EAlchemyElement::Fire, EAlchemyElement::Wind, EAlchemyElement::Water,
        EAlchemyElement::Fire
    };
    return Elements.IsValidIndex(Index) ? Elements[Index] : EAlchemyElement::Fire;
}

FDailyMushroomCardRow UDailyMushroomSubsystem::MakeDefinition(const int32 Day, const int32 Index)
{
    const TArray<FName>& Catalog = MushroomCatalog();
    FDailyMushroomCardRow Definition;
    if (!Catalog.IsValidIndex(Index))
    {
        return Definition;
    }

    FRandomStream Random(DailySeed(Day, Index));
    Definition.MushroomId = Catalog[Index];
    Definition.DisplayName = Catalog[Index].ToString();
    Definition.Day = Day;
    Definition.bAvailable = Random.RandRange(0, 16) < 10;
    Definition.CardId = FName(*FString::Printf(TEXT("mushroom_day_%d_%02d"), Day, Index));
    Definition.Element = MushroomElementForIndex(Index);
    Definition.PipValue = Random.RandRange(1, 9);
    Definition.Weight = Random.RandRange(1, 3);
    Definition.Tags = TEXT("daily_mushroom,small_card");
    switch (Definition.Element)
    {
    case EAlchemyElement::Fire: Definition.Value.Fire = Definition.PipValue; break;
    case EAlchemyElement::Wind: Definition.Value.Wind = Definition.PipValue; break;
    case EAlchemyElement::Water: Definition.Value.Water = Definition.PipValue; break;
    }
    return Definition;
}

TArray<FDailyMushroomCardRow> UDailyMushroomSubsystem::GetDailyMushrooms() const
{
    TArray<FDailyMushroomCardRow> Result;
    for (int32 Index = 0; Index < MushroomCatalog().Num(); ++Index)
    {
        Result.Add(MakeDefinition(CurrentDay, Index));
    }
    return Result;
}

bool UDailyMushroomSubsystem::GetDailyMushroom(const FName MushroomId, FDailyMushroomCardRow& OutDefinition) const
{
    const int32 Index = MushroomCatalog().Find(MushroomId);
    if (Index == INDEX_NONE)
    {
        OutDefinition = FDailyMushroomCardRow();
        return false;
    }
    OutDefinition = MakeDefinition(CurrentDay, Index);
    return true;
}

bool UDailyMushroomSubsystem::IsMushroomAvailable(const FName MushroomId) const
{
    FDailyMushroomCardRow Definition;
    return GetDailyMushroom(MushroomId, Definition) && Definition.bAvailable;
}

bool UDailyMushroomSubsystem::GetMushroomValue(const FName MushroomId, FAlchemyVector& OutValue) const
{
    FDailyMushroomCardRow Definition;
    if (!GetDailyMushroom(MushroomId, Definition))
    {
        OutValue = FAlchemyVector();
        return false;
    }
    OutValue = Definition.Value;
    return true;
}

TArray<FDailyElementModifierRow> UDailyMushroomSubsystem::GetDailyElementModifierRuleTemplates() const
{
    TArray<FDailyElementModifierRow> Templates;

    FDailyElementModifierRow Block;
    Block.RuleId = TEXT("placeholder_low_score_block");
    Block.ConditionId = TEXT("score_threshold_pending");
    Block.ConditionPlaceholder = TEXT("前一天评分低于阈值（待数值系统定义）");
    Block.EffectPlaceholder = TEXT("次日该角色主导元素暂时无法获取");
    Block.Tags = TEXT("placeholder,low_score,block_element");
    Templates.Add(Block);

    FDailyElementModifierRow Reduce;
    Reduce.RuleId = TEXT("placeholder_low_score_reduce");
    Reduce.ConditionId = TEXT("score_band_pending");
    Reduce.ConditionPlaceholder = TEXT("前一天评分处于惩罚区间（待数值系统定义）");
    Reduce.EffectPlaceholder = TEXT("次日该角色主导元素的可采数量减少");
    Reduce.Tags = TEXT("placeholder,low_score,reduce_availability");
    Templates.Add(Reduce);

    FDailyElementModifierRow Bonus;
    Bonus.RuleId = TEXT("placeholder_high_score_bonus");
    Bonus.ConditionId = TEXT("score_bonus_pending");
    Bonus.ConditionPlaceholder = TEXT("前一天评分达到奖励区间（待数值系统定义）");
    Bonus.EffectPlaceholder = TEXT("次日该角色主导元素增加或增强（待数值系统定义）");
    Bonus.Tags = TEXT("placeholder,high_score,bonus_element");
    Templates.Add(Bonus);

    return Templates;
}

TArray<FDailyElementModifierRow> UDailyMushroomSubsystem::GetElementModifiersForDay(const int32 TargetDay) const
{
    TArray<FDailyElementModifierRow> Result;
    if (!Save || TargetDay < 1)
    {
        return Result;
    }

    for (const FDailyElementModifierRow& Modifier : Save->ElementModifiers)
    {
        if (Modifier.TargetDay == TargetDay)
        {
            Result.Add(Modifier);
        }
    }
    return Result;
}

bool UDailyMushroomSubsystem::AddElementModifier(const FDailyElementModifierRow& Modifier)
{
    if (!Save || Modifier.TargetDay < 1 || Modifier.RuleId.IsNone())
    {
        return false;
    }

    FDailyElementModifierRow Copy = Modifier;
    Copy.AvailabilityScale = FMath::Clamp(Copy.AvailabilityScale, 0.0f, 1.0f);
    Copy.PipValueScale = FMath::Max(0.0f, Copy.PipValueScale);
    Save->ElementModifiers.RemoveAll([&Copy](const FDailyElementModifierRow& Existing)
    {
        return Existing.RuleId == Copy.RuleId && Existing.TargetDay == Copy.TargetDay;
    });
    Save->ElementModifiers.Add(Copy);
    return Persist();
}

bool UDailyMushroomSubsystem::ClearElementModifiersForDay(const int32 TargetDay)
{
    if (!Save || TargetDay < 1)
    {
        return false;
    }
    Save->ElementModifiers.RemoveAll([TargetDay](const FDailyElementModifierRow& Modifier)
    {
        return Modifier.TargetDay == TargetDay;
    });
    return Persist();
}

FString UDailyMushroomSubsystem::GetDayStatusText() const
{
    int32 AvailableCount = 0;
    for (const FDailyMushroomCardRow& Definition : GetDailyMushrooms())
    {
        AvailableCount += Definition.bAvailable ? 1 : 0;
    }
    return FString::Printf(TEXT("第 %d 天 · 今日可采 %d 种蘑菇"), CurrentDay, AvailableCount);
}
