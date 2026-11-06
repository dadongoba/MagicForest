#include "ForestNPCDialogueSubsystem.h"
#include "ForestAlchemy.h"
#include "ForestCommissionAcceptance.h"
#include "AlchemyPersonaPolicy.h"
#include "AlchemyDialogueKnowledge.h"
#include "AlchemyDialogueTurnPolicy.h"
#include "DialogueAIServiceSubsystem.h"
#include "DialogueMemoryTableSubsystem.h"
#include "AlchemyGameplaySubsystem.h"
#include "DailyMushroomSubsystem.h"
#include "AlchemyCampaignPolicy.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
namespace ForestDialogue
{
 FString Json(const TSharedRef<FJsonObject>& Object){FString Out;FJsonSerializer::Serialize(Object,TJsonWriterFactory<>::Create(&Out));return Out;}
 FDialogueMessage Message(const FString& Role,const FString& Text){FDialogueMessage M;M.Role=Role;M.Content=Text;return M;}
 FString CleanJson(FString Text)
 {
  Text.TrimStartAndEndInline();if(Text.StartsWith(TEXT("```"))){int32 End=Text.Find(TEXT("\n"));Text=Text.Mid(End+1);Text.RemoveFromEnd(TEXT("```"));Text.TrimStartAndEndInline();}return Text;
 }
}
void UForestDialogueRequestContext::Completed(bool Success,const FString& Text)
{if(Owner.IsValid())Owner->HandleResponse(this,Success,Text);}
void UForestDialogueRequestContext::ReplyCompleted(const FAlchemyDialogueTurnResult& Reply)
{VerifiedReply=Reply;bVerifiedReply=true;if(Owner.IsValid())Owner->HandleResponse(this,Reply.bSuccess,Reply.bSuccess?Reply.Text:Reply.Error);}
void UForestNPCDialogueSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
 Super::Initialize(Collection);
 Collection.InitializeDependency<UDialogueMemoryTableSubsystem>();Collection.InitializeDependency<UAlchemyGameplaySubsystem>();Collection.InitializeDependency<UDialogueAIServiceSubsystem>();Collection.InitializeDependency<UDailyMushroomSubsystem>();
 RegisterForestPersonas();ConfigurePersistence(Slot,TEXT("DialogueMemoryLedger"));
 auto* AI=GetGameInstance()->GetSubsystem<UDialogueAIServiceSubsystem>();
 FString Url=FPlatformMisc::GetEnvironmentVariable(TEXT("FOREST_DIALOGUE_BASE_URL"));if(Url.IsEmpty())Url=TEXT("https://api.deepseek.com/v1");
 FString Model=FPlatformMisc::GetEnvironmentVariable(TEXT("FOREST_DIALOGUE_MODEL"));if(Model.IsEmpty())Model=TEXT("deepseek-chat");
 AI->SetGatewayConfig(Url,FPlatformMisc::GetEnvironmentVariable(TEXT("FOREST_DIALOGUE_API_KEY")),Model);
 Status=AI->HasApiKey()?TEXT("可以交谈"):TEXT("未配置 AI Key；可启用离线联调");
}
void UForestNPCDialogueSubsystem::RegisterForestPersonas()
{
 auto* Gameplay=GetGameInstance()->GetSubsystem<UAlchemyGameplaySubsystem>();
 const TArray<FName> Ids={TEXT("forest_witch"),TEXT("natta"),TEXT("fawnia")};
 const TArray<FName> SourceIds={TEXT("novelist"),TEXT("blacksmith"),TEXT("operator")};
 const TArray<FString> Names={TEXT("森林女巫"),TEXT("Natta"),TEXT("Fawnia")};
 for(int32 I=0;I<Ids.Num();++I)
 {
  FDialoguePersonaProfile Profile;
  if(!Gameplay->GetPersonaProfile(SourceIds[I],Profile))continue;
  // Keep the scene identity and separate memory while inheriting the complete authored persona.
  Profile.PersonaId=Ids[I];Profile.Name=Names[I];
  Gameplay->RegisterPersonaProfile(Profile);
 }
}
bool UForestNPCDialogueSubsystem::ConfigurePersistence(const FString& ConversationSlot,const FString& MemorySlot)
{
 if(!ActiveNpc.IsNone() || !Requests.IsEmpty() || ConversationSlot.IsEmpty() || MemorySlot.IsEmpty())return false;
 Slot=ConversationSlot;Save=nullptr;
 if(UGameplayStatics::DoesSaveGameExist(Slot,0))Save=Cast<UForestConversationSave>(UGameplayStatics::LoadGameFromSlot(Slot,0));
 else Save=Cast<UForestConversationSave>(UGameplayStatics::CreateSaveGameObject(UForestConversationSave::StaticClass()));
 auto* Memory=GetGameInstance()->GetSubsystem<UDialogueMemoryTableSubsystem>();Memory->ConfigureSaveSlot(MemorySlot,0);FString Error;
 bPersistenceReady=Save && Memory->LoadLedger(Error);if(!bPersistenceReady)Status=TEXT("存档读取失败，请检查存档槽：")+Error;
 return bPersistenceReady;
}
FForestConversationRow& UForestNPCDialogueSubsystem::Row(FName Id)
{
 for(auto& R:Save->Rows)if(R.NpcId==Id)return R;
 FForestConversationRow New;New.NpcId=Id;Save->Rows.Add(New);return Save->Rows.Last();
}
bool UForestNPCDialogueSubsystem::Persist()
{const bool Ok=Save && UGameplayStatics::SaveGameToSlot(Save,Slot,0);if(!Ok)Status=TEXT("会话存档失败；本次记录仍在内存中");return Ok;}
bool UForestNPCDialogueSubsystem::BeginConversation(FName Id)
{
 if(!bPersistenceReady || !ActiveNpc.IsNone())return false;
 if(const auto* D=GetGameInstance()->GetSubsystem<UDailyMushroomSubsystem>();D && D->IsCampaignCompleted())return false;
 FDialoguePersonaProfile Profile;if(!GetGameInstance()->GetSubsystem<UAlchemyGameplaySubsystem>()->GetPersonaProfile(Id,Profile))return false;
 auto* Memory=GetGameInstance()->GetSubsystem<UDialogueMemoryTableSubsystem>();auto* Day=GetGameInstance()->GetSubsystem<UDailyMushroomSubsystem>();const int32 WorldDay=Day?Day->GetCurrentDay():1;int32 Level=1;int64 Flags=0,Last=0;
 for(const auto& S:Memory->GetLedgerSnapshot().NpcStates)if(S.NpcId==Id){Level=S.Familiarity;Flags=S.KnownSecretFlags;Last=S.LastSessionId;}
 FString Error;if(!Memory->SetAuthoritativeNpcProgress(Id,Level,Flags,WorldDay,true,Error)){Status=Error;return false;}
 auto& R=Row(Id);
 if(R.PersonaRevision<AlchemyPersonaPolicy::Revision)
 {
  const auto Before=R;R.ArchivedPersonaHistory=R.History;R.History.Reset();R.PersonaRevision=AlchemyPersonaPolicy::Revision;
  if(AlchemyPersonaPolicy::ContainsRetiredBinding(Id,R.OfferText)){R.OfferText.Reset();R.OfferDay=0;R.OfferCommissionId=0;}
  if(!Persist()){R=Before;return false;}
 }
 if(R.KnowledgeRevision<AlchemyDialogueKnowledge::Revision)
 {
  const auto Before=R;
  if(R.KnowledgeRevision<2){R.ArchivedKnowledgeHistory=R.History;R.History.Reset();R.OfferText.Reset();R.OfferDay=0;R.OfferCommissionId=0;}
  R.DisclosedTopics=0;R.DeliveredTaskFacts.Reset();R.DeliveredPersonaFacts.Reset();
  R.KnowledgeRevision=AlchemyDialogueKnowledge::Revision;
  if(!Persist()){R=Before;return false;}
 }
 if(R.HistoryDay!=WorldDay){R.History.Reset();R.HistoryDay=WorldDay;}R.NextSession=FMath::Max(R.NextSession,Last+1);
 ActiveNpc=Id;SessionTranscript.Reset();bWaiting=false;++Generation;
 Status=bOfflineTest?TEXT("离线联调：模拟回复，不调用 AI"):TEXT("输入内容，按 Enter 发送");
 OnConversationUpdated.Broadcast();RetryPendingSummaries();return true;
}
bool UForestNPCDialogueSubsystem::IsFirstDayMessengerTutorialPending() const
{
 if(!bPersistenceReady || !Save || Save->bFirstDayMessengerTutorialDelivered || !ActiveNpc.IsNone())return false;
 const auto* Day=GetGameInstance()?GetGameInstance()->GetSubsystem<UDailyMushroomSubsystem>():nullptr;
 return Day && Day->GetCurrentDay()==1;
}
bool UForestNPCDialogueSubsystem::BeginFirstDayMessengerTutorial()
{
 if(!IsFirstDayMessengerTutorialPending() || !BeginConversation(TEXT("fawnia")))return false;
 const FString Opening=TEXT("嗐，终于找到你了。新来的女巫，对吧？第一天先听我说清楚：林子里的蘑菇每天不一样，摘之前留意它带的是火、风还是水。接了委托，就拿当天能找到的材料去配；接到的委托会记在魔法书的委托手札里。收齐材料后回小屋，在合成台选好委托、调配并封瓶，不用把原料拿给我们。我们三人的今日委托都结算完，才会迎来下一天。路和人我熟，下一步找不着就来问我。");
 auto& R=Row(TEXT("fawnia"));const auto Message=ForestDialogue::Message(TEXT("assistant"),Opening);
 R.History.Add(Message);SessionTranscript.Add(Message);while(R.History.Num()>16)R.History.RemoveAt(0);
 Save->bFirstDayMessengerTutorialDelivered=true;Status=TEXT("Fawnia 找到了你。可以继续问她，或按 Esc 结束交谈。");
 Persist();OnConversationUpdated.Broadcast();return true;
}
FString UForestNPCDialogueSubsystem::GetTranscript() const
{
 if(!Save)return TEXT("");FString Out;
 for(const auto& R:Save->Rows)if(R.NpcId==ActiveNpc)
  for(const auto& M:R.History)Out+=FString::Printf(TEXT("%s：%s\n\n"),M.Role==TEXT("user")?TEXT("你"):TEXT("对方"),*M.Content);
 return Out;
}
FString UForestNPCDialogueSubsystem::GetCompactMemoryJson(FName Id) const
{
 auto* Memory=GetGameInstance()->GetSubsystem<UDialogueMemoryTableSubsystem>();const auto Snapshot=Memory->GetLedgerSnapshot();auto Root=MakeShared<FJsonObject>();
 for(const auto& S:Snapshot.NpcStates)if(S.NpcId==Id)
 {Root->SetStringField(TEXT("relationship"),AlchemyDialogueKnowledge::SafeContext(AlchemyPersonaPolicy::ContextText(Id,S.RelationshipSummary)));Root->SetStringField(TEXT("last_exchange"),AlchemyDialogueKnowledge::SafeContext(AlchemyPersonaPolicy::ContextText(Id,S.LastExchangeSummary)));}
 TArray<TSharedPtr<FJsonValue>> Facts,Loops,World;
 for(const auto& F:Snapshot.CharacterFacts)if(F.NpcId==Id && AlchemyDialogueKnowledge::IsSafeReply(F.ObjectText) && F.Status!=EDialogueMemoryFactStatus::Contradicted && F.Visibility!=EDialogueMemoryVisibility::NpcPrivate && !AlchemyPersonaPolicy::ContainsRetiredBinding(Id,F.FactKey.ToString()+TEXT(" ")+F.ObjectText))
 {auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("key"),F.FactKey.ToString());O->SetStringField(TEXT("subject"),F.Subject.ToString());O->SetStringField(TEXT("predicate"),F.Predicate.ToString());O->SetStringField(TEXT("text"),F.ObjectText);O->SetStringField(TEXT("authority"),F.Authority==EDialogueMemoryAuthority::GameEvent?TEXT("game_event"):F.Authority==EDialogueMemoryAuthority::NpcDisclosure?TEXT("npc_disclosure"):TEXT("uncertain_report"));O->SetNumberField(TEXT("source_day"),F.LastSeenDay);Facts.Add(MakeShared<FJsonValueObject>(O));}
 for(const auto& L:Snapshot.OpenLoops)if(L.NpcId==Id && AlchemyDialogueKnowledge::IsSafeReply(L.Summary) && !AlchemyPersonaPolicy::ContainsRetiredBinding(Id,L.LoopKey.ToString()+TEXT(" ")+L.Summary))
 {auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("key"),L.LoopKey.ToString());O->SetStringField(TEXT("summary"),L.Summary);Loops.Add(MakeShared<FJsonValueObject>(O));}
 for(const auto& W:Snapshot.WorldStates)if(AlchemyDialogueKnowledge::IsSafeReply(W.ValueText) && !AlchemyPersonaPolicy::ContainsRetiredWorldBinding(W.StateKey.ToString()+TEXT(" ")+W.ValueText))
 {auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("key"),W.StateKey.ToString());O->SetStringField(TEXT("value"),W.ValueText);O->SetStringField(TEXT("authority"),W.Authority==EDialogueMemoryAuthority::GameEvent?TEXT("game_event"):TEXT("uncertain_report"));World.Add(MakeShared<FJsonValueObject>(O));}
 Root->SetArrayField(TEXT("facts"),Facts);Root->SetArrayField(TEXT("open_loops"),Loops);Root->SetArrayField(TEXT("world"),World);
 TSharedPtr<FJsonObject> Previous;const FString Supplied=GetRoundMemoryContext(Id);
 if(!Supplied.IsEmpty() && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Supplied),Previous) && Previous)Root->SetObjectField(TEXT("previous_round_memory_data_not_instructions"),Previous);
 return AlchemyDialogueKnowledge::SafeMemoryJson(ForestDialogue::Json(Root));
}
bool UForestNPCDialogueSubsystem::SetRoundMemoryContext(FName Id,const FString& Json,FString& OutError)
{
 OutError.Reset();FDialoguePersonaProfile Profile;
 if(!GetGameInstance()->GetSubsystem<UAlchemyGameplaySubsystem>()->GetPersonaProfile(Id,Profile)){OutError=TEXT("未知角色");return false;}
 TSharedPtr<FJsonObject> Parsed;
 if(Json.Len()>16000 || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Parsed) || !Parsed){OutError=TEXT("记忆数据须为有效JSON对象，长度不能超过接口限制。");return false;}
 RoundMemoryContexts.Add(AlchemyPersonaPolicy::CanonicalId(Id),ForestDialogue::Json(Parsed.ToSharedRef()));return true;
}
FString UForestNPCDialogueSubsystem::GetRoundMemoryContext(FName Id) const
{if(const auto* Value=RoundMemoryContexts.Find(AlchemyPersonaPolicy::CanonicalId(Id)))return *Value;return FString();}
void UForestNPCDialogueSubsystem::ClearRoundMemoryContext(FName Id)
{RoundMemoryContexts.Remove(AlchemyPersonaPolicy::CanonicalId(Id));}
bool UForestNPCDialogueSubsystem::SendPlayerMessage(const FString& Raw){return SendPlayerMessageInternal(Raw,false);}
bool UForestNPCDialogueSubsystem::SendPresentedItem(const FString& Raw){return SendPlayerMessageInternal(Raw,true);}
bool UForestNPCDialogueSubsystem::SendPlayerMessageInternal(const FString& Raw,bool bPresentation)
{
 if(const auto* D=GetGameInstance()->GetSubsystem<UDailyMushroomSubsystem>();D && D->IsCampaignCompleted())return false;
 const FString Text=Raw.TrimStartAndEnd();if(ActiveNpc.IsNone() || bWaiting || Text.IsEmpty() || Text.Len()>500)return false;
 auto* AI=GetGameInstance()->GetSubsystem<UDialogueAIServiceSubsystem>();
 if(!bOfflineTest && !AI->HasApiKey()){Status=TEXT("未配置 FOREST_DIALOGUE_API_KEY。设置后重新启动 UE；或在场景管理器启用离线联调。");OnConversationUpdated.Broadcast();return false;}
 auto* Gameplay=GetGameInstance()->GetSubsystem<UAlchemyGameplaySubsystem>();
 const int32 WorldDay=GetGameInstance()->GetSubsystem<UDailyMushroomSubsystem>()->GetCurrentDay();
 if(!Gameplay)return false;
 // Online progression is proposed by semantic analysis, then validated below.
 if(bOfflineTest)
 {int32 L=1,T=0;FString E;if(!Gameplay->AdvancePersonaFromPlayerText(ActiveNpc,Text,0,WorldDay,true,L,T,E))return false;}
 auto& R=Row(ActiveNpc);const auto M=ForestDialogue::Message(TEXT("user"),Text);R.History.Add(M);SessionTranscript.Add(M);bWaiting=true;Status=TEXT("对方正在回答…");Persist();OnConversationUpdated.Broadcast();
 if(bOfflineTest)
 {auto* Context=NewObject<UForestDialogueRequestContext>(this);Context->Owner=this;Context->NpcId=ActiveNpc;Context->Generation=Generation;Context->Day=WorldDay;Requests.Add(Context);HandleResponse(Context,true,OfflineReply(Text));return true;}
 int32 Level=1;for(const auto& S:GetGameInstance()->GetSubsystem<UDialogueMemoryTableSubsystem>()->GetLedgerSnapshot().NpcStates)if(S.NpcId==ActiveNpc)Level=S.Familiarity;
 const FName CommissionPersona=ActiveNpc==TEXT("natta")?TEXT("blacksmith"):ActiveNpc==TEXT("fawnia")?TEXT("operator"):ActiveNpc==TEXT("forest_witch")?TEXT("novelist"):ActiveNpc;
 const int32 PersonaOffset=CommissionPersona==TEXT("operator")?3:CommissionPersona==TEXT("novelist")?6:CommissionPersona==TEXT("blacksmith")?0:-1;
 const int32 CommissionOrderIndex=AlchemyCampaign::CommissionId(PersonaOffset,WorldDay);
 auto* Context=NewObject<UForestDialogueRequestContext>(this);Context->Owner=this;Context->NpcId=ActiveNpc;Context->Generation=Generation;Context->Day=WorldDay;Context->CommissionId=CommissionOrderIndex;Context->Familiarity=Level;Context->bPresentation=bPresentation;Context->KnowledgeFingerprint=AlchemyDialogueKnowledge::Fingerprint();
 Context->Scope=MakeShared<FDialogueRequestScope>();Context->Scope->Deadline=FPlatformTime::Seconds()+45.;ActiveTurnScope=Context->Scope;
 if(R.KnowledgeFingerprint!=Context->KnowledgeFingerprint){R.DeliveredTaskFacts.Reset();R.DeliveredPersonaFacts.Reset();R.KnowledgeFingerprint=Context->KnowledgeFingerprint;}
 bool OfferInRecent=false;for(int32 I=FMath::Max(0,R.History.Num()-8);I<R.History.Num();++I)if(R.History[I].Role==TEXT("assistant") && R.History[I].Content==R.OfferText)OfferInRecent=true;
 if(!OfferInRecent && R.OfferDay==WorldDay && R.OfferCommissionId==CommissionOrderIndex && !R.OfferText.IsEmpty())Context->Evidence.Add(ForestDialogue::Message(TEXT("assistant"),R.OfferText));
 for(int32 I=FMath::Max(0,R.History.Num()-8);I<R.History.Num();++I)Context->Evidence.Add(R.History[I]);
 auto* Alchemy=GetWorld()->GetSubsystem<UForestAlchemy>();
 if(R.KnowledgeDay!=WorldDay || R.KnowledgeCommission!=CommissionOrderIndex){R.KnowledgeDay=WorldDay;R.KnowledgeCommission=CommissionOrderIndex;R.DisclosedTopics=0;R.DeliveredTaskFacts.Reset();}
 Context->bAllowAcceptance=!bPresentation && Alchemy && !Alchemy->IsCommissionAccepted(CommissionOrderIndex);
 FAlchemyCommissionDefinition KnowledgeCommission;const bool HasKnowledge=Gameplay->GetCommission(CommissionOrderIndex,KnowledgeCommission) && AlchemyDialogueKnowledge::HasReviewedEntry(KnowledgeCommission);
 if(Context->bAllowAcceptance || HasKnowledge)RequestAcceptanceCheck(Context);else RequestNpcReply(Context);
 return true;
}
void UForestNPCDialogueSubsystem::RequestAcceptanceCheck(UForestDialogueRequestContext* Context)
{
 Context->bAcceptanceCheck=true;Requests.AddUnique(Context);
 FAlchemyCommissionDefinition C;auto* G=GetGameInstance()->GetSubsystem<UAlchemyGameplaySubsystem>();
 if(!G || !G->GetCommission(Context->CommissionId,C)){HandleResponse(Context,false,TEXT("commission_missing"));return;}
 auto Data=MakeShared<FJsonObject>();Data->SetStringField(TEXT("task_catalog"),AlchemyDialogueKnowledge::Catalog(C,Context->Familiarity));
 Data->SetBoolField(TEXT("may_accept"),Context->bAllowAcceptance);Data->SetBoolField(TEXT("presentation"),Context->bPresentation);Data->SetNumberField(TEXT("current_layer"),Context->Familiarity);
 Data->SetBoolField(TEXT("format_retry"),Context->PlanRepairs>0);
 if(!Context->PlanRepairNotice.IsEmpty())Data->SetStringField(TEXT("plan_validation_error"),Context->PlanRepairNotice);
 FDialoguePersonaProfile Profile;
 if(G->GetPersonaProfile(Context->NpcId,Profile))
 {
  TArray<TSharedPtr<FJsonValue>> Unlocks;
  for(const auto& Layer:Profile.Layers)if(Layer.FamiliarityLevel>Context->Familiarity)
  {auto O=MakeShared<FJsonObject>();O->SetNumberField(TEXT("layer"),Layer.FamiliarityLevel);O->SetStringField(TEXT("topic"),Layer.Summary);TArray<TSharedPtr<FJsonValue>> Hints;for(const auto& K:Layer.TriggerKeywords)Hints.Add(MakeShared<FJsonValueString>(K));O->SetArrayField(TEXT("topic_hints_not_literal_triggers"),Hints);Unlocks.Add(MakeShared<FJsonValueObject>(O));}
  Data->SetArrayField(TEXT("unlock_topics"),Unlocks);
 }
 TArray<TSharedPtr<FJsonValue>> Known;for(const auto& Id:Row(Context->NpcId).DeliveredTaskFacts)Known.Add(MakeShared<FJsonValueString>(Id.ToString()));Data->SetArrayField(TEXT("already_delivered_fact_ids"),Known);
 TArray<TSharedPtr<FJsonValue>> Evidence;
 for(int32 I=0;I<Context->Evidence.Num();++I){auto M=MakeShared<FJsonObject>();M->SetNumberField(TEXT("index"),I);M->SetStringField(TEXT("role"),Context->Evidence[I].Role);M->SetStringField(TEXT("content"),Context->Evidence[I].Content);Evidence.Add(MakeShared<FJsonValueObject>(M));}
 Data->SetArrayField(TEXT("messages"),Evidence);
 FDialogueAICompletion Callback;Callback.BindDynamic(Context,&UForestDialogueRequestContext::Completed);
 GetGameInstance()->GetSubsystem<UDialogueAIServiceSubsystem>()->RequestScopedCompletion({ForestDialogue::Message(TEXT("system"),AlchemyDialogueKnowledge::AnalysisRules()),ForestDialogue::Message(TEXT("user"),ForestDialogue::Json(Data))},0.f,850,Callback,true,Context->Scope);
}
void UForestNPCDialogueSubsystem::RequestNpcReply(UForestDialogueRequestContext* Context)
{
 Context->bAcceptanceCheck=false;Requests.AddUnique(Context);
 auto* G=GetGameInstance()->GetSubsystem<UAlchemyGameplaySubsystem>();auto* AI=GetGameInstance()->GetSubsystem<UDialogueAIServiceSubsystem>();
 FAlchemyCommissionDefinition C;if(!G || !AI || !G->GetCommission(Context->CommissionId,C)){HandleResponse(Context,false,TEXT("dialogue_service_missing"));return;}
 TArray<FName> Known=Row(Context->NpcId).DeliveredTaskFacts;for(const auto& Id:Row(Context->NpcId).DeliveredPersonaFacts)Known.AddUnique(Id);
 Context->AllowedFacts=AlchemyDialogueKnowledge::AllowedFacts(C,Context->Familiarity,Context->SelectedFacts,Known);
 FString Prompt=G->BuildPersonaSystemPrompt(Context->NpcId,Context->Familiarity,Context->CommissionId,GetCompactMemoryJson(Context->NpcId),{});
 Prompt+=AlchemyDialogueKnowledge::FactGuidance(C,Context->Familiarity,Context->AllowedFacts,Known);
 Prompt+=TEXT("\n【本轮意图】\n")+Context->Intent;
 if(auto* A=GetWorld()->GetSubsystem<UForestAlchemy>())Prompt+=TEXT("\n【程序确认的委托状态】\n")+A->TaskStatusForNpc(Context->NpcId);
 if(!Context->AcceptanceNotice.IsEmpty())Prompt+=TEXT("\n【本轮登记结果，必须遵守】\n")+Context->AcceptanceNotice;
 TArray<FDialogueMessage> Messages;Messages.Add(ForestDialogue::Message(TEXT("system"),Prompt));
 for(int32 I=FMath::Max(0,Context->Evidence.Num()-8);I<Context->Evidence.Num();++I)Messages.Add(Context->Evidence[I]);
 FName Fallback=TEXT("task.purpose");
 for(const auto& Id:Context->SelectedFacts)if(Context->AllowedFacts.Contains(Id) && Id.ToString().StartsWith(TEXT("task."))){Fallback=Id;break;}
 FString FallbackText=AlchemyDialogueKnowledge::FactText(C,Context->Familiarity,Fallback);
 if(Context->Intent==TEXT("guess_formula")){FallbackText=TEXT("数字我不替你判断。要看的是材料需要达到什么效果。");Fallback=NAME_None;}
 else if(Context->Intent==TEXT("backstory") || Context->Intent==TEXT("smalltalk") || Context->Intent==TEXT("show_item")){FallbackText=AlchemyPersonaPolicy::CleanReply(Context->NpcId,FString());Fallback=NAME_None;}
 else if(Context->Intent==TEXT("other")){FallbackText=TEXT("你想问委托的用途，还是调配步骤？要回小屋合成台调配并封瓶，不用把原料当面交给我。");Fallback=NAME_None;}
 if(Context->bPresentation){FallbackText=TEXT("东西我看到了。你想问它的性质，还是它和这份委托的关系？");Fallback=NAME_None;}
 if(!Context->AcceptanceNotice.IsEmpty())FallbackText=Context->AcceptanceNotice.Contains(TEXT("登记保存成功"))?TEXT("这份委托记下了。回小屋合成台调配并封瓶，接到的事可以在魔法书里查。"):TEXT("这次还没记下委托，稍后再确认。回小屋合成台调配，不用把原料交给我。");
 if(!Context->AcceptanceNotice.IsEmpty())Fallback=NAME_None;
 FAlchemyNpcReplyCompletion Callback;Callback.BindDynamic(Context,&UForestDialogueRequestContext::ReplyCompleted);
 AI->RequestNpcTurn(Context->NpcId,Messages,AlchemyDialogueKnowledge::Catalog(C,Context->Familiarity),Context->AllowedFacts,Fallback,FallbackText,Callback,Context->Scope);
}
FString UForestNPCDialogueSubsystem::OfflineReply(const FString& Text) const
{
 const FString Name=ActiveNpc==TEXT("natta")?TEXT("Natta"):ActiveNpc==TEXT("fawnia")?TEXT("Fawnia"):TEXT("森林女巫");
 FString Previous;for(const auto& S:GetGameInstance()->GetSubsystem<UDialogueMemoryTableSubsystem>()->GetLedgerSnapshot().NpcStates)if(S.NpcId==ActiveNpc)Previous=AlchemyPersonaPolicy::ContextText(ActiveNpc,S.LastExchangeSummary);
 if(Text.Contains(TEXT("记得")) && !Previous.IsEmpty())return FString::Printf(TEXT("我是%s。记得，上次我们聊到：%s"),*Name,*Previous);
 const FName Canon=AlchemyPersonaPolicy::CanonicalId(ActiveNpc);
 if(Canon==TEXT("blacksmith"))return TEXT("听见了。你想问具体用途，就把问题说清楚。");
 if(Canon==TEXT("operator"))return TEXT("诶，听见了。这回我先听你说完，免得我又绕远了。");
 return TEXT("你这句话，我倒想记下来。先说正事，你还想问什么？");
}
void UForestNPCDialogueSubsystem::HandleResponse(UForestDialogueRequestContext* Context,bool Success,const FString& Text)
{
 Requests.Remove(Context);if(!Save)return;
 auto& R=Row(Context->NpcId);
 if(Context->bSummary)
 {
  if(R.PendingSession!=Context->Session){RequestSummary(R);return;}
  if(Success)
  {
   const int32 WorldDay=GetGameInstance()->GetSubsystem<UDailyMushroomSubsystem>()->GetCurrentDay();
   const auto Result=GetGameInstance()->GetSubsystem<UDialogueMemoryTableSubsystem>()->ApplyMemoryPatchJson(AlchemyPersonaPolicy::FilterMemoryPatch(R.NpcId,ForestDialogue::CleanJson(Text)),R.NpcId,R.PendingSession,WorldDay,true);
   Success=Result.bSuccess;if(!Success)Status=TEXT("记忆校验失败，已保留待重试会话：")+Result.ErrorCode.ToString();
  }
  else Status=TEXT("记忆总结暂不可用，会话已保存，稍后可重试");
  if(Success){R.PendingSession=0;R.PendingTranscript.Reset();Status=bOfflineTest?TEXT("离线联调记忆已保存"):TEXT("对话记忆已保存");}
  Persist();OnConversationUpdated.Broadcast();return;
 }
 if(!GetGameInstance()->GetSubsystem<UDailyMushroomSubsystem>() || !AlchemyDialogueTurnPolicy::IsFresh(Context->Generation,Generation,Context->Day,GetGameInstance()->GetSubsystem<UDailyMushroomSubsystem>()->GetCurrentDay(),Context->NpcId==ActiveNpc,!ActiveNpc.IsNone()) || (!bOfflineTest && Context->KnowledgeFingerprint!=AlchemyDialogueKnowledge::Fingerprint()))
 {
  if(ActiveTurnScope && ActiveTurnScope==Context->Scope)
  {const auto Old=ActiveTurnScope;ActiveTurnScope.Reset();bWaiting=false;GetGameInstance()->GetSubsystem<UDialogueAIServiceSubsystem>()->CancelNpcTurns(Old);Status=TEXT("任务或知识已更新，请重新交谈。");OnConversationUpdated.Broadcast();}
  return;
 }
 if(Context->bAcceptanceCheck)
 {
  bool Accepted=false;double Offer=-1,Accept=-1;FString OfferQuote,AcceptQuote;TSharedPtr<FJsonObject> Decision;
  bool Valid=Success && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(ForestDialogue::CleanJson(Text)),Decision) && Decision.IsValid() &&
   Decision->TryGetBoolField(TEXT("accepted"),Accepted) && Accepted && Decision->TryGetNumberField(TEXT("offer_index"),Offer) && Decision->TryGetNumberField(TEXT("accept_index"),Accept) &&
   Offer>=0 && Accept>=0 && Offer<Context->Evidence.Num() && Accept<Context->Evidence.Num() && Offer==FMath::FloorToDouble(Offer) && Accept==FMath::FloorToDouble(Accept) &&
   Decision->TryGetStringField(TEXT("offer_quote"),OfferQuote) && Decision->TryGetStringField(TEXT("accept_quote"),AcceptQuote);
  FString Intent;TArray<FName> Selected;bool PlanValid=false;
  const TArray<TSharedPtr<FJsonValue>>* Ids=nullptr;
  if(Decision && Decision->TryGetStringField(TEXT("intent"),Intent) && Decision->TryGetArrayField(TEXT("fact_ids"),Ids))
  {
   const TArray<FString> Intents={TEXT("task_question"),TEXT("smalltalk"),TEXT("backstory"),TEXT("guess_formula"),TEXT("accept"),TEXT("reject"),TEXT("show_item"),TEXT("other")};PlanValid=Intents.Contains(Intent);
   FAlchemyCommissionDefinition C;auto* G=GetGameInstance()->GetSubsystem<UAlchemyGameplaySubsystem>();TArray<FName> CatalogIds,Effects,Bindings;
   if(G->GetCommission(Context->CommissionId,C))for(const auto& F:AlchemyDialogueKnowledge::Facts(C,Context->Familiarity))if(F.Topic!=TEXT("constraint"))
   {
    CatalogIds.Add(F.Id);
    for(const TCHAR* Element:{TEXT("fire"),TEXT("wind"),TEXT("water")})if(F.Id.ToString().StartsWith(FString(TEXT("task."))+Element+TEXT(".effect.")))
    {Effects.Add(F.Id);Bindings.Add(FName(*(FString(TEXT("task."))+Element+TEXT(".a"))));}
   }
   for(const auto& V:*Ids){FString Id;if(!V->TryGetString(Id) || !CatalogIds.Contains(FName(*Id))){PlanValid=false;break;}Selected.AddUnique(FName(*Id));}
   TArray<FName> TaskSelected;for(const auto& Id:Selected)if(Id.ToString().StartsWith(TEXT("task.")))TaskSelected.AddUnique(Id);
   PlanValid=PlanValid && AlchemyDialogueTurnPolicy::WithinNewFactBudget(TaskSelected,R.DeliveredTaskFacts,2);
   PlanValid=PlanValid && AlchemyDialogueTurnPolicy::ValidateBindings(TaskSelected,R.DeliveredTaskFacts,Effects,Bindings);
  }
  if(!PlanValid)Context->PlanRepairNotice=TEXT("上次计划未通过本地校验。intent只能使用规则中的枚举；fact_ids只能选当前目录里的非约束编号。每轮最多两条尚未实际讲述的任务事实：如果超过两条，请按本轮问题只保留最相关两条，其余不要选，不要丢弃整个相关主题。图鉴effect事实必须与同元素的task.<element>.a用途对象一起选入，或该用途对象已经实际讲过；不能只选抽象图鉴效果。已讲事实可复用。只修复计划，不添加动作证据。上次输出仅作查错资料：\n")+Text.Left(4000);
  if(!Success)
  {
   if((Text==TEXT("response_empty") || Text==TEXT("response_truncated")) && Context->PlanRepairs++==0){RequestAcceptanceCheck(Context);return;}
   if(Text!=TEXT("response_empty") && Text!=TEXT("response_truncated")){Context->bAcceptanceCheck=false;HandleResponse(Context,false,Text);return;}
  }
  if(!PlanValid && Context->PlanRepairs++==0){RequestAcceptanceCheck(Context);return;}
  Context->Intent=PlanValid?Intent:TEXT("other");Context->SelectedFacts=PlanValid?Selected:TArray<FName>();
  // Invalid action/plan sections never authorize side effects. Valid smalltalk is
  // still answerable without accepting a task or opening fresh knowledge.
  if(!PlanValid){Valid=false;Accepted=false;Context->AcceptanceNotice=Context->bAllowAcceptance?TEXT("本轮需求识别未完成，未登记新的委托，不得声称已接取。"):FString();}
  if(PlanValid)
  {
   double Requested=Context->Familiarity;bool Personal=false;FString Proof,Latest;
   Decision->TryGetNumberField(TEXT("requested_layer"),Requested);Decision->TryGetBoolField(TEXT("has_unlock_evidence"),Personal);Decision->TryGetStringField(TEXT("layer_evidence"),Proof);
   for(int32 I=Context->Evidence.Num()-1;I>=0;--I)if(Context->Evidence[I].Role==TEXT("user")){Latest=Context->Evidence[I].Content;break;}
   const bool Evidence=!Proof.TrimStartAndEnd().IsEmpty() && Latest.Contains(Proof) && Requested==FMath::FloorToDouble(Requested);
   if(AlchemyDialogueTurnPolicy::MayPromote(Context->Familiarity,static_cast<int32>(Requested),Evidence,Personal,Context->bPresentation))
   {
    auto* Memory=GetGameInstance()->GetSubsystem<UDialogueMemoryTableSubsystem>();int64 Flags=0;for(const auto& S:Memory->GetLedgerSnapshot().NpcStates)if(S.NpcId==Context->NpcId)Flags=S.KnownSecretFlags;
    FString Error;if(Memory->SetAuthoritativeNpcProgress(Context->NpcId,static_cast<int32>(Requested),Flags,Context->Day,true,Error))Context->Familiarity=static_cast<int32>(Requested);
   }
  }
  if(!Context->bAllowAcceptance){Valid=false;Accepted=false;}
  // Preserve an evidenced offer independently of agreement. Extended questioning
  // must not erase the commission merely because it left the recent-turn window.
  double OfferOnly=-1;FString QuoteOnly;
  if(Success && Decision.IsValid() && Decision->TryGetNumberField(TEXT("offer_index"),OfferOnly) && OfferOnly>=0 && OfferOnly<Context->Evidence.Num()-1 && OfferOnly==FMath::FloorToDouble(OfferOnly) && Decision->TryGetStringField(TEXT("offer_quote"),QuoteOnly) && !QuoteOnly.TrimStartAndEnd().IsEmpty())
  {
   const auto& Offered=Context->Evidence[static_cast<int32>(OfferOnly)];
   if(Offered.Role==TEXT("assistant") && Offered.Content.Contains(QuoteOnly))
   {R.OfferDay=Context->Day;R.OfferCommissionId=Context->CommissionId;R.OfferText=Offered.Content;Persist();}
  }
  if(Valid)
  {
   const int32 O=static_cast<int32>(Offer),U=static_cast<int32>(Accept);
   Valid=ForestCommissionAcceptance::EvidenceIsValid(true,O,U,Context->Evidence.Num(),Context->Evidence[O].Role==TEXT("assistant"),Context->Evidence[U].Role==TEXT("user"),!OfferQuote.TrimStartAndEnd().IsEmpty() && Context->Evidence[O].Content.Contains(OfferQuote),!AcceptQuote.TrimStartAndEnd().IsEmpty() && Context->Evidence[U].Content.Contains(AcceptQuote));
  }
  if(Valid)
  {
   auto* A=GetWorld()->GetSubsystem<UForestAlchemy>();
   const bool Saved=A && A->RecordConversationAcceptance(Context->NpcId,Context->Day);
   Context->AcceptanceNotice=Saved?TEXT("本轮对话已确认玩家承担委托且登记保存成功。自然回应玩家的答应，提醒回小屋合成；不报目标数值。"):TEXT("本轮登记保存失败，未接取。请告知尚未记下，让玩家稍后再次确认；不得声称任务已登记。");
  }
  else if(Context->bAllowAcceptance && (Accepted))Context->AcceptanceNotice=TEXT("本轮接取识别暂不可用，未登记。若玩家是在答应承担委托，请说明还需再次确认；不得声称已接取。");
  if(!Context->AcceptanceNotice.IsEmpty()){R.LastActionReceipt=Context->AcceptanceNotice.Contains(TEXT("登记保存成功"))?TEXT("委托已接取，已记入魔法书。回小屋合成台调配并封瓶。"):TEXT("委托尚未登记，可继续交谈确认。");Persist();Status=Context->AcceptanceNotice.Contains(TEXT("登记保存成功"))?TEXT("委托已接取，已记入魔法书；对方正在回答…"):TEXT("本轮尚未登记委托；对方正在回答…");OnConversationUpdated.Broadcast();}
  RequestNpcReply(Context);return;
 }
 bWaiting=false;
 if(Success)
 {
  if(Context->bVerifiedReply)
  {
   TArray<FName> Task,Persona;
   for(const auto& Id:Context->VerifiedReply.DeliveredFacts)(Id.ToString().StartsWith(TEXT("person."))?Persona:Task).AddUnique(Id);
   AlchemyDialogueTurnPolicy::CommitDelivered(Context->AllowedFacts,Task,R.DeliveredTaskFacts,true);
   AlchemyDialogueTurnPolicy::CommitDelivered(Context->AllowedFacts,Persona,R.DeliveredPersonaFacts,true);
   R.bLastReplyFallback=Context->VerifiedReply.bFallback;
  }
  const auto M=ForestDialogue::Message(TEXT("assistant"),Text);R.History.Add(M);SessionTranscript.Add(M);while(R.History.Num()>16)R.History.RemoveAt(0);
  R.LastReplyError.Reset();Status=bOfflineTest?TEXT("离线联调回复，不判定接取"):Context->AcceptanceNotice.IsEmpty()?TEXT("Enter 发送 · Esc 结束"):R.LastActionReceipt;
 }
 else
 {
  // The player's actual statement and any committed task receipt survive failure.
  R.LastReplyError=Text;Status=TEXT("暂时没能收到答复，请稍后重试。");
  if(!Context->AcceptanceNotice.IsEmpty())Status+=TEXT("\n")+R.LastActionReceipt;
 }
 ActiveTurnScope.Reset();Persist();OnConversationUpdated.Broadcast();
}
FString UForestNPCDialogueSubsystem::OfflinePatch(const FForestConversationRow& R) const
{
 auto Root=MakeShared<FJsonObject>();Root->SetNumberField(TEXT("schema_version"),1);Root->SetStringField(TEXT("npc_id"),R.NpcId.ToString());Root->SetNumberField(TEXT("session_id"),R.PendingSession);
 FString Last;for(const auto& M:R.PendingTranscript)if(M.Role==TEXT("user"))Last=M.Content.Left(140);
 auto Patch=MakeShared<FJsonObject>();Patch->SetStringField(TEXT("attitude"),TEXT("neutral"));Patch->SetStringField(TEXT("relationship_summary"),TEXT("离线联调：玩家与该角色完成了交谈。"));Patch->SetStringField(TEXT("last_exchange_summary"),Last.IsEmpty()?TEXT("离线测试交谈"):Last);Patch->SetArrayField(TEXT("topic_tags"),{});Root->SetObjectField(TEXT("npc_patch"),Patch);
 Root->SetArrayField(TEXT("fact_upserts"),{});Root->SetArrayField(TEXT("world_suggestions"),{});Root->SetArrayField(TEXT("open_loop_upserts"),{});return ForestDialogue::Json(Root);
}
void UForestNPCDialogueSubsystem::RequestSummary(FForestConversationRow& R)
{
 if(R.PendingSession<1 || R.PendingTranscript.IsEmpty())return;
 for(auto C:Requests)if(C->bSummary && C->NpcId==R.NpcId)return;
 auto* AI=GetGameInstance()->GetSubsystem<UDialogueAIServiceSubsystem>();if(!bOfflineTest && !AI->HasApiKey())return;
 auto* C=NewObject<UForestDialogueRequestContext>(this);C->Owner=this;C->NpcId=R.NpcId;C->Session=R.PendingSession;C->bSummary=true;Requests.Add(C);
 if(bOfflineTest){HandleResponse(C,true,OfflinePatch(R));return;}
 FString Prompt;auto Plugin=IPluginManager::Get().FindPlugin(TEXT("DialogueMemoryTable"));
 if(!Plugin.IsValid() || !FFileHelper::LoadFileToString(Prompt,*(Plugin->GetBaseDir()/TEXT("Resources/DialogueMemorySummarizerPrompt.md"))))
 {HandleResponse(C,false,TEXT("记忆提示词资源缺失"));return;}
 auto* Gameplay=GetGameInstance()->GetSubsystem<UAlchemyGameplaySubsystem>();FDialoguePersonaProfile Profile;
 int32 Familiarity=1;for(const auto& S:GetGameInstance()->GetSubsystem<UDialogueMemoryTableSubsystem>()->GetLedgerSnapshot().NpcStates)if(S.NpcId==R.NpcId)Familiarity=S.Familiarity;
 if(!Gameplay || !Gameplay->GetPersonaProfile(R.NpcId,Profile)){HandleResponse(C,false,TEXT("人物设定读取失败"));return;}
 Prompt+=TEXT("\n【当前权威人物设定与可披露范围】\n")+Profile.VoiceTone+TEXT("\n")+Profile.DailyState+TEXT("\n")+Profile.Layers[FMath::Clamp(Familiarity,1,3)-1].Prompt;
 FString Transcript;for(const auto& M:R.PendingTranscript)if(M.Role!=TEXT("assistant") || !AlchemyPersonaPolicy::ContainsRetiredBinding(R.NpcId,M.Content))Transcript+=M.Role+TEXT(": ")+M.Content+TEXT("\n");
 FString Input=FString::Printf(TEXT("npc_id=%s\nsession_id=%lld\n当前记忆=%s\n本次对话：\n%s\n严格只输出 JSON 对象。"),*R.NpcId.ToString(),R.PendingSession,*GetCompactMemoryJson(R.NpcId),*Transcript);
 FDialogueAICompletion Callback;Callback.BindDynamic(C,&UForestDialogueRequestContext::Completed);
 AI->RequestChatCompletion({ForestDialogue::Message(TEXT("system"),Prompt),ForestDialogue::Message(TEXT("user"),Input)},0.1f,1400,Callback);
}
void UForestNPCDialogueSubsystem::EndConversation()
{
 if(ActiveNpc.IsNone() || !Save)return;
 auto& R=Row(ActiveNpc);
 ++Generation;bWaiting=false;
 if(auto* AI=GetGameInstance()->GetSubsystem<UDialogueAIServiceSubsystem>())AI->CancelNpcTurns(ActiveTurnScope);ActiveTurnScope.Reset();
 if(!SessionTranscript.IsEmpty())
 {
  // A failed earlier summary is retained and included in the next attempt.
  R.PendingTranscript.Append(SessionTranscript);while(R.PendingTranscript.Num()>32)R.PendingTranscript.RemoveAt(0);
  R.PendingSession=R.NextSession++;Persist();
 }
 ActiveNpc=NAME_None;SessionTranscript.Reset();RequestSummary(R);Persist();OnConversationUpdated.Broadcast();
}
void UForestNPCDialogueSubsystem::RetryPendingSummaries()
{if(!Save)return;for(auto& R:Save->Rows)RequestSummary(R);}
int32 UForestNPCDialogueSubsystem::GetPendingSummaryCount() const
{int32 Count=0;if(Save)for(const auto& R:Save->Rows)if(R.PendingSession>0)++Count;return Count;}
void UForestNPCDialogueSubsystem::Deinitialize()
{EndConversation();Persist();for(auto R:Requests)R->Owner.Reset();Requests.Reset();Super::Deinitialize();}
