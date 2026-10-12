#include "ForestNPCDialogueSubsystem.h"
#include "DialogueAIServiceSubsystem.h"
#include "DialogueMemoryTableSubsystem.h"
#include "AlchemyGameplaySubsystem.h"
#include "DailyMushroomSubsystem.h"
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
 FDialoguePersonaProfile Profile;if(!GetGameInstance()->GetSubsystem<UAlchemyGameplaySubsystem>()->GetPersonaProfile(Id,Profile))return false;
 auto* Memory=GetGameInstance()->GetSubsystem<UDialogueMemoryTableSubsystem>();auto* Day=GetGameInstance()->GetSubsystem<UDailyMushroomSubsystem>();const int32 WorldDay=Day?Day->GetCurrentDay():1;int32 Level=1;int64 Flags=0,Last=0;
 for(const auto& S:Memory->GetLedgerSnapshot().NpcStates)if(S.NpcId==Id){Level=S.Familiarity;Flags=S.KnownSecretFlags;Last=S.LastSessionId;}
 FString Error;if(!Memory->SetAuthoritativeNpcProgress(Id,Level,Flags,WorldDay,true,Error)){Status=Error;return false;}
 auto& R=Row(Id);R.NextSession=FMath::Max(R.NextSession,Last+1);
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
 const FString Opening=TEXT("嗐，终于找到你了。新来的女巫，对吧？第一天先听我说清楚：林子里的蘑菇每天不一样，摘之前留意它带的是火、风还是水。接了委托，就拿当天能找到的材料去配；干锅里确认成品，这一天才算过完。路和人我熟，下一步找不着就来问我。");
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
 {Root->SetStringField(TEXT("relationship"),S.RelationshipSummary);Root->SetStringField(TEXT("last_exchange"),S.LastExchangeSummary);Root->SetNumberField(TEXT("familiarity"),S.Familiarity);Root->SetNumberField(TEXT("known_secret_flags"),S.KnownSecretFlags);}
 TArray<TSharedPtr<FJsonValue>> Facts,Loops,World;
 for(const auto& F:Snapshot.CharacterFacts)if(F.NpcId==Id && F.Status!=EDialogueMemoryFactStatus::Contradicted)
 {auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("key"),F.FactKey.ToString());O->SetStringField(TEXT("subject"),F.Subject.ToString());O->SetStringField(TEXT("predicate"),F.Predicate.ToString());O->SetStringField(TEXT("text"),F.ObjectText);Facts.Add(MakeShared<FJsonValueObject>(O));}
 for(const auto& L:Snapshot.OpenLoops)if(L.NpcId==Id)
 {auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("key"),L.LoopKey.ToString());O->SetStringField(TEXT("summary"),L.Summary);O->SetNumberField(TEXT("status"),static_cast<int32>(L.Status));Loops.Add(MakeShared<FJsonValueObject>(O));}
 for(const auto& W:Snapshot.WorldStates)
 {auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("key"),W.StateKey.ToString());O->SetStringField(TEXT("value"),W.ValueText);O->SetNumberField(TEXT("authority"),static_cast<int32>(W.Authority));World.Add(MakeShared<FJsonValueObject>(O));}
 Root->SetArrayField(TEXT("facts"),Facts);Root->SetArrayField(TEXT("open_loops"),Loops);Root->SetArrayField(TEXT("world"),World);return ForestDialogue::Json(Root);
}
bool UForestNPCDialogueSubsystem::SendPlayerMessage(const FString& Raw)
{
 const FString Text=Raw.TrimStartAndEnd();if(ActiveNpc.IsNone() || bWaiting || Text.IsEmpty() || Text.Len()>500)return false;
 auto* AI=GetGameInstance()->GetSubsystem<UDialogueAIServiceSubsystem>();
 if(!bOfflineTest && !AI->HasApiKey()){Status=TEXT("未配置 FOREST_DIALOGUE_API_KEY。设置后重新启动 UE；或在场景管理器启用离线联调。");OnConversationUpdated.Broadcast();return false;}
 auto* Gameplay=GetGameInstance()->GetSubsystem<UAlchemyGameplaySubsystem>();
 int64 KnownSecretFlags=0;
 for(const auto& State:GetGameInstance()->GetSubsystem<UDialogueMemoryTableSubsystem>()->GetLedgerSnapshot().NpcStates)
  if(State.NpcId==ActiveNpc){KnownSecretFlags=State.KnownSecretFlags;break;}
 int32 NewFamiliarity=1,TriggeredLayer=0;FString ProgressError;
 const int32 WorldDay=GetGameInstance()->GetSubsystem<UDailyMushroomSubsystem>()->GetCurrentDay();
 if(!Gameplay || !Gameplay->AdvancePersonaFromPlayerText(ActiveNpc,Text,KnownSecretFlags,WorldDay,true,NewFamiliarity,TriggeredLayer,ProgressError))
 {Status=ProgressError.IsEmpty()?TEXT("熟悉度更新失败，请稍后重试"):ProgressError;OnConversationUpdated.Broadcast();return false;}
 auto& R=Row(ActiveNpc);const auto M=ForestDialogue::Message(TEXT("user"),Text);R.History.Add(M);SessionTranscript.Add(M);bWaiting=true;Status=TEXT("对方正在回答…");Persist();OnConversationUpdated.Broadcast();
 if(bOfflineTest)
 {auto* Context=NewObject<UForestDialogueRequestContext>(this);Context->Owner=this;Context->NpcId=ActiveNpc;Context->Generation=Generation;Requests.Add(Context);HandleResponse(Context,true,OfflineReply(Text));return true;}
 int32 Level=1;for(const auto& S:GetGameInstance()->GetSubsystem<UDialogueMemoryTableSubsystem>()->GetLedgerSnapshot().NpcStates)if(S.NpcId==ActiveNpc)Level=S.Familiarity;
 const FName CommissionPersona=ActiveNpc==TEXT("natta")?TEXT("blacksmith"):ActiveNpc==TEXT("fawnia")?TEXT("operator"):ActiveNpc==TEXT("forest_witch")?TEXT("novelist"):ActiveNpc;
 const int32 PersonaOffset=CommissionPersona==TEXT("operator")?3:CommissionPersona==TEXT("novelist")?6:CommissionPersona==TEXT("blacksmith")?0:-1;
 const int32 DaySlot=((FMath::Max(1,WorldDay)-1)%3)+1;
 const int32 CommissionOrderIndex=PersonaOffset>=0?PersonaOffset+DaySlot:0;
 auto* Context=NewObject<UForestDialogueRequestContext>(this);Context->Owner=this;Context->NpcId=ActiveNpc;Context->Generation=Generation;Requests.Add(Context);
 FDialogueAICompletion Callback;Callback.BindDynamic(Context,&UForestDialogueRequestContext::Completed);
 AI->RequestNpcDialogue(ActiveNpc,Level,CommissionOrderIndex,GetCompactMemoryJson(ActiveNpc),R.History,Callback);return true;
}
FString UForestNPCDialogueSubsystem::OfflineReply(const FString& Text) const
{
 const FString Name=ActiveNpc==TEXT("natta")?TEXT("Natta"):ActiveNpc==TEXT("fawnia")?TEXT("Fawnia"):TEXT("森林女巫");
 FString Previous;for(const auto& S:GetGameInstance()->GetSubsystem<UDialogueMemoryTableSubsystem>()->GetLedgerSnapshot().NpcStates)if(S.NpcId==ActiveNpc)Previous=S.LastExchangeSummary;
 if(Text.Contains(TEXT("记得")) && !Previous.IsEmpty())return FString::Printf(TEXT("我是%s。记得，上次我们聊到：%s"),*Name,*Previous);
 return FString::Printf(TEXT("我是%s。听到了你说的“%s”。等交谈结束，我会保存这次交流的记录。"),*Name,*Text.Left(120));
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
   const auto Result=GetGameInstance()->GetSubsystem<UDialogueMemoryTableSubsystem>()->ApplyMemoryPatchJson(ForestDialogue::CleanJson(Text),R.NpcId,R.PendingSession,WorldDay,true);
   Success=Result.bSuccess;if(!Success)Status=TEXT("记忆校验失败，已保留待重试会话：")+Result.ErrorCode.ToString();
  }
  else Status=TEXT("记忆总结暂不可用，会话已保存，稍后可重试");
  if(Success){R.PendingSession=0;R.PendingTranscript.Reset();Status=bOfflineTest?TEXT("离线联调记忆已保存"):TEXT("对话记忆已保存");}
  Persist();OnConversationUpdated.Broadcast();return;
 }
 if(Context->Generation!=Generation || Context->NpcId!=ActiveNpc)return;
 bWaiting=false;
 if(Success)
 {const auto M=ForestDialogue::Message(TEXT("assistant"),Text.Left(4000));R.History.Add(M);SessionTranscript.Add(M);while(R.History.Num()>16)R.History.RemoveAt(0);Status=bOfflineTest?TEXT("离线联调回复 · Enter 发送 · Esc 结束"):TEXT("Enter 发送 · Esc 结束");}
 else
 {if(!R.History.IsEmpty())R.History.Pop();if(!SessionTranscript.IsEmpty())SessionTranscript.Pop();Status=TEXT("回复失败，可重试：")+Text.Left(250);}
 Persist();OnConversationUpdated.Broadcast();
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
 FString Transcript;for(const auto& M:R.PendingTranscript)Transcript+=M.Role+TEXT(": ")+M.Content+TEXT("\n");
 FString Input=FString::Printf(TEXT("npc_id=%s\nsession_id=%lld\n当前记忆=%s\n本次对话：\n%s\n严格只输出 JSON 对象。"),*R.NpcId.ToString(),R.PendingSession,*GetCompactMemoryJson(R.NpcId),*Transcript);
 FDialogueAICompletion Callback;Callback.BindDynamic(C,&UForestDialogueRequestContext::Completed);
 AI->RequestChatCompletion({ForestDialogue::Message(TEXT("system"),Prompt),ForestDialogue::Message(TEXT("user"),Input)},0.1f,1400,Callback);
}
void UForestNPCDialogueSubsystem::EndConversation()
{
 if(ActiveNpc.IsNone() || !Save)return;
 auto& R=Row(ActiveNpc);if(bWaiting){if(!R.History.IsEmpty())R.History.Pop();if(!SessionTranscript.IsEmpty())SessionTranscript.Pop();}
 ++Generation;bWaiting=false;
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
