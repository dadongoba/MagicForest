#include "DialogueAIServiceSubsystem.h"
#include "AlchemyDialogueReplyGate.h"
#include "AlchemyDialogueKnowledge.h"

#include "AlchemyGameplaySubsystem.h"
#include "Dom/JsonObject.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonSerializer.h"

void UDialogueAIServiceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    GatewayBaseUrl = TEXT("https://api.deepseek.com/v1");
    GatewayModel = TEXT("deepseek-chat");
    ApiKey.Reset();
}

void UDialogueAIServiceSubsystem::SetGatewayConfig(
    const FString& InBaseUrl,
    const FString& InApiKey,
    const FString& InModel)
{
    GatewayBaseUrl = InBaseUrl.TrimStartAndEnd();
    GatewayBaseUrl.RemoveFromEnd(TEXT("/"));
    ApiKey = InApiKey.TrimStartAndEnd();
    GatewayModel = InModel.TrimStartAndEnd().IsEmpty() ? TEXT("deepseek-chat") : InModel.TrimStartAndEnd();
}

void UDialogueAIServiceSubsystem::RequestChatCompletion(
    const TArray<FDialogueMessage>& Messages,
    const float Temperature,
    const int32 MaxTokens,
    FDialogueAICompletion Callback)
{
    if (ApiKey.IsEmpty())
    {
        Callback.ExecuteIfBound(false, TEXT("请先配置 DeepSeek API Key。"));
        return;
    }

    if (Messages.Num() == 0)
    {
        Callback.ExecuteIfBound(false, TEXT("消息列表不能为空。"));
        return;
    }

    TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
    Payload->SetStringField(TEXT("model"), GatewayModel);
    Payload->SetNumberField(TEXT("temperature"), FMath::Clamp(Temperature, 0.0f, 2.0f));
    Payload->SetNumberField(TEXT("max_tokens"), FMath::Clamp(MaxTokens, 1, 8192));

    TArray<TSharedPtr<FJsonValue>> JsonMessages;
    for (const FDialogueMessage& Message : Messages)
    {
        TSharedPtr<FJsonObject> JsonMessage = MakeShared<FJsonObject>();
        JsonMessage->SetStringField(TEXT("role"), Message.Role.IsEmpty() ? TEXT("user") : Message.Role);
        JsonMessage->SetStringField(TEXT("content"), Message.Content);
        JsonMessages.Add(MakeShared<FJsonValueObject>(JsonMessage));
    }
    Payload->SetArrayField(TEXT("messages"), JsonMessages);

    FString Body;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
    if (!FJsonSerializer::Serialize(Payload.ToSharedRef(), Writer))
    {
        Callback.ExecuteIfBound(false, TEXT("无法序列化 DeepSeek 请求。"));
        return;
    }

    const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(GatewayBaseUrl + TEXT("/chat/completions"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
    Request->SetContentAsString(Body);
    Request->SetTimeout(35.f);
    const TWeakObjectPtr<UDialogueAIServiceSubsystem> WeakThis(this);
    Request->OnProcessRequestComplete().BindLambda(
        [Callback, WeakThis](
            FHttpRequestPtr,
            FHttpResponsePtr Response,
            const bool bWasSuccessful)
        {
            if(WeakThis.IsValid())WeakThis->CompleteRequest(Response, bWasSuccessful, Callback);
        });
    if(!Request->ProcessRequest())Callback.ExecuteIfBound(false,TEXT("无法启动 AI 请求。"));
}

void UDialogueAIServiceSubsystem::RequestNpcDialogue(
    const FName PersonaId,
    const int32 Familiarity,
    const int32 CommissionOrderIndex,
    const FString& CompactMemoryJson,
    const TArray<FDialogueMessage>& DialogueHistory,
    FDialogueAICompletion Callback)
{
    UAlchemyGameplaySubsystem* Gameplay = GetGameInstance() ? GetGameInstance()->GetSubsystem<UAlchemyGameplaySubsystem>() : nullptr;
    if (!Gameplay)
    {
        Callback.ExecuteIfBound(false, TEXT("AlchemyGameplaySubsystem 不可用。"));
        return;
    }

    TArray<FDialogueMessage> Messages;
    FDialogueMessage SystemMessage;
    SystemMessage.Role = TEXT("system");
    SystemMessage.Content = Gameplay->BuildPersonaSystemPrompt(
        PersonaId,
        Familiarity,
        CommissionOrderIndex,
        CompactMemoryJson,
        {});
    Messages.Add(SystemMessage);

    for (int32 Index=FMath::Max(0,DialogueHistory.Num()-8);Index<DialogueHistory.Num();++Index)
    {
        if(DialogueHistory[Index].Role!=TEXT("assistant") || AlchemyDialogueKnowledge::IsSafeReply(DialogueHistory[Index].Content))Messages.Add(DialogueHistory[Index]);
    }

    RequestReviewedNpcMessages(PersonaId,Messages,Callback);
}

void UDialogueAIServiceSubsystem::RequestReviewedNpcMessages(FName PersonaId,const TArray<FDialogueMessage>& Messages,FDialogueAICompletion Callback)
{
    if(Messages.IsEmpty() || Messages[0].Role!=TEXT("system") || Messages[0].Content.IsEmpty()){Callback.ExecuteIfBound(false,TEXT("npc_prompt_missing"));return;}
    auto* Gate=NewObject<UAlchemyDialogueReplyGate>(this);Gate->Service=this;Gate->Result=Callback;Gate->Messages=Messages;Gate->PersonaId=PersonaId;
    for(int32 I=Messages.Num()-1;I>=0;--I)if(Messages[I].Role==TEXT("user")){Gate->PlayerText=Messages[I].Content;break;}
    NpcReplyGates.Add(Gate);Gate->Send();
}

void UDialogueAIServiceSubsystem::ReleaseNpcReplyGate(UAlchemyDialogueReplyGate* Gate){NpcReplyGates.Remove(Gate);}

void UDialogueAIServiceSubsystem::RequestWorldConsequence(
    const int32 CommissionOrderIndex,
    const FAlchemyEvaluationResult& Evaluation,
    const FString& CompactMemoryJson,
    FDialogueAICompletion Callback)
{
    UAlchemyGameplaySubsystem* Gameplay = GetGameInstance() ? GetGameInstance()->GetSubsystem<UAlchemyGameplaySubsystem>() : nullptr;
    if (!Gameplay)
    {
        Callback.ExecuteIfBound(false, TEXT("AlchemyGameplaySubsystem 不可用。"));
        return;
    }

    FDialogueMessage Message;
    Message.Role = TEXT("user");
    Message.Content = Gameplay->BuildWorldConsequencePrompt(CommissionOrderIndex, Evaluation, CompactMemoryJson);
    TArray<FDialogueMessage> Messages;
    Messages.Add(Message);
    RequestChatCompletion(Messages, 0.7f, 500, Callback);
}

void UDialogueAIServiceSubsystem::CompleteRequest(
    const TSharedPtr<IHttpResponse, ESPMode::ThreadSafe>& Response,
    const bool bWasSuccessful,
    FDialogueAICompletion Callback) const
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        Callback.ExecuteIfBound(false, TEXT("DeepSeek 请求失败或没有响应。"));
        return;
    }

    if (Response->GetResponseCode() < 200 || Response->GetResponseCode() >= 300)
    {
        Callback.ExecuteIfBound(
            false,
            FString::Printf(TEXT("DeepSeek API 报错 [%d]: %s"), Response->GetResponseCode(), *Response->GetContentAsString()));
        return;
    }

    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        Callback.ExecuteIfBound(false, TEXT("DeepSeek 返回内容不是合法 JSON。"));
        return;
    }

    const TArray<TSharedPtr<FJsonValue>>* Choices = nullptr;
    if (!Root->TryGetArrayField(TEXT("choices"), Choices) || !Choices || Choices->Num() == 0 ||
        !(*Choices)[0].IsValid() || (*Choices)[0]->Type != EJson::Object)
    {
        Callback.ExecuteIfBound(false, TEXT("DeepSeek 返回中没有 choices。"));
        return;
    }

    const TSharedPtr<FJsonObject> ChoiceObject = (*Choices)[0]->AsObject();
    const TSharedPtr<FJsonObject>* MessageObject = nullptr;
    if (!ChoiceObject.IsValid() || !ChoiceObject->TryGetObjectField(TEXT("message"), MessageObject) ||
        !MessageObject || !MessageObject->IsValid())
    {
        Callback.ExecuteIfBound(false, TEXT("DeepSeek 返回中没有 message。"));
        return;
    }

    FString Content;
    if (!(*MessageObject)->TryGetStringField(TEXT("content"), Content))
    {
        Callback.ExecuteIfBound(false, TEXT("DeepSeek 返回中没有 content。"));
        return;
    }

    Callback.ExecuteIfBound(true, Content.TrimStartAndEnd());
}
