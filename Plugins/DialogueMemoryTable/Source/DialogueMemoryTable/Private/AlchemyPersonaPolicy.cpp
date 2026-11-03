#include "AlchemyPersonaPolicy.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Internationalization/Regex.h"

namespace AlchemyPersonaPolicy
{
FName CanonicalId(FName Id)
{
    if(Id==TEXT("natta"))return TEXT("blacksmith");
    if(Id==TEXT("fawnia"))return TEXT("operator");
    if(Id==TEXT("forest_witch"))return TEXT("novelist");
    return Id;
}
bool ContainsRetiredBinding(FName Id,const FString& Text)
{
    const FName Canon=CanonicalId(Id);
    TArray<FString> Retired;
    if(Canon==TEXT("blacksmith"))Retired={TEXT("老师"),TEXT("中学"),TEXT("teacher"),TEXT("签署空置"),TEXT("空置确认"),TEXT("手腕肿胀"),TEXT("手部变形"),TEXT("绷带")};
    else if(Canon==TEXT("operator"))Retired={TEXT("同事"),TEXT("隔壁工位"),TEXT("colleague"),TEXT("永久沉默"),TEXT("从此不再说话")};
    else if(Canon==TEXT("novelist"))Retired={TEXT("签署删改稿"),TEXT("签了删改稿"),TEXT("亲手提交删改"),TEXT("亲自提交删改")};
    for(const auto& Word:Retired)if(Text.Contains(Word,ESearchCase::IgnoreCase))return true;
    return false;
}
FString ContextText(FName Id,const FString& Text)
{
    return ContainsRetiredBinding(Id,Text)?FString():Text;
}
bool ContainsRetiredWorldBinding(const FString& Text)
{
    return ContainsRetiredBinding(TEXT("blacksmith"),Text) || ContainsRetiredBinding(TEXT("operator"),Text) || ContainsRetiredBinding(TEXT("novelist"),Text);
}
FString CleanReply(FName Id,const FString& Raw)
{
    FString Text=Raw;
    // Strip bracketed performance notes; preserve spoken words, punctuation and quantities.
    const FRegexPattern Pattern(TEXT("\\([^)]*\\)|（[^）]*）|\\[[^\\]]*\\]|【[^】]*】|\\*[^*\\r\\n]+\\*"));
    FRegexMatcher Matcher(Pattern,Text);TArray<TPair<int32,int32>> Ranges;
    while(Matcher.FindNext())Ranges.Emplace(Matcher.GetMatchBeginning(),Matcher.GetMatchEnding());
    for(int32 I=Ranges.Num()-1;I>=0;--I)Text.RemoveAt(Ranges[I].Key,Ranges[I].Value-Ranges[I].Key);
    Text.TrimStartAndEndInline();if(!Text.IsEmpty())return Text.Left(4000);
    const FName Canon=CanonicalId(Id);
    if(Canon==TEXT("blacksmith"))return TEXT("刚才那句没说清。你具体想问哪件事？");
    if(Canon==TEXT("operator"))return TEXT("诶，我这句绕远了。你接着说，我先听。");
    if(Canon==TEXT("novelist"))return TEXT("我刚才光顾着选词了。你接着说，我听着呢。");
    return TEXT("你再说具体些，我听着。");
}
FString FilterMemoryPatch(FName Id,const FString& Json)
{
    TSharedPtr<FJsonObject> Root;
    if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Root) || !Root.IsValid())return Json;
    const TSharedPtr<FJsonObject>* Patch=nullptr;
    if(Root->TryGetObjectField(TEXT("npc_patch"),Patch) && Patch && Patch->IsValid())
    {
        for(const TCHAR* Key:{TEXT("relationship_summary"),TEXT("last_exchange_summary"),TEXT("last_quote")})
        {FString Value;if((*Patch)->TryGetStringField(Key,Value) && ContainsRetiredBinding(Id,Value))(*Patch)->SetStringField(Key,TEXT(""));}
    }
    for(const TCHAR* Key:{TEXT("fact_upserts"),TEXT("open_loop_upserts"),TEXT("world_suggestions")})
    {
        const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;if(!Root->TryGetArrayField(Key,Rows))continue;
        TArray<TSharedPtr<FJsonValue>> Kept;
        for(const auto& Row:*Rows)
        {
            FString Encoded;const TSharedPtr<FJsonObject>* Object=nullptr;
            if(Row.IsValid() && Row->TryGetObject(Object) && Object && Object->IsValid())FJsonSerializer::Serialize(Object->ToSharedRef(),TJsonWriterFactory<>::Create(&Encoded));
            const bool Retired=FString(Key)==TEXT("world_suggestions")?ContainsRetiredWorldBinding(Encoded):ContainsRetiredBinding(Id,Encoded);
            if(!Retired)Kept.Add(Row);
        }
        Root->SetArrayField(Key,Kept);
    }
    FString Result;FJsonSerializer::Serialize(Root.ToSharedRef(),TJsonWriterFactory<>::Create(&Result));return Result;
}
}
