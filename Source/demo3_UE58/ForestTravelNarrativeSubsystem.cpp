#include "ForestTravelNarrativeSubsystem.h"

#include "DialogueAIServiceSubsystem.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

namespace ForestTravelNarrative
{
 FDialogueMessage Message(const FString& Role,const FString& Content)
 {
  FDialogueMessage Result;Result.Role=Role;Result.Content=Content;return Result;
 }
}

void UForestTravelNarrativeRequestContext::Completed(const bool bSuccess,const FString& Text)
{if(Owner.IsValid())Owner->HandleGatewayResponse(this,bSuccess,Text);}

void UForestTravelNarrativeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
 Super::Initialize(Collection);Collection.InitializeDependency<UDialogueAIServiceSubsystem>();
}

FString UForestTravelNarrativeSubsystem::ElementName(const EAlchemyElement Element)
{
 switch(Element){case EAlchemyElement::Wind:return TEXT("风");case EAlchemyElement::Water:return TEXT("水");default:return TEXT("火");}
}

FString UForestTravelNarrativeSubsystem::CleanJson(FString Text)
{
 Text.TrimStartAndEndInline();
 if(Text.StartsWith(TEXT("```"))){const int32 FirstLine=Text.Find(TEXT("\n"));if(FirstLine!=INDEX_NONE)Text=Text.Mid(FirstLine+1);Text.RemoveFromEnd(TEXT("```"));Text.TrimStartAndEndInline();}
 return Text;
}

FString UForestTravelNarrativeSubsystem::BuildPrompt(const FForestTravelNarrativeInput& Input)
{
 return FString::Printf(
  TEXT("为一次魔法寻路生成一个临时涌现物的名称和短叙事。\n")
  TEXT("已消耗物品：%s（id=%s）\n目标角色：%s（id=%s）\n")
  TEXT("元素：%s；点数：%d；三轴数值：火%d/风%d/水%d；数值来源：%s；是否占位：%s。\n")
  TEXT("要求：名称2-10个汉字；叙事35-80个汉字，描述物品被交付后如何化成寻路媒介并把玩家引向目标。")
  TEXT("只输出JSON对象，不要Markdown，不要解释：{\"item_name\":\"...\",\"narrative\":\"...\"}"),
  *Input.ConsumedItemName,*Input.ConsumedItemId,*Input.TargetNpcName,*Input.TargetNpcId.ToString(),
  *(Input.bHasElementValue?ElementName(Input.Element):TEXT("未接入")),Input.PipValue,Input.Value.Fire,Input.Value.Wind,Input.Value.Water,
  *Input.ValueSource,Input.bUsesPlaceholderValue?TEXT("true"):TEXT("false"));
}

bool UForestTravelNarrativeSubsystem::ParseResult(const FString& Text,FForestTravelNarrativeResult& OutResult)
{
 TSharedPtr<FJsonObject> Root;const auto Reader=TJsonReaderFactory<>::Create(CleanJson(Text));
 if(!FJsonSerializer::Deserialize(Reader,Root) || !Root.IsValid())return false;
 FString Name,Narrative;if(!Root->TryGetStringField(TEXT("item_name"),Name) || !Root->TryGetStringField(TEXT("narrative"),Narrative))return false;
 Name.TrimStartAndEndInline();Narrative.TrimStartAndEndInline();if(Name.IsEmpty() || Narrative.IsEmpty())return false;
 OutResult.EmergentItemName=Name.Left(24);OutResult.ShortNarrative=Narrative.Left(180);return true;
}

FForestTravelNarrativeResult UForestTravelNarrativeSubsystem::BuildFallback(const FForestTravelNarrativeInput& Input,const FString& Error) const
{
 FForestTravelNarrativeResult Result;Result.RequestId=Input.RequestId;Result.bUsedPlaceholderValue=Input.bUsesPlaceholderValue;Result.Error=Error;
 if(!Input.bHasElementValue){Result.EmergentItemName=TEXT("未定形路引");Result.ShortNarrative=FString::Printf(TEXT("交出的%s暂时失去原形，凝成一道指向%s的微光；它的元素与强度仍等待数值表补全。"),*Input.ConsumedItemName,*Input.TargetNpcName);return Result;}
 const FString Strength=Input.PipValue>=7?TEXT("盛"):Input.PipValue>=4?TEXT("鸣"):TEXT("微");
 switch(Input.Element)
 {
 case EAlchemyElement::Wind:Result.EmergentItemName=Strength+TEXT("风路羽");Result.ShortNarrative=FString::Printf(TEXT("交出的%s散成一簇带声的风羽，贴着林间可走的路向%s所在之处飘去。"),*Input.ConsumedItemName,*Input.TargetNpcName);break;
 case EAlchemyElement::Water:Result.EmergentItemName=Strength+TEXT("潮引露");Result.ShortNarrative=FString::Printf(TEXT("交出的%s化作一线冷亮水痕，沿地面绕开阻碍，向%s所在之处延伸。"),*Input.ConsumedItemName,*Input.TargetNpcName);break;
 default:Result.EmergentItemName=Strength+TEXT("烬路签");Result.ShortNarrative=FString::Printf(TEXT("交出的%s燃成不灼人的细小余烬，逐点落在通往%s的道路上。"),*Input.ConsumedItemName,*Input.TargetNpcName);break;
 }
 return Result;
}

void UForestTravelNarrativeSubsystem::GenerateTravelNarrative(const FForestTravelNarrativeInput& Input,FForestTravelNarrativeCompletion Completion)
{
 LatestRequestedId=Input.RequestId;
 if(Input.RequestId<=0 || Input.ConsumedItemName.IsEmpty() || Input.TargetNpcId.IsNone() || Input.TargetNpcName.IsEmpty())
 {auto Result=BuildFallback(Input,TEXT("寻路叙事输入不完整。"));LastResult=Result;Completion.ExecuteIfBound(Result);return;}
 auto* AI=GetGameInstance()?GetGameInstance()->GetSubsystem<UDialogueAIServiceSubsystem>():nullptr;
 if(!AI || !AI->HasApiKey())
 {auto Result=BuildFallback(Input,TEXT("未配置共享 AI Key，已使用本地叙事。"));LastResult=Result;Completion.ExecuteIfBound(Result);return;}
 auto* Context=NewObject<UForestTravelNarrativeRequestContext>(this);Context->Owner=this;Context->Input=Input;Context->Completion=Completion;Requests.Add(Context);
 FDialogueAICompletion GatewayCompletion;GatewayCompletion.BindDynamic(Context,&UForestTravelNarrativeRequestContext::Completed);
 const TArray<FDialogueMessage> Messages={
  ForestTravelNarrative::Message(TEXT("system"),TEXT("你是克制的现代工业幻想叙事器。输入事实不可改写。严格按指定JSON结构返回。")),
  ForestTravelNarrative::Message(TEXT("user"),BuildPrompt(Input))};
 AI->RequestChatCompletion(Messages,0.75f,180,GatewayCompletion);
}

void UForestTravelNarrativeSubsystem::HandleGatewayResponse(UForestTravelNarrativeRequestContext* Context,const bool bSuccess,const FString& Text)
{
 if(!Context || !Requests.Contains(Context))return;Requests.Remove(Context);
 FForestTravelNarrativeResult Result=BuildFallback(Context->Input,bSuccess?TEXT(""):Text.Left(250));
 if(bSuccess && ParseResult(Text,Result)){Result.bGeneratedByAI=true;Result.Error.Reset();}
 else if(bSuccess)Result.Error=TEXT("AI 返回结构无效，已使用本地叙事。");
 if(Result.RequestId==LatestRequestedId)LastResult=Result;Context->Completion.ExecuteIfBound(Result);
}

void UForestTravelNarrativeSubsystem::Deinitialize()
{
 for(auto& Request:Requests)if(Request)Request->Owner.Reset();Requests.Reset();Super::Deinitialize();
}
