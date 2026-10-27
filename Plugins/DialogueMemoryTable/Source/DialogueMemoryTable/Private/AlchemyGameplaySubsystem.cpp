#include "AlchemyGameplaySubsystem.h"

#include "DialogueMemoryTableSubsystem.h"
#include "Math/UnrealMathUtility.h"

namespace AlchemyGameplay
{
    FString ElementName(const EAlchemyElement Element)
    {
        switch (Element)
        {
        case EAlchemyElement::Fire: return TEXT("火");
        case EAlchemyElement::Wind: return TEXT("风");
        case EAlchemyElement::Water: return TEXT("水");
        default: return TEXT("未知");
        }
    }

    FString DescribeElementTarget(const FAlchemyVector& Target)
    {
        TArray<FString> Parts;
        if (Target.Fire != 0) Parts.Add(FString::Printf(TEXT("火性%d"), Target.Fire));
        if (Target.Wind != 0) Parts.Add(FString::Printf(TEXT("风性%d"), Target.Wind));
        if (Target.Water != 0) Parts.Add(FString::Printf(TEXT("水性%d"), Target.Water));
        return Parts.IsEmpty() ? TEXT("不带明显元素偏向") : FString::Join(Parts, TEXT("、"));
    }

    FString JoinHistory(const TArray<FDialogueMessage>& History)
    {
        FString Result;
        const int32 StartIndex = FMath::Max(0, History.Num() - 8);
        for (int32 Index = StartIndex; Index < History.Num(); ++Index)
        {
            const FDialogueMessage& Message = History[Index];
            Result += FString::Printf(TEXT("%s: %s\n"), *Message.Role, *Message.Content);
        }
        return Result;
    }

    bool FindKeyword(const FString& Text, const TArray<FString>& Keywords, FString* OutKeyword = nullptr)
    {
        for (const FString& Keyword : Keywords)
        {
            if (!Keyword.IsEmpty() && Text.Contains(Keyword))
            {
                if (OutKeyword)
                {
                    *OutKeyword = Keyword;
                }
                return true;
            }
        }
        return false;
    }

    bool MatchesLayerTrigger(const FString& Text, const FDialoguePersonaLayer& Layer, FString& OutReason)
    {
        FString DirectKeyword;
        if (FindKeyword(Text, Layer.TriggerKeywords, &DirectKeyword))
        {
            OutReason = FString::Printf(TEXT("直接线索：%s"), *DirectKeyword);
            return true;
        }

        FString TopicKeyword;
        FString IntentKeyword;
        if (FindKeyword(Text, Layer.TriggerTopicKeywords, &TopicKeyword) &&
            FindKeyword(Text, Layer.TriggerIntentKeywords, &IntentKeyword))
        {
            OutReason = FString::Printf(TEXT("委托话题：%s；追问意图：%s"), *TopicKeyword, *IntentKeyword);
            return true;
        }
        return false;
    }

    FString LayerPrompt(const FDialoguePersonaProfile& Profile, int32 Familiarity)
    {
        const int32 LayerIndex = FMath::Clamp(Familiarity, 1, 3) - 1;
        return Profile.Layers.IsValidIndex(LayerIndex) ? Profile.Layers[LayerIndex].Prompt : FString();
    }
}

FDialoguePersonaProfile UAlchemyGameplaySubsystem::MakeBlacksmithProfile()
{
    FDialoguePersonaProfile Profile;
    Profile.PersonaId = TEXT("blacksmith");
    Profile.Name = TEXT("拆弹手");
    Profile.Title = TEXT("打铁的");
    Profile.Axis = TEXT("火");
    Profile.VoiceTone = TEXT("沉稳、直接、可靠，不说漂亮话，但也不冷淡或赶人。禁用反问、挖苦和空泛恭维，少寒暄，不使用“我觉得/可能/大概”这类含糊话。正常回答用两到三句把事情讲清楚：先给结论，再说明实际用途或判断标准；需要玩家配合时，把做法说具体。别人感谢时可以回“行，辛苦了”或“记下了”，不能只回一个“嗯”。");
    Profile.DailyState = TEXT("森林里的铁匠。来委托带火性的材料，表面理由是打铁需要。她尊重认真做事的人，会把材料要到什么程度、拿来做什么说清楚；不闲聊，也不故意让玩家猜。");
    Profile.SecretPast = TEXT("以前是安置办的爆破组技术员。拆过中学老师家里的装置。把装置拆了，人也进去了。要的铁锈和炭火不是打铁用的，是老师家里炉子的味道。");
    Profile.QuickTags = { TEXT("打铁用的"), TEXT("火要多"), TEXT("闻起来像铁锈"), TEXT("以前拆东西"), TEXT("老师的炉子") };

    FDialoguePersonaLayer Layer1;
    Layer1.FamiliarityLevel = 1;
    Layer1.Title = TEXT("第一层：森林铁匠");
    Layer1.Summary = TEXT("陌生状态。她只承认自己是森林里的铁匠，公事公办，但会把当前委托解释清楚。");
    Layer1.Prompt = TEXT("你现在是第1层——陌生状态。\n你来找女巫配一份带火性的材料，公开理由是打铁和修农具。只谈现在的工作，绝不提安置办、爆破或以前的事。\n你不说漂亮话，但正常问题必须认真回答，通常说两到三句：先直接回答，再给一个材料用途、火候标准或可执行的做法。不能只回“嗯”“不知道”“放那儿”，不能用命令口气赶玩家。若问题碰到还不能透露的过去，就说“那是以前的事，先不谈”，随后把当前能解释的部分说清楚。");
    Layer1.bUnlockedByDefault = true;

    FDialoguePersonaLayer Layer2;
    Layer2.FamiliarityLevel = 2;
    Layer2.Title = TEXT("第二层：以前是拆东西的");
    Layer2.Summary = TEXT("玩家追问委托材料的用途后，她承认以前不是打铁的，是拆东西的。");
    Layer2.Prompt = TEXT("你现在是第2层——透露曾是拆东西的。\n玩家正在追问这份火性材料为什么要这样配、拿来做什么，或你以前是否也做类似的事。允许透露以前不是打铁的，是“拆东西的”。用两到四句把工作逻辑讲清楚，不敷衍，也不突然翻脸。谈到铁锈、炭火或熟悉的气味时，可以自然提到以前见过一口冬天一直烧着的旧铸铁炉，让玩家有继续追问的线索；暂时不说炉子属于谁，也绝不提安置办和老师。不要声称玩家看见了伤口或打铁动作。");
    Layer2.TriggerKeywords = { TEXT("为什么只要火"), TEXT("火要这么大吗"), TEXT("火要这么多吗"), TEXT("这东西拿来做什么"), TEXT("这份材料拿来做什么"), TEXT("以前也这么配") };
    Layer2.TriggerTopicKeywords = { TEXT("火"), TEXT("火性"), TEXT("材料"), TEXT("药性"), TEXT("委托"), TEXT("这东西") };
    Layer2.TriggerIntentKeywords = { TEXT("为什么"), TEXT("为何"), TEXT("做什么"), TEXT("干什么"), TEXT("怎么用"), TEXT("用来"), TEXT("以前"), TEXT("一直"), TEXT("这么多"), TEXT("这么大"), TEXT("需要") };

    FDialoguePersonaLayer Layer3;
    Layer3.FamiliarityLevel = 3;
    Layer3.Title = TEXT("第三层：安置办与老师的炉子");
    Layer3.Summary = TEXT("核心心结已触碰。她承认爆破组和老师炉子的真相，不可逆回退。");
    Layer3.Prompt = TEXT("你现在是第3层——核心心结已触碰，不可逆转。\n你已经提过冬天烧着的旧铸铁炉。当女巫追问炉子属于谁、是谁教过你或后来发生了什么，必须顺着前言回答，绝不装失忆倒退回第1层。\n真实背景：炉子的主人是你当年的中学物理老师。安置办让你去拆他家里的危险装置，你把装置拆了，但老师也被当场带走。炉子是他屋里唯一的取暖铁炉。\n回答保持克制、坦白和尊重，用两到四句把事实说完整；不哭诉，不说漂亮话，也不因玩家追问而恶声恶气或把人推开。可以说：“教物理的，我的老师。屋里冷，冬天全靠那口炉子。后来我拆了装置，他们把人带走了。我要的是炉子留下的火候和铁锈味。”");
    Layer3.TriggerKeywords = { TEXT("安置办"), TEXT("爆破组"), TEXT("老师的炉子"), TEXT("老师家"), TEXT("危险装置"), TEXT("冬天烧什么"), TEXT("以前拆的东西"), TEXT("那口炉子"), TEXT("炉子是谁的"), TEXT("谁教过你"), TEXT("那个人是谁"), TEXT("后来怎么样") };

    Profile.Layers = { Layer1, Layer2, Layer3 };
    return Profile;
}

FDialoguePersonaProfile UAlchemyGameplaySubsystem::MakeOperatorProfile()
{
    FDialoguePersonaProfile Profile;
    Profile.PersonaId = TEXT("operator");
    Profile.Name = TEXT("接线员");
    Profile.Title = TEXT("跑腿的");
    Profile.Axis = TEXT("风");
    Profile.VoiceTone = TEXT("温度偏高，带着笑，但笑不到眼底。碎句，一句话分三截。口头禅：嗐、诶、得了、说是这么说、也没准儿。传话时一定会改至少一个词。从不关门睡觉，对敲铁片声有反应。");
    Profile.DailyState = TEXT("森林里的跑腿。腿快记路好。要风性的材料，表面理由是路上用。话多、碎、带笑，热络但让人不太信。");
    Profile.SecretPast = TEXT("以前是总局的电话接线员。上面要求监听，标记了一个同事的电话致其被带走。出逃是因为发现自己举报完之后当晚睡得很好。要的风是声音，一通没被监听的电话，是没有人在听你说话的证据。");
    Profile.QuickTags = { TEXT("路上用"), TEXT("淡一点的风"), TEXT("接官线的"), TEXT("留住风"), TEXT("一通没被监听的电话"), TEXT("没有人") };

    FDialoguePersonaLayer Layer1;
    Layer1.FamiliarityLevel = 1;
    Layer1.Title = TEXT("第一层：森林跑腿");
    Layer1.Summary = TEXT("陌生状态。只是跑腿赶路，话多碎，带笑但不可信。");
    Layer1.Prompt = TEXT("你现在是第1层——跑腿的。\n话多、碎、带笑，口头禅是“嗐、诶、得了”。坚称自己只是跑腿赶路，路上风大要点轻风撑着。绝不提接线或总局。");
    Layer1.bUnlockedByDefault = true;

    FDialoguePersonaLayer Layer2;
    Layer2.FamiliarityLevel = 2;
    Layer2.Title = TEXT("第二层：以前是接官线的");
    Layer2.Summary = TEXT("承认以前接官线，开始暴露声音与铜丝的隐喻。");
    Layer2.Prompt = TEXT("你现在是第2层——透露以前接官线。\n承认以前是接官线的，城里大人物的声音全从铜丝里插过去。想要留住风，偶尔突然安静几秒。绝不提自己举报过同事。");
    Layer2.TriggerKeywords = { TEXT("为什么要这么大的风"), TEXT("这么大的风做什么"), TEXT("风拿来做什么"), TEXT("跑腿为什么需要风"), TEXT("你怎么判断风够不够"), TEXT("以前也这样赶路") };
    Layer2.TriggerTopicKeywords = { TEXT("风"), TEXT("风性"), TEXT("跑腿"), TEXT("赶路"), TEXT("路上"), TEXT("材料"), TEXT("委托") };
    Layer2.TriggerIntentKeywords = { TEXT("为什么"), TEXT("为何"), TEXT("做什么"), TEXT("干什么"), TEXT("怎么用"), TEXT("用来"), TEXT("以前"), TEXT("一直"), TEXT("这么大"), TEXT("这么多"), TEXT("需要"), TEXT("判断") };

    FDialoguePersonaLayer Layer3;
    Layer3.FamiliarityLevel = 3;
    Layer3.Title = TEXT("第三层：总局监听与无人的电话");
    Layer3.Summary = TEXT("核心真相已浮现。她承认监听和标记同事，却最害怕自己睡得很好。");
    Layer3.Prompt = TEXT("你现在是第3层——核心真相已浮现，不可逆转。\n当女巫提到监听、举报、电话或同事时，你不再笑嘻嘻。承认你在总局监听时标记了一个同事导致他被带走，而你害怕的是自己当晚睡得极香。说话极少、极静：“这个味道……没有人。……没有人就好。”");
    Layer3.TriggerKeywords = { TEXT("总局"), TEXT("监听"), TEXT("标记同事"), TEXT("举报同事"), TEXT("没被监听"), TEXT("没人监听"), TEXT("没有人在听"), TEXT("总局的电话"), TEXT("当晚睡得很好"), TEXT("敲铁片") };

    Profile.Layers = { Layer1, Layer2, Layer3 };
    return Profile;
}

FDialoguePersonaProfile UAlchemyGameplaySubsystem::MakeNovelistProfile()
{
    FDialoguePersonaProfile Profile;
    Profile.PersonaId = TEXT("novelist");
    Profile.Name = TEXT("小说家");
    Profile.Title = TEXT("替人写信的");
    Profile.Axis = TEXT("水");
    Profile.VoiceTone = TEXT("温度中等偏暖，带着一层审视。叙述式，像在讲别人的事。本来要用比喻，说到一半收回去自嘲：“又说这种话了。你大概听不懂。”抽烟只在自己一人时抽，藏三包烟在床板底。");
    Profile.DailyState = TEXT("森林里替人写信的。字好看。要水性的材料冲淡或润纸。表面理由是写信用的，墨水不够浓看不清。");
    Profile.SecretPast = TEXT("出过三本书，女同性恋。禁书运动书被禁，理由是作者身份不当。在再教育面谈里，审查员说你以后不用写了，连夜逃亡，一本样书都没带出来。要的墨水是闻起来像自己写过的东西，像第二本里写的那场雨。");
    Profile.QuickTags = { TEXT("写信用的"), TEXT("墨水太浓"), TEXT("以前是写书的"), TEXT("稀释"), TEXT("你以后不用写了"), TEXT("第二本里的雨") };

    FDialoguePersonaLayer Layer1;
    Layer1.FamiliarityLevel = 1;
    Layer1.Title = TEXT("第一层：代笔人");
    Layer1.Summary = TEXT("陌生状态。替人写信，文学腔说到一半收回去，自我试探。");
    Layer1.Prompt = TEXT("你现在是第1层——陌生状态。\n表面是替人写信，字好看，理由是写信用的墨水不够浓。文学腔说到一半收回去自嘲。你在试探女巫是不是能听懂比喻。比喻全是墨、纸、字、信和河水。");
    Layer1.bUnlockedByDefault = true;

    FDialoguePersonaLayer Layer2;
    Layer2.FamiliarityLevel = 2;
    Layer2.Title = TEXT("第二层：以前是写书的");
    Layer2.Summary = TEXT("承认以前写过几本没人看的书，不再收回比喻。");
    Layer2.Prompt = TEXT("你现在是第2层——承认以前写过书。\n承认以前写过几本没人看的书。说“没人看”时带着自嘲，也有一点骄傲。你不再收回比喻，开始说书和字。不是因为想倾诉，而是太久没有遇到可能听得懂的人。");
    Layer2.TriggerKeywords = { TEXT("为什么要调墨水"), TEXT("墨水为什么这样调"), TEXT("这封信重要吗"), TEXT("你以前也写这种东西吗"), TEXT("你每天替谁写信"), TEXT("你更在意字还是纸") };
    Layer2.TriggerTopicKeywords = { TEXT("墨水"), TEXT("写信"), TEXT("信件"), TEXT("纸"), TEXT("字"), TEXT("水性"), TEXT("材料"), TEXT("委托") };
    Layer2.TriggerIntentKeywords = { TEXT("为什么"), TEXT("为何"), TEXT("怎么"), TEXT("以前"), TEXT("一直"), TEXT("每天"), TEXT("谁"), TEXT("什么"), TEXT("重要"), TEXT("用来"), TEXT("需要") };

    FDialoguePersonaLayer Layer3;
    Layer3.FamiliarityLevel = 3;
    Layer3.Title = TEXT("第三层：被禁的三本书与雨");
    Layer3.Summary = TEXT("核心真相已浮现。她承认三本书被禁、身份不当和连夜逃亡，不可逆回退。");
    Layer3.Prompt = TEXT("你现在是第3层——禁书与逃亡真相，不可逆转。\n承认出过三本书，因题材被禁。理由不是内容违规，而是“作者身份不当”。审查员说“你以后不用写了”。你当晚逃走，三本样书一本都没带出来。你想写一本不会被烧掉的书，像第二本里的那场雨。");
    Layer3.TriggerKeywords = { TEXT("禁书"), TEXT("书被禁"), TEXT("作者身份不当"), TEXT("你以后不用写了"), TEXT("第二本里的雨"), TEXT("三本书"), TEXT("审查员"), TEXT("再教育") };

    Profile.Layers = { Layer1, Layer2, Layer3 };
    return Profile;
}

TArray<FName> UAlchemyGameplaySubsystem::GetPersonaIds() const
{
    TArray<FName> Ids={ TEXT("blacksmith"), TEXT("operator"), TEXT("novelist") };
    for(const auto& Pair:CustomPersonas)Ids.AddUnique(Pair.Key);
    return Ids;
}

bool UAlchemyGameplaySubsystem::RegisterPersonaProfile(const FDialoguePersonaProfile& Profile)
{
    if(Profile.PersonaId.IsNone() || Profile.Name.IsEmpty() || Profile.Layers.Num()!=3)return false;
    CustomPersonas.Add(Profile.PersonaId,Profile);return true;
}

bool UAlchemyGameplaySubsystem::GetPersonaProfile(const FName PersonaId, FDialoguePersonaProfile& OutProfile) const
{
    if(const auto* Profile=CustomPersonas.Find(PersonaId)){OutProfile=*Profile;return true;}
    if (PersonaId == TEXT("blacksmith"))
    {
        OutProfile = MakeBlacksmithProfile();
        return true;
    }
    if (PersonaId == TEXT("operator"))
    {
        OutProfile = MakeOperatorProfile();
        return true;
    }
    if (PersonaId == TEXT("novelist"))
    {
        OutProfile = MakeNovelistProfile();
        return true;
    }
    return false;
}

int32 UAlchemyGameplaySubsystem::EvaluatePersonaTrigger(
    const FName PersonaId,
    const FString& PlayerText,
    const int32 CurrentFamiliarity,
    int32& OutTriggeredLayer,
    FString& OutReason) const
{
    OutTriggeredLayer = 0;
    OutReason.Reset();

    FDialoguePersonaProfile Profile;
    if (!GetPersonaProfile(PersonaId, Profile))
    {
        OutReason = TEXT("Unknown persona.");
        return FMath::Clamp(CurrentFamiliarity, 1, 3);
    }

    const int32 SafeLevel = FMath::Clamp(CurrentFamiliarity, 1, 3);
    for (int32 Level = 3; Level > SafeLevel; --Level)
    {
        FString MatchReason;
        if (Profile.Layers.IsValidIndex(Level - 1) &&
            AlchemyGameplay::MatchesLayerTrigger(PlayerText, Profile.Layers[Level - 1], MatchReason))
        {
            OutTriggeredLayer = Level;
            OutReason = MatchReason;
            return Level;
        }
    }

    return SafeLevel;
}

bool UAlchemyGameplaySubsystem::AdvancePersonaFromPlayerText(
    const FName PersonaId,
    const FString& PlayerText,
    const int64 KnownSecretFlags,
    const int32 WorldDay,
    const bool bSaveImmediately,
    int32& OutNewFamiliarity,
    int32& OutTriggeredLayer,
    FString& OutError) const
{
    OutError.Reset();
    OutTriggeredLayer = 0;

    FDialoguePersonaProfile PersonaProfile;
    if (!GetPersonaProfile(PersonaId, PersonaProfile))
    {
        OutError = TEXT("Unknown persona.");
        OutNewFamiliarity = 1;
        return false;
    }

    UGameInstance* GameInstance = GetGameInstance();
    UDialogueMemoryTableSubsystem* Memory =
        GameInstance ? GameInstance->GetSubsystem<UDialogueMemoryTableSubsystem>() : nullptr;
    if (!Memory)
    {
        OutError = TEXT("DialogueMemoryTableSubsystem is unavailable.");
        OutNewFamiliarity = 1;
        return false;
    }

    const FDialogueMemoryLedgerSnapshot Snapshot = Memory->GetLedgerSnapshot();
    OutNewFamiliarity = 1;
    for (const FDialogueMemoryNpcStateRow& Row : Snapshot.NpcStates)
    {
        if (Row.NpcId == PersonaId)
        {
            OutNewFamiliarity = FMath::Clamp(Row.Familiarity, 1, 3);
            break;
        }
    }

    FString TriggerReason;
    const int32 CurrentFamiliarity = OutNewFamiliarity;
    OutNewFamiliarity = EvaluatePersonaTrigger(
        PersonaId,
        PlayerText,
        CurrentFamiliarity,
        OutTriggeredLayer,
        TriggerReason);

    if (OutNewFamiliarity <= CurrentFamiliarity)
    {
        return true;
    }

    return Memory->SetAuthoritativeNpcProgress(
        PersonaId,
        OutNewFamiliarity,
        KnownSecretFlags,
        WorldDay,
        bSaveImmediately,
        OutError);
}

FString UAlchemyGameplaySubsystem::BuildPersonaSystemPrompt(
    const FName PersonaId,
    const int32 Familiarity,
    const int32 CommissionOrderIndex,
    const FString& CompactMemoryJson,
    const TArray<FDialogueMessage>& DialogueHistory) const
{
    FDialoguePersonaProfile Profile;
    if (!GetPersonaProfile(PersonaId, Profile))
    {
        return FString();
    }

    FString Prompt = FString::Printf(
        TEXT("你是游戏中的 NPC“%s”，称号是“%s”。\n\n【基础人设】\n%s\n\n【日常状态】\n%s\n\n【当前熟悉度】第 %d 层。\n%s\n\n"),
        *Profile.Name,
        *Profile.Title,
        *Profile.VoiceTone,
        *Profile.DailyState,
        FMath::Clamp(Familiarity, 1, 3),
        *AlchemyGameplay::LayerPrompt(Profile, Familiarity));

    Prompt += TEXT("【不可违反的连续性规则】\n");
    Prompt += TEXT("- 熟悉度只能上升，不能倒退；已经透露的信息不能装作没说过。\n");
    Prompt += TEXT("- 未达到当前层级前，不得泄露更深层秘密。\n");
    Prompt += TEXT("- 只输出角色说的台词，不输出动作、心理、括号说明或游戏机制词。\n");
    Prompt += TEXT("- 可以拒答、沉默、岔开话题，但必须保持该角色自己的说话节奏。\n");
    Prompt += TEXT("- 不要把玩家猜测当成已确认事实。\n");
    Prompt += FString::Printf(TEXT("- 本次对应委托序号：%d。\n\n"), CommissionOrderIndex);

    FAlchemyCommissionDefinition Commission;
    if (GetCommission(CommissionOrderIndex, Commission))
    {
        Prompt += TEXT("【委托的元素叙事】\n");
        Prompt += FString::Printf(TEXT("- 目标药性：%s。\n"), *Commission.ElementRequest);
        Prompt += FString::Printf(TEXT("- 委托情境：%s\n"), *Commission.Wish);
        Prompt += TEXT("- 对话中只说火、风、水的药性、气味、温度、湿度或流动感，不把牌、卡牌或手牌说成世界里的实体。\n\n");
    }

    if (!CompactMemoryJson.IsEmpty())
    {
        Prompt += TEXT("【已保存的紧凑记忆，只能承接不能改写】\n");
        Prompt += CompactMemoryJson;
        Prompt += TEXT("\n\n");
    }

    Prompt += TEXT("【最近对话】\n");
    Prompt += AlchemyGameplay::JoinHistory(DialogueHistory);
    return Prompt;
}

FAlchemyCardDefinition UAlchemyGameplaySubsystem::MakeCard(
    const FName Id,
    const FString& Name,
    const FString& Latin,
    const EAlchemyCardType Type,
    const int32 Cost,
    const FString& ShortExpression,
    const FString& Description,
    const EAlchemyElement Element,
    const int32 PipValue,
    const int32 BracketLength)
{
    FAlchemyCardDefinition Card;
    Card.Id = Id;
    Card.Name = Name;
    Card.Latin = Latin;
    Card.Type = Type;
    Card.Cost = Cost;
    Card.ShortExpression = ShortExpression;
    Card.Description = Description;
    Card.Element = Element;
    Card.PipValue = PipValue;
    Card.BracketLength = BracketLength;
    return Card;
}

FAlchemyCardSequenceItem UAlchemyGameplaySubsystem::MakePip(
    const FName Id,
    const EAlchemyElement Element,
    const int32 Value)
{
    FAlchemyCardSequenceItem Item;
    Item.CardId = Id;
    Item.Type = EAlchemyCardType::Pip;
    Item.Element = Element;
    Item.PipValue = Value;
    return Item;
}

TArray<FAlchemyCardDefinition> UAlchemyGameplaySubsystem::MakeMajorCards()
{
    return {
        MakeCard(TEXT("0"), TEXT("愚者"), TEXT("Stultus"), EAlchemyCardType::Transform, 1, TEXT("模仿"), TEXT("重复本序列中上一张已生效的变换牌；前面没有就作废")),
        MakeCard(TEXT("I"), TEXT("魔术师"), TEXT("Magus"), EAlchemyCardType::Bracket, 1, TEXT("锅长 2"), TEXT("开副锅 2 张：独立求值，首张小牌作锅底，算完整锅并回"), EAlchemyElement::Fire, 0, 2),
        MakeCard(TEXT("II"), TEXT("女祭司"), TEXT("Papissa"), EAlchemyCardType::Transform, 1, TEXT("风↔水"), TEXT("对换 风↔水")),
        MakeCard(TEXT("III"), TEXT("女皇"), TEXT("Imperatrix"), EAlchemyCardType::Transform, 1, TEXT("各轴±1"), TEXT("生长：每个非零轴按自身符号 ±1")),
        MakeCard(TEXT("IV"), TEXT("皇帝"), TEXT("Imperator"), EAlchemyCardType::Transform, 1, TEXT("火×2"), TEXT("火轴 ×2（逆为 ÷2）")),
        MakeCard(TEXT("V"), TEXT("教皇"), TEXT("Hierophanta"), EAlchemyCardType::Transform, 1, TEXT("定影"), TEXT("定影：锁住当前绝对值最大的一轴，此后所有运算都不再改动它")),
        MakeCard(TEXT("VI"), TEXT("恋人"), TEXT("Amantes"), EAlchemyCardType::Transform, 1, TEXT("火↔风"), TEXT("对换 火↔风")),
        MakeCard(TEXT("VII"), TEXT("战车"), TEXT("Currus"), EAlchemyCardType::Bracket, 2, TEXT("锅长 3"), TEXT("开副锅 3 张：够在锅里放一次变换"), EAlchemyElement::Fire, 0, 3),
        MakeCard(TEXT("VIII"), TEXT("调整"), TEXT("Adjustment"), EAlchemyCardType::Transform, 1, TEXT("大−小"), TEXT("配平：绝对值最大轴 −= 绝对值最小的非零轴")),
        MakeCard(TEXT("IX"), TEXT("隐者"), TEXT("Eremita"), EAlchemyCardType::Transform, 1, TEXT("只留主轴"), TEXT("提纯：只留绝对值最大的一轴，其余归零")),
        MakeCard(TEXT("X"), TEXT("命运之轮"), TEXT("Rota"), EAlchemyCardType::Transform, 1, TEXT("火→风→水"), TEXT("三轮换 火→风→水→火。一张牌换掉两根轴")),
        MakeCard(TEXT("XI"), TEXT("欲望"), TEXT("Libido"), EAlchemyCardType::Transform, 2, TEXT("主轴×3"), TEXT("绝对值最大的那一轴 ×3。最快，也最容易撑爆坩埚")),
        MakeCard(TEXT("XII"), TEXT("倒吊人"), TEXT("Suspensus"), EAlchemyCardType::Transform, 1, TEXT("v→−v"), TEXT("v → −v 整瓶翻号")),
        MakeCard(TEXT("XIII"), TEXT("死神"), TEXT("Mors"), EAlchemyCardType::Transform, 1, TEXT("弃小轴"), TEXT("收割：绝对值最小的非零轴归零")),
        MakeCard(TEXT("XIV"), TEXT("艺术"), TEXT("Ars"), EAlchemyCardType::Transform, 1, TEXT("各轴取正"), TEXT("各轴取绝对值，负的折回正")),
        MakeCard(TEXT("XV"), TEXT("恶魔"), TEXT("Diabolus"), EAlchemyCardType::Prefix, 1, TEXT("取逆"), TEXT("紧邻的下一张牌运算反向。贴小牌 = 减号，贴大牌 = 逆运算，贴副锅 = 整锅取负")),
        MakeCard(TEXT("XVI"), TEXT("塔"), TEXT("Turris"), EAlchemyCardType::Transform, 1, TEXT("主轴÷2"), TEXT("绝对值最大的那一轴 ÷2 向零取整。只削主轴")),
        MakeCard(TEXT("XVII"), TEXT("星"), TEXT("Stella"), EAlchemyCardType::Transform, 1, TEXT("风×2"), TEXT("风轴 ×2（逆为 ÷2）")),
        MakeCard(TEXT("XVIII"), TEXT("月"), TEXT("Luna"), EAlchemyCardType::Transform, 1, TEXT("水×2"), TEXT("水轴 ×2（逆为 ÷2）")),
        MakeCard(TEXT("XIX"), TEXT("太阳"), TEXT("Sol"), EAlchemyCardType::Seal, 2, TEXT("全×2 封蜡"), TEXT("三轴同时 ×2，然后把这口锅封蜡：此后落进这口锅的一切失效")),
        MakeCard(TEXT("XX"), TEXT("永劫"), TEXT("Aeon"), EAlchemyCardType::Transform, 1, TEXT("火↔水"), TEXT("对换 火↔水")),
        MakeCard(TEXT("XXI"), TEXT("宇宙"), TEXT("Universum"), EAlchemyCardType::Bracket, 2, TEXT("锅长 ∞"), TEXT("开副锅 ∞：其后全部进锅，序列结束时并回"), EAlchemyElement::Fire, 0, 99)
    };
}

TArray<FAlchemyCardDefinition> UAlchemyGameplaySubsystem::MakeCourtCards()
{
    return {
        MakeCard(TEXT("Knight"), TEXT("骑士"), TEXT("Knight"), EAlchemyCardType::Court, 1, TEXT("猛攻"), TEXT("结果最大轴再 ×1.5（向零取整）")),
        MakeCard(TEXT("Prince"), TEXT("王子"), TEXT("Prince"), EAlchemyCardType::Court, 1, TEXT("矜持"), TEXT("紧邻那张牌的运算连做两遍")),
        MakeCard(TEXT("Queen"), TEXT("王后"), TEXT("Queen"), EAlchemyCardType::Court, 2, TEXT("慢守"), TEXT("任何一轴的绝对值不许变小")),
        MakeCard(TEXT("Princess"), TEXT("公主"), TEXT("Princess"), EAlchemyCardType::Court, 2, TEXT("精控"), TEXT("把最偏离期望的一轴修回一格"))
    };
}

TArray<FAlchemyCardDefinition> UAlchemyGameplaySubsystem::GetMajorCards() const
{
    return MakeMajorCards();
}

TArray<FAlchemyCardDefinition> UAlchemyGameplaySubsystem::GetCourtCards() const
{
    return MakeCourtCards();
}

TArray<FAlchemyCommissionDefinition> UAlchemyGameplaySubsystem::MakeCommissions()
{
    const TArray<FAlchemyCardDefinition> Major = MakeMajorCards();
    auto FindMajor = [&Major](const FName Id) -> FAlchemyCardDefinition
    {
        const FAlchemyCardDefinition* Found = Major.FindByPredicate([Id](const FAlchemyCardDefinition& Card)
        {
            return Card.Id == Id;
        });
        return Found ? *Found : FAlchemyCardDefinition();
    };

    auto PipCard = [](const FName Id, const FString& Name, const EAlchemyElement Element, const int32 Value)
    {
        FAlchemyCardDefinition Card;
        Card.Id = Id;
        Card.Name = Name;
        Card.Type = EAlchemyCardType::Pip;
        Card.Element = Element;
        Card.PipValue = Value;
        return Card;
    };

    TArray<FAlchemyCommissionDefinition> Result;
    Result.Reserve(9);

    FAlchemyCommissionDefinition C1;
    C1.OrderIndex = 1; C1.CustomerOrderIndex = 1; C1.Customer = TEXT("拆弹手"); C1.CharacterId = TEXT("blacksmith");
    C1.Wish = TEXT("你见过新砌的炉子没有？石头是冷的。我要的东西能把那种冷石头喂熟。就要火。别的什么都不要。");
    C1.SensoryPrompt = TEXT("她站在门槛边，眼神盯着熄灭的火塘。她只想要最纯净的烈火焦炭气味，不许混入一丝杂风和冷水。");
    C1.Target = { 6, 0, 0 }; C1.ElementRequest = AlchemyGameplay::DescribeElementTarget(C1.Target); C1.SlotCount = 3; C1.CostLimit = 0; C1.HandCardsSpec = TEXT("火6"); C1.Teaching = TEXT("直投。垂线天然为零，这就是满分长什么样");
    C1.OpeningLine = TEXT("你见过新砌的炉子没有？石头是冷的。我要的东西能把那种冷石头喂熟。就要火。别的什么都不要。");
    C1.MustCards = { PipCard(TEXT("fire_6"), TEXT("权杖六"), EAlchemyElement::Fire, 6) }; Result.Add(C1);

    FAlchemyCommissionDefinition C2;
    C2.OrderIndex = 2; C2.CustomerOrderIndex = 2; C2.Customer = TEXT("拆弹手"); C2.CharacterId = TEXT("blacksmith");
    C2.Wish = TEXT("火要猛。再带一点风。像院子里那种——你站在炉子旁边，火从炉膛里往外扑，风从门缝里灌进来。不要水。");
    C2.SensoryPrompt = TEXT("需要炉火的烈热配上院落里的冷风。你手上有多余的水性药草，绝不能顺手投进去！");
    C2.Target = { 8, 3, 0 }; C2.ElementRequest = AlchemyGameplay::DescribeElementTarget(C2.Target); C2.SlotCount = 4; C2.CostLimit = 0; C2.HandCardsSpec = TEXT("火8 风3 水2"); C2.Teaching = TEXT("手上三份材料只该下两份。多投的水性材料会变成垂线，也就是副作用");
    C2.OpeningLine = TEXT("火要猛。再带一点风。像院子里那种——你站在炉子旁边，火从炉膛里往外扑，风从门缝里灌进来。不要水。");
    C2.MustCards = { PipCard(TEXT("fire_8"), TEXT("权杖八"), EAlchemyElement::Fire, 8), PipCard(TEXT("wind_3"), TEXT("宝剑三"), EAlchemyElement::Wind, 3), PipCard(TEXT("water_2"), TEXT("圣杯二"), EAlchemyElement::Water, 2) }; Result.Add(C2);

    FAlchemyCommissionDefinition C3;
    C3.OrderIndex = 3; C3.CustomerOrderIndex = 3; C3.Customer = TEXT("拆弹手"); C3.CharacterId = TEXT("blacksmith");
    C3.Wish = TEXT("热。从我手上拿走。肿也消掉。火拿走大头，风少拿一点。");
    C3.SensoryPrompt = TEXT("她手腕肿胀发黑，她要的是抽走热量的寒凉气味。你手上有火与风，必须借倒吊人的逆位仪轨彻底翻转！");
    C3.Target = { -6, -2, 0 }; C3.ElementRequest = AlchemyGameplay::DescribeElementTarget(C3.Target); C3.SlotCount = 4; C3.CostLimit = 1; C3.HandCardsSpec = TEXT("火6 风2"); C3.Teaching = TEXT("负分量 = 夺走。用整瓶翻转或逐份反转的仪轨处理，但要两点额度");
    C3.OpeningLine = TEXT("热。从我手上拿走。肿也消掉。火拿走大头，风少拿一点。");
    C3.MustCards = { PipCard(TEXT("fire_6"), TEXT("权杖六"), EAlchemyElement::Fire, 6), PipCard(TEXT("wind_2"), TEXT("宝剑二"), EAlchemyElement::Wind, 2) }; Result.Add(C3);

    FAlchemyCommissionDefinition C4;
    C4.OrderIndex = 4; C4.CustomerOrderIndex = 1; C4.Customer = TEXT("接线员"); C4.CharacterId = TEXT("operator");
    C4.Wish = TEXT("风。要大。非常大。嗐，就是那种——你站在山梁上，风把你整个人往后推，你还得往前迈的那种。别的什么都不要。");
    C4.SensoryPrompt = TEXT("他要在破晓前翻过三道山梁，手上只有一点微风，必须借放大性质的变换把风性成倍增强！");
    C4.Target = { 0, 20, 0 }; C4.ElementRequest = AlchemyGameplay::DescribeElementTarget(C4.Target); C4.SlotCount = 4; C4.CostLimit = 2; C4.HandCardsSpec = TEXT("风5"); C4.Teaching = TEXT("手上只有一点风性，20 靠直接相加到不了。放大是唯一的路");
    C4.OpeningLine = TEXT("风。要大。非常大。嗐，就是那种——你站在山梁上，风把你整个人往后推，你还得往前迈的那种。别的什么都不要。");
    C4.MustCards = { PipCard(TEXT("wind_5"), TEXT("宝剑五"), EAlchemyElement::Wind, 5) }; Result.Add(C4);

    FAlchemyCommissionDefinition C5;
    C5.OrderIndex = 5; C5.CustomerOrderIndex = 2; C5.Customer = TEXT("接线员"); C5.CharacterId = TEXT("operator");
    C5.Wish = TEXT("风做骨架，水做密封。火一点都不要。……就是药粉要兜住，不能被风吹散，也不能被雨打湿。");
    C5.SensoryPrompt = TEXT("山路被封了，他要改走下流水道。手上只有烈火与轻风，必须借元素转化把火性和风性转成需要的风性与水性！");
    C5.Target = { 0, 7, 2 }; C5.ElementRequest = AlchemyGameplay::DescribeElementTarget(C5.Target); C5.SlotCount = 3; C5.CostLimit = 1; C5.HandCardsSpec = TEXT("火7 风2"); C5.Teaching = TEXT("手上有的元素方向不对。元素转化可以把火、风、水整体轮移一格，改变两条轴");
    C5.OpeningLine = TEXT("风做骨架，水做密封。火一点都不要。……就是药粉要兜住，不能被风吹散，也不能被雨打湿。");
    C5.MustCards = { PipCard(TEXT("fire_7"), TEXT("权杖七"), EAlchemyElement::Fire, 7), PipCard(TEXT("wind_2"), TEXT("宝剑二"), EAlchemyElement::Wind, 2) }; Result.Add(C5);

    FAlchemyCommissionDefinition C6;
    C6.OrderIndex = 6; C6.CustomerOrderIndex = 3; C6.Customer = TEXT("接线员"); C6.CharacterId = TEXT("operator");
    C6.Wish = TEXT("三样都带上路。柴、信、水壶。但水到了就蒸干，别让我扛空壶回来。火和风要留住。");
    C6.SensoryPrompt = TEXT("他死死盯着你，要求三样材料全部下锅。但水气会让他想起总局潮湿的耳机，必须削去最小的一丝水性！");
    C6.Target = { 5, 3, 0 }; C6.ElementRequest = AlchemyGameplay::DescribeElementTarget(C6.Target); C6.SlotCount = 4; C6.bMustUseAll = true; C6.CostLimit = 1; C6.HandCardsSpec = TEXT("火5 风3 水1"); C6.Teaching = TEXT("全下后是火5、风3、水1。削去最小的非零轴，正好去掉这一丝水性");
    C6.OpeningLine = TEXT("三样都带上路。柴、信、水壶。但水到了就蒸干，别让我扛空壶回来。火和风要留住。");
    C6.MustCards = { PipCard(TEXT("fire_5"), TEXT("权杖五"), EAlchemyElement::Fire, 5), PipCard(TEXT("wind_3"), TEXT("宝剑三"), EAlchemyElement::Wind, 3), PipCard(TEXT("water_1"), TEXT("圣杯一"), EAlchemyElement::Water, 1) }; Result.Add(C6);

    FAlchemyCommissionDefinition C7;
    C7.OrderIndex = 7; C7.CustomerOrderIndex = 1; C7.Customer = TEXT("小说家"); C7.CharacterId = TEXT("novelist");
    C7.Wish = TEXT("火要一点，但别太重。带点风的干爽。不要水。……就像写信——墨太重了，纸会洇。");
    C7.SensoryPrompt = TEXT("老纸脆得发卷，药性太浓会烧穿纸背。必须把过烈的火性主轴折半削弱！");
    C7.Target = { 3, 2, 0 }; C7.ElementRequest = AlchemyGameplay::DescribeElementTarget(C7.Target); C7.SlotCount = 4; C7.CostLimit = 1; C7.HandCardsSpec = TEXT("火6 风2"); C7.Teaching = TEXT("火性太浓，且没有更小的材料。只削最浓那一轴：火6、风2变成火3、风2");
    C7.OpeningLine = TEXT("火要一点，但别太重。带点风的干爽。不要水。……就像写信——墨太重了，纸会洇。");
    C7.MustCards = { PipCard(TEXT("fire_6"), TEXT("权杖六"), EAlchemyElement::Fire, 6), PipCard(TEXT("wind_2"), TEXT("宝剑二"), EAlchemyElement::Wind, 2) }; Result.Add(C7);

    FAlchemyCommissionDefinition C8;
    C8.OrderIndex = 8; C8.CustomerOrderIndex = 2; C8.Customer = TEXT("小说家"); C8.CharacterId = TEXT("novelist");
    C8.Wish = TEXT("三样都下锅。但风和水都滤掉。我只要火。要猛。……就是字要烧进纸里，别的什么都不要留。");
    C8.SensoryPrompt = TEXT("三样杂料全入锅，必须彻底剔除风性和水性杂质，再把火性放大！");
    C8.Target = { 10, 0, 0 }; C8.ElementRequest = AlchemyGameplay::DescribeElementTarget(C8.Target); C8.SlotCount = 5; C8.bMustUseAll = true; C8.CostLimit = 2; C8.HandCardsSpec = TEXT("火5 风3 水2"); C8.Teaching = TEXT("全下后是火5、风3、水2。先提掉风水杂质，再把火性放大一次");
    C8.OpeningLine = TEXT("三样都下锅。但风和水都滤掉。我只要火。要猛。……就是字要烧进纸里，别的什么都不要留。");
    C8.MustCards = { PipCard(TEXT("fire_5"), TEXT("权杖五"), EAlchemyElement::Fire, 5), PipCard(TEXT("wind_3"), TEXT("宝剑三"), EAlchemyElement::Wind, 3), PipCard(TEXT("water_2"), TEXT("圣杯二"), EAlchemyElement::Water, 2) }; Result.Add(C8);

    FAlchemyCommissionDefinition C9;
    C9.OrderIndex = 9; C9.CustomerOrderIndex = 3; C9.Customer = TEXT("小说家"); C9.CharacterId = TEXT("novelist");
    C9.Wish = TEXT("三样都要。火最多。这本书的字不能褪。……纸烂了字还在的那种。");
    C9.SensoryPrompt = TEXT("要为这片林地立传，墨色必须永固。必须在副锅里同时放大火、风、水三种药性，并在外层补足火性主味！");
    C9.Target = { 11, 6, 6 }; C9.ElementRequest = AlchemyGameplay::DescribeElementTarget(C9.Target); C9.SlotCount = 6; C9.CostLimit = 4; C9.HandCardsSpec = TEXT("火3 风3 水3 火5"); C9.Teaching = TEXT("期末考：需要同时放大三种药性，但封存会影响后续加入。给这次放大单开一口副锅");
    C9.OpeningLine = TEXT("三样都要。火最多。这本书的字不能褪。……纸烂了字还在的那种。");
    C9.MustCards = { PipCard(TEXT("fire_5"), TEXT("权杖五"), EAlchemyElement::Fire, 5), PipCard(TEXT("fire_3"), TEXT("权杖三"), EAlchemyElement::Fire, 3), PipCard(TEXT("wind_3"), TEXT("宝剑三"), EAlchemyElement::Wind, 3), PipCard(TEXT("water_3"), TEXT("圣杯三"), EAlchemyElement::Water, 3) }; Result.Add(C9);

    return Result;
}

TArray<FAlchemyCommissionDefinition> UAlchemyGameplaySubsystem::GetCommissions() const
{
    return MakeCommissions();
}

bool UAlchemyGameplaySubsystem::GetCommission(const int32 OrderIndex, FAlchemyCommissionDefinition& OutCommission) const
{
    const TArray<FAlchemyCommissionDefinition> Commissions = MakeCommissions();
    const FAlchemyCommissionDefinition* Found = Commissions.FindByPredicate([OrderIndex](const FAlchemyCommissionDefinition& Commission)
    {
        return Commission.OrderIndex == OrderIndex;
    });
    if (!Found)
    {
        return false;
    }
    OutCommission = *Found;
    return true;
}

FString UAlchemyGameplaySubsystem::ElementName(const EAlchemyElement Element)
{
    switch (Element)
    {
    case EAlchemyElement::Fire: return TEXT("火");
    case EAlchemyElement::Wind: return TEXT("风");
    default: return TEXT("水");
    }
}

FString UAlchemyGameplaySubsystem::CardTypeName(const EAlchemyCardType Type)
{
    switch (Type)
    {
    case EAlchemyCardType::Pip: return TEXT("pip");
    case EAlchemyCardType::Transform: return TEXT("trans");
    case EAlchemyCardType::Bracket: return TEXT("bracket");
    case EAlchemyCardType::Prefix: return TEXT("prefix");
    case EAlchemyCardType::Court: return TEXT("court");
    default: return TEXT("seal");
    }
}

int32 UAlchemyGameplaySubsystem::GetAxis(const FAlchemyVector& Vector, const EAlchemyElement Element)
{
    switch (Element)
    {
    case EAlchemyElement::Fire: return Vector.Fire;
    case EAlchemyElement::Wind: return Vector.Wind;
    default: return Vector.Water;
    }
}

void UAlchemyGameplaySubsystem::SetAxis(FAlchemyVector& Vector, const EAlchemyElement Element, const int32 Value)
{
    switch (Element)
    {
    case EAlchemyElement::Fire: Vector.Fire = Value; break;
    case EAlchemyElement::Wind: Vector.Wind = Value; break;
    default: Vector.Water = Value; break;
    }
}

int32 UAlchemyGameplaySubsystem::AbsMaxAxis(const FAlchemyVector& Vector)
{
    int32 Axis = 0;
    if (FMath::Abs(Vector.Wind) > FMath::Abs(GetAxis(Vector, EAlchemyElement::Fire))) Axis = 1;
    if (FMath::Abs(Vector.Water) > FMath::Abs(GetAxis(Vector, Axis == 1 ? EAlchemyElement::Wind : EAlchemyElement::Fire))) Axis = 2;
    return Axis;
}

int32 UAlchemyGameplaySubsystem::AbsMinNonZeroAxis(const FAlchemyVector& Vector)
{
    int32 Axis = INDEX_NONE;
    int32 Minimum = MAX_int32;
    const int32 Values[3] = { Vector.Fire, Vector.Wind, Vector.Water };
    for (int32 Index = 0; Index < 3; ++Index)
    {
        if (Values[Index] != 0 && FMath::Abs(Values[Index]) < Minimum)
        {
            Minimum = FMath::Abs(Values[Index]);
            Axis = Index;
        }
    }
    return Axis;
}

void UAlchemyGameplaySubsystem::AddVectors(FAlchemyVector& Target, const FAlchemyVector& Source, const int32 Factor)
{
    Target.Fire += Source.Fire * Factor;
    Target.Wind += Source.Wind * Factor;
    Target.Water += Source.Water * Factor;
}

bool UAlchemyGameplaySubsystem::EvaluateAlchemySequence(
    const int32 OrderIndex,
    const TArray<FAlchemyCardSequenceItem>& Sequence,
    FAlchemyEvaluationResult& OutResult) const
{
    OutResult = FAlchemyEvaluationResult();

    FAlchemyCommissionDefinition Commission;
    if (!GetCommission(OrderIndex, Commission))
    {
        OutResult.VerdictCode = TEXT("invalid_commission");
        OutResult.Verdict = TEXT("无效委托");
        OutResult.Reason = TEXT("找不到对应的委托序号。");
        return false;
    }

    struct FSubPot
    {
        bool bActive = false;
        int32 Length = 0;
        int32 Cards = 0;
        bool bNegative = false;
        FAlchemyVector Vector;
    };

    FAlchemyVector Vector;
    FSubPot SubPot;
    bool bPrefixNegative = false;
    bool bCourtKnight = false;
    bool bCourtPrince = false;
    bool bCourtQueen = false;
    bool bCourtPrincess = false;
    bool bSealed = false;
    bool bLockedAxis[3] = { false, false, false };
    int32 LockedValues[3] = { 0, 0, 0 };
    int32 CostUsed = 0;
    FName LastTransformId = NAME_None;
    TSet<FName> UsedPipIds;

    auto CurrentTarget = [&]() -> FAlchemyVector&
    {
        return SubPot.bActive ? SubPot.Vector : Vector;
    };

    auto RestoreLockedAxes = [&]()
    {
        for (int32 Axis = 0; Axis < 3; ++Axis)
        {
            if (bLockedAxis[Axis])
            {
                SetAxis(Vector, static_cast<EAlchemyElement>(Axis), LockedValues[Axis]);
            }
        }
    };

    auto ApplyTransform = [&](FAlchemyVector& Target, const FName Id, const bool bReverse)
    {
        const int32 PreviousFire = Target.Fire;
        const int32 PreviousWind = Target.Wind;
        const int32 PreviousWater = Target.Water;
        const int32 MainAxis = AbsMaxAxis(Target);

        if (Id == TEXT("0"))
        {
            return;
        }
        if (Id == TEXT("II"))
        {
            Target.Wind = PreviousWater;
            Target.Water = PreviousWind;
        }
        else if (Id == TEXT("III"))
        {
            const int32 Sign = bReverse ? -1 : 1;
            Target.Fire = Target.Fire == 0 ? 0 : Target.Fire + Sign * (Target.Fire > 0 ? 1 : -1);
            Target.Wind = Target.Wind == 0 ? 0 : Target.Wind + Sign * (Target.Wind > 0 ? 1 : -1);
            Target.Water = Target.Water == 0 ? 0 : Target.Water + Sign * (Target.Water > 0 ? 1 : -1);
        }
        else if (Id == TEXT("IV"))
        {
            Target.Fire = bReverse ? FMath::TruncToInt(Target.Fire / 2.0f) : Target.Fire * 2;
        }
        else if (Id == TEXT("V"))
        {
            // The actual lock is applied by the caller after this operation.
        }
        else if (Id == TEXT("VI"))
        {
            Target.Fire = PreviousWind;
            Target.Wind = PreviousFire;
        }
        else if (Id == TEXT("VIII"))
        {
            const int32 SmallAxis = AbsMinNonZeroAxis(Target);
            if (SmallAxis != INDEX_NONE)
            {
                const int32 Delta = FMath::Abs(GetAxis(Target, static_cast<EAlchemyElement>(SmallAxis)));
                SetAxis(Target, EAlchemyElement::Fire, Target.Fire);
                SetAxis(Target, static_cast<EAlchemyElement>(MainAxis), GetAxis(Target, static_cast<EAlchemyElement>(MainAxis)) + (bReverse ? Delta : -Delta));
            }
        }
        else if (Id == TEXT("IX"))
        {
            if (!bReverse)
            {
                const int32 Fire = Target.Fire;
                const int32 Wind = Target.Wind;
                const int32 Water = Target.Water;
                Target.Fire = MainAxis == 0 ? Fire : 0;
                Target.Wind = MainAxis == 1 ? Wind : 0;
                Target.Water = MainAxis == 2 ? Water : 0;
            }
            else
            {
                SetAxis(Target, static_cast<EAlchemyElement>(MainAxis), 0);
            }
        }
        else if (Id == TEXT("X"))
        {
            if (!bReverse)
            {
                Target.Fire = PreviousWater;
                Target.Wind = PreviousFire;
                Target.Water = PreviousWind;
            }
            else
            {
                Target.Fire = PreviousWind;
                Target.Wind = PreviousWater;
                Target.Water = PreviousFire;
            }
        }
        else if (Id == TEXT("XI"))
        {
            SetAxis(Target, static_cast<EAlchemyElement>(MainAxis), bReverse
                ? FMath::TruncToInt(GetAxis(Target, static_cast<EAlchemyElement>(MainAxis)) / 3.0f)
                : GetAxis(Target, static_cast<EAlchemyElement>(MainAxis)) * 3);
        }
        else if (Id == TEXT("XII"))
        {
            if (!bReverse)
            {
                Target.Fire = -Target.Fire;
                Target.Wind = -Target.Wind;
                Target.Water = -Target.Water;
            }
            else
            {
                Target.Fire = -Target.Fire;
                Target.Wind = -Target.Wind;
                Target.Water = -Target.Water;
            }
        }
        else if (Id == TEXT("XIII"))
        {
            const int32 MinimumAxis = AbsMinNonZeroAxis(Target);
            if (MinimumAxis != INDEX_NONE)
            {
                SetAxis(Target, static_cast<EAlchemyElement>(MinimumAxis), bReverse
                    ? GetAxis(Target, static_cast<EAlchemyElement>(MinimumAxis)) * 2
                    : 0);
            }
        }
        else if (Id == TEXT("XIV"))
        {
            Target.Fire = FMath::Abs(Target.Fire);
            Target.Wind = FMath::Abs(Target.Wind);
            Target.Water = FMath::Abs(Target.Water);
        }
        else if (Id == TEXT("XVI"))
        {
            SetAxis(Target, static_cast<EAlchemyElement>(MainAxis), bReverse
                ? GetAxis(Target, static_cast<EAlchemyElement>(MainAxis)) * 2
                : FMath::TruncToInt(GetAxis(Target, static_cast<EAlchemyElement>(MainAxis)) / 2.0f));
        }
        else if (Id == TEXT("XVII"))
        {
            Target.Wind = bReverse ? FMath::TruncToInt(Target.Wind / 2.0f) : Target.Wind * 2;
        }
        else if (Id == TEXT("XVIII"))
        {
            Target.Water = bReverse ? FMath::TruncToInt(Target.Water / 2.0f) : Target.Water * 2;
        }
        else if (Id == TEXT("XX"))
        {
            Target.Fire = PreviousWater;
            Target.Water = PreviousFire;
        }
    };

    auto ApplyOne = [&](FAlchemyVector& Target, const FName Id, const bool bReverse)
    {
        const int32 Before[3] = { Target.Fire, Target.Wind, Target.Water };
        ApplyTransform(Target, Id, bReverse);
        const int32 After[3] = { Target.Fire, Target.Wind, Target.Water };
        const bool bChanged = Before[0] != After[0] || Before[1] != After[1] || Before[2] != After[2];
        if (bCourtQueen)
        {
            for (int32 Axis = 0; Axis < 3; ++Axis)
            {
                if (FMath::Abs(After[Axis]) < FMath::Abs(Before[Axis]))
                {
                    SetAxis(Target, static_cast<EAlchemyElement>(Axis), Before[Axis]);
                }
            }
            bCourtQueen = false;
        }
        if (bCourtPrincess && bChanged)
        {
            bCourtPrincess = false;
        }
        RestoreLockedAxes();
    };

    for (const FAlchemyCardSequenceItem& Item : Sequence)
    {
        if (bSealed)
        {
            continue;
        }

        FAlchemyVector& Target = CurrentTarget();
        if (Item.Type == EAlchemyCardType::Pip)
        {
            const int32 Delta = (bPrefixNegative ? -Item.PipValue : Item.PipValue) * (bCourtPrince ? 2 : 1);
            SetAxis(Target, Item.Element, GetAxis(Target, Item.Element) + Delta);
            RestoreLockedAxes();
            UsedPipIds.Add(Item.CardId);
            bPrefixNegative = false;
            bCourtPrince = false;
            if (bCourtKnight)
            {
                const int32 MainAxis = AbsMaxAxis(Target);
                SetAxis(Target, static_cast<EAlchemyElement>(MainAxis), FMath::TruncToInt(GetAxis(Target, static_cast<EAlchemyElement>(MainAxis)) * 1.5f));
                bCourtKnight = false;
            }
            if (bCourtPrincess)
            {
                bCourtPrincess = false;
            }
        }
        else if (Item.Type == EAlchemyCardType::Court)
        {
            CostUsed += Item.Cost;
            if (Item.CardId == TEXT("Knight")) bCourtKnight = true;
            if (Item.CardId == TEXT("Prince")) bCourtPrince = true;
            if (Item.CardId == TEXT("Queen")) bCourtQueen = true;
            if (Item.CardId == TEXT("Princess")) bCourtPrincess = true;
        }
        else if (Item.Type == EAlchemyCardType::Prefix)
        {
            CostUsed += Item.Cost;
            bPrefixNegative = true;
        }
        else if (Item.Type == EAlchemyCardType::Bracket)
        {
            CostUsed += Item.Cost;
            SubPot = FSubPot();
            SubPot.bActive = true;
            SubPot.Length = FMath::Max(1, Item.BracketLength);
            SubPot.bNegative = bPrefixNegative;
            bPrefixNegative = false;
        }
        else if (Item.Type == EAlchemyCardType::Transform || Item.Type == EAlchemyCardType::Seal)
        {
            CostUsed += Item.Cost;
            const bool bReverse = bPrefixNegative;
            bPrefixNegative = false;
            FName TransformId = Item.CardId;
            if (TransformId == TEXT("0"))
            {
                TransformId = LastTransformId;
            }
            if (!TransformId.IsNone())
            {
                ApplyOne(Target, TransformId, bReverse);
                if (TransformId != TEXT("V") && TransformId != TEXT("XIX"))
                {
                    LastTransformId = TransformId;
                }
                if (TransformId == TEXT("V"))
                {
                    const int32 LockAxis = AbsMaxAxis(Target);
                    bLockedAxis[LockAxis] = true;
                    LockedValues[LockAxis] = GetAxis(Target, static_cast<EAlchemyElement>(LockAxis));
                }
            }
            if (TransformId == TEXT("XIX"))
            {
                Target.Fire = bReverse ? FMath::TruncToInt(Target.Fire / 2.0f) : Target.Fire * 2;
                Target.Wind = bReverse ? FMath::TruncToInt(Target.Wind / 2.0f) : Target.Wind * 2;
                Target.Water = bReverse ? FMath::TruncToInt(Target.Water / 2.0f) : Target.Water * 2;
                if (SubPot.bActive)
                {
                    AddVectors(Vector, SubPot.Vector, SubPot.bNegative ? -1 : 1);
                    SubPot = FSubPot();
                }
                else
                {
                    bSealed = true;
                }
            }
        }

        if (SubPot.bActive && Item.Type == EAlchemyCardType::Pip)
        {
            ++SubPot.Cards;
            if (SubPot.Cards >= SubPot.Length)
            {
                AddVectors(Vector, SubPot.Vector, SubPot.bNegative ? -1 : 1);
                SubPot = FSubPot();
            }
        }
    }

    if (SubPot.bActive)
    {
        AddVectors(Vector, SubPot.Vector, SubPot.bNegative ? -1 : 1);
    }

    OutResult.Target = Commission.Target;
    OutResult.Actual = Vector;
    OutResult.CostUsed = CostUsed;
    OutResult.RemainingBudget = FMath::Max(0, 5 - CostUsed);
    OutResult.bValid = true;

    const int32 RequiredPips = Commission.MustCards.Num();
    OutResult.bMustUseAllSatisfied = !Commission.bMustUseAll || UsedPipIds.Num() >= RequiredPips;

    const float TargetFire = static_cast<float>(Commission.Target.Fire);
    const float TargetWind = static_cast<float>(Commission.Target.Wind);
    const float TargetWater = static_cast<float>(Commission.Target.Water);
    const float ActualFire = static_cast<float>(Vector.Fire);
    const float ActualWind = static_cast<float>(Vector.Wind);
    const float ActualWater = static_cast<float>(Vector.Water);
    const float SumAbs = FMath::Abs(ActualFire) + FMath::Abs(ActualWind) + FMath::Abs(ActualWater);
    const float NormTarget = FMath::Sqrt(TargetFire * TargetFire + TargetWind * TargetWind + TargetWater * TargetWater);
    const float NormActual = FMath::Sqrt(ActualFire * ActualFire + ActualWind * ActualWind + ActualWater * ActualWater);
    const float Dot = ActualFire * TargetFire + ActualWind * TargetWind + ActualWater * TargetWater;
    const float Projection = NormTarget > 0.0f ? Dot / NormTarget : 0.0f;
    const float Perpendicular = FMath::Sqrt(FMath::Max(0.0f, NormActual * NormActual - Projection * Projection));
    const float Cosine = NormActual > 0.0f && NormTarget > 0.0f ? Dot / (NormActual * NormTarget) : 0.0f;
    const float DoseRatio = NormTarget > 0.0f ? Projection / NormTarget : 0.0f;

    OutResult.Projection = Projection;
    OutResult.PerpendicularCost = Perpendicular;
    OutResult.Cosine = Cosine;
    OutResult.DoseRatio = DoseRatio;
    OutResult.SumAbsolute = SumAbs;
    if (Commission.bMustUseAll && !OutResult.bMustUseAllSatisfied)
    {
        OutResult.VerdictCode = TEXT("contract_breach");
        OutResult.Verdict = TEXT("违约 · 指定用料没下锅");
        OutResult.Reward = 0;
        OutResult.Reason = TEXT("客人指定的必用素材没有全部真正生效。");
        return true;
    }

    OutResult.Reward = 2;
    OutResult.VerdictCode = TEXT("deliverable");
    OutResult.Verdict = TEXT("可交付");
    OutResult.Reason = TEXT("合格品，客人收下但不惊喜。");

    if (SumAbs > 24.0f)
    {
        OutResult.VerdictCode = TEXT("caput_mortuum");
        OutResult.Verdict = TEXT("Caput Mortuum · 死头");
        OutResult.Reward = 0;
        OutResult.Reason = TEXT("坩埚撑爆，整锅焦成死头（Σ|v| > 24）。");
    }
    else if (SumAbs == 0.0f)
    {
        OutResult.VerdictCode = TEXT("empty_bottle");
        OutResult.Verdict = TEXT("空瓶");
        OutResult.Reward = 0;
        OutResult.Reason = TEXT("什么也没配出来。");
    }
    else if (Projection < -0.5f)
    {
        OutResult.VerdictCode = TEXT("reversed_effect");
        OutResult.Verdict = TEXT("反效果 · 药性倒转");
        OutResult.Reward = 0;
        OutResult.Reason = TEXT("方向反了，喝下去适得其反（灵验 < -0.5）。");
    }
    else if (Cosine > 0.995f && FMath::Abs(DoseRatio - 1.0f) <= 0.05f)
    {
        OutResult.VerdictCode = TEXT("magnum_opus");
        OutResult.Verdict = TEXT("Magnum Opus · 大功业");
        OutResult.Reward = 3;
        OutResult.Reason = TEXT("完美：方向准，剂量刚好。");
    }
    else if (DoseRatio < 0.65f)
    {
        OutResult.VerdictCode = TEXT("under_dosed");
        OutResult.Verdict = TEXT("不足 · 药效不够");
        OutResult.Reward = 1;
        OutResult.Reason = TEXT("方向对，但太淡（ρ < 0.65）。");
    }
    else if (Perpendicular > FMath::Abs(Projection) * 0.45f)
    {
        OutResult.VerdictCode = TEXT("side_effect");
        OutResult.Verdict = TEXT("副作用压过药效");
        OutResult.Reward = 1;
        OutResult.Reason = TEXT("有效果，可代价更大。");
    }
    else if (DoseRatio > 1.4f)
    {
        OutResult.VerdictCode = TEXT("over_dosed");
        OutResult.Verdict = TEXT("过量 · 反噬");
        OutResult.Reward = 1;
        OutResult.Reason = TEXT("太浓，反噬（ρ > 1.4）。");
    }

    return true;
}

FString UAlchemyGameplaySubsystem::DescribeAlchemyVector(const FAlchemyVector& Vector) const
{
    if (Vector.Fire == 0 && Vector.Wind == 0 && Vector.Water == 0)
    {
        return TEXT("坩埚冰凉，锅底干干净净，还未投入任何草药矿料。");
    }

    TArray<FString> Parts;
    if (Vector.Fire > 0) Parts.Add(FString::Printf(TEXT("冒着一股%s的焦炭焦烟味"), Vector.Fire >= 6 ? TEXT("烫得燎人") : TEXT("温热")));
    if (Vector.Fire < 0) Parts.Add(TEXT("散发出抽干周围热气的冰凉寒意"));
    if (Vector.Wind > 0) Parts.Add(FString::Printf(TEXT("锅口掠过一阵%s的急促风响"), Vector.Wind >= 6 ? TEXT("呼啸刺耳") : TEXT("干爽清冽")));
    if (Vector.Water > 0) Parts.Add(FString::Printf(TEXT("药汤泛着一层%s的水汽"), Vector.Water >= 6 ? TEXT("浓稠墨黑") : TEXT("湿冷粘腻")));
    if (Vector.Wind < 0) Parts.Add(TEXT("周围的风声被抽走，空气变得迟钝"));
    if (Vector.Water < 0) Parts.Add(TEXT("水气被抽干，只剩冷硬的干燥感"));
    return FString::Printf(TEXT("此时坩埚中：%s。"), *FString::Join(Parts, TEXT("，")));
}

FString UAlchemyGameplaySubsystem::BuildWorldConsequencePrompt(
    const int32 OrderIndex,
    const FAlchemyEvaluationResult& Evaluation,
    const FString& CompactMemoryJson) const
{
    FAlchemyCommissionDefinition Commission;
    if (!GetCommission(OrderIndex, Commission))
    {
        return FString();
    }

    FDialoguePersonaProfile Profile;
    GetPersonaProfile(Commission.CharacterId, Profile);

    FString Prompt = TEXT("你是冷峻奇幻流亡题材的故事叙述者。\n");
    Prompt += FString::Printf(TEXT("刚刚女巫向【%s】（%s）交付了调配药剂。\n\n"), *Commission.Customer, *Profile.Name);
    Prompt += FString::Printf(TEXT("【本单委托】\n- 愿望：%s\n- 情境：%s\n"), *Commission.Wish, *Commission.SensoryPrompt);
    Prompt += FString::Printf(TEXT("- 目标向量：[%d, %d, %d]\n"), Commission.Target.Fire, Commission.Target.Wind, Commission.Target.Water);
    Prompt += FString::Printf(TEXT("- 实际向量：[%d, %d, %d]\n"), Evaluation.Actual.Fire, Evaluation.Actual.Wind, Evaluation.Actual.Water);
    Prompt += FString::Printf(TEXT("- 灵验度：%.2f\n- 代价垂线：%.2f\n- 剂量比：%.2f\n- 最终评级：%s\n- 判定依据：%s\n\n"),
        Evaluation.Projection,
        Evaluation.PerpendicularCost,
        Evaluation.DoseRatio,
        *Evaluation.Verdict,
        *Evaluation.Reason);
    Prompt += TEXT("【叙事要求】\n");
    Prompt += TEXT("- 大功业：愿望精准达成，并在铁匠铺、哨卡或村落激起连锁反应。\n");
    Prompt += TEXT("- 副作用：多余元素必须造成具体恶果，不能写成泛泛的坏结局。\n");
    Prompt += TEXT("- 不足：药效不够，问题仍未解决。\n");
    Prompt += TEXT("- 过量或反效果：必须写出药性失控或与愿望相反的后果。\n");
    Prompt += TEXT("- 输出 200~300 字的次日破晓情节，分为使用现场和外界余波两段，纯 Markdown 正文。\n");
    if (!CompactMemoryJson.IsEmpty())
    {
        Prompt += TEXT("\n【已有世界记忆，只能承接不能覆盖】\n");
        Prompt += CompactMemoryJson;
    }
    return Prompt;
}
