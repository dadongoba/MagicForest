#include "ForestAlchemy.h"
#include "ForestHarvestMagic.h"
#include "ForestNPCInteraction.h"
#include "AlchemyGameplaySubsystem.h"
#include "DailyMushroomSubsystem.h"
#include "PotionMimicWorldSubsystem.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"
#include "Internationalization/Regex.h"
#include "InputCoreTypes.h"

namespace ForestAlchemy
{
 const FName Collected(TEXT("Forest.ArcanaCollected"));
 bool IsBottle(const UStaticMeshComponent* Mesh){if(!Mesh || !Mesh->GetStaticMesh())return false;const FString N=Mesh->GetStaticMesh()->GetName();return N.StartsWith(TEXT("SM_Potions_MDL_Potion")) || N.StartsWith(TEXT("SM_EmptyPotions_MDL_EmptyPotion"));}
 bool IsCauldron(const UStaticMeshComponent* Mesh){return Mesh && Mesh->GetStaticMesh() && Mesh->GetStaticMesh()->GetName().Contains(TEXT("Cauldron_lo")) && !Mesh->GetStaticMesh()->GetName().Contains(TEXT("Handle")) && !Mesh->GetStaticMesh()->GetName().Contains(TEXT("Chains"));}
 FString StableId(const AActor* Actor)
 {
  FString S=Actor->GetPathName();FRegexPattern P(TEXT("UEDPIE_[0-9]+_"));FRegexMatcher M(P,S);TArray<TPair<int32,int32>> Ranges;while(M.FindNext())Ranges.Emplace(M.GetMatchBeginning(),M.GetMatchEnding());for(int32 I=Ranges.Num()-1;I>=0;--I)S.RemoveAt(Ranges[I].Key,Ranges[I].Value-Ranges[I].Key);return S;
 }
 FAlchemyCardSequenceItem AsSequence(const FAlchemyCardDefinition& Card){FAlchemyCardSequenceItem R;R.CardId=Card.Id;R.Type=Card.Type;R.Cost=Card.Cost;R.BracketLength=Card.BracketLength;R.Element=Card.Element;R.PipValue=Card.PipValue;return R;}
}
int32 UForestAlchemy::GetDay() const{auto* GI=GetWorld()?GetWorld()->GetGameInstance():nullptr;auto* D=GI?GI->GetSubsystem<UDailyMushroomSubsystem>():nullptr;return D?D->GetCurrentDay():1;}
bool UForestAlchemy::Persist(){return Save && UGameplayStatics::SaveGameToSlot(Save,TEXT("ForestDailyArcana"),0);}
TArray<FForestDailyMajor> UForestAlchemy::GetTodayMajors() const{return Save && Save->Day==GetDay()?Save->Majors:TArray<FForestDailyMajor>();}
TArray<FForestBrewRecord> UForestAlchemy::GetBrewRecords() const{return Save?Save->Brews:TArray<FForestBrewRecord>();}
bool UForestAlchemy::GetMajorDefinition(FName Id,FAlchemyCardDefinition& Out) const
{
 auto* A=GetWorld()->GetGameInstance()->GetSubsystem<UAlchemyGameplaySubsystem>();if(!A)return false;for(const auto& Card:A->GetMajorCards())if(Card.Id==Id){Out=Card;return true;}return false;
}
void UForestAlchemy::Setup()
{
 auto* PC=UGameplayStatics::GetPlayerController(this,0);if(!PC || !PC->GetPawn())return;Player=PC;
 if(!Save){Save=Cast<UForestAlchemySave>(UGameplayStatics::LoadGameFromSlot(TEXT("ForestDailyArcana"),0));if(!Save)Save=Cast<UForestAlchemySave>(UGameplayStatics::CreateSaveGameObject(UForestAlchemySave::StaticClass()));}
 if(!Save){Status=TEXT("大牌记录无法创建。");return;}
 // A saved receipt precedes the day transition. Recovery never charges the material bag twice.
 if(Save->PendingAdvanceFromDay>0){auto* D=GetWorld()->GetGameInstance()->GetSubsystem<UDailyMushroomSubsystem>();if(GetDay()==Save->PendingAdvanceFromDay && !D->ConfirmDryPotSynthesis())return;Save->PendingAdvanceFromDay=0;}
 SyncDay();Input=NewObject<UInputComponent>(PC);Input->RegisterComponent();Input->Priority=200;
 Input->BindKey(EKeys::E,IE_Pressed,this,&UForestAlchemy::Interact).bConsumeInput=false;PC->PushInputComponent(Input);
 Notice=CreateWidget<UForestArcanaNotice>(PC);if(Notice){Notice->AddToViewport(40);Notice->SetVisibility(ESlateVisibility::Collapsed);}
 bReady=true;Discover();
}
bool UForestAlchemy::SyncDay()
{
 if(!Save || Save->Day==GetDay())return true;
 const int32 OldDay=Save->Day,OldSeed=Save->Seed;const auto OldCards=Save->Majors;const auto OldClaims=Save->ClaimedBottles;
 Save->Day=GetDay();Save->Seed=static_cast<int32>(FCrc::StrCrc32(*FGuid::NewGuid().ToString()));Save->Majors.Reset();Save->ClaimedBottles.Reset();
 if(!Persist()){Save->Day=OldDay;Save->Seed=OldSeed;Save->Majors=OldCards;Save->ClaimedBottles=OldClaims;Tell(TEXT("今日大牌记录保存失败，暂不能抽取。"));return false;}
 for(auto& B:Bottles)if(auto* Actor=B.Actor.Get()){Actor->Tags.Remove(ForestAlchemy::Collected);Actor->SetActorHiddenInGame(B.bHidden);Actor->SetActorEnableCollision(B.bCollision);Actor->SetActorTransform(B.Home);}
 if(IsWorkbenchOpen())CloseWorkbench();CommissionId=0;bNeedsCabinSpawn=true;ClearSequence();return true;
}
void UForestAlchemy::Discover()
{
 for(TActorIterator<AActor> It(GetWorld());It;++It)
 {
  AActor* Actor=*It;auto* Mesh=Actor->FindComponentByClass<UStaticMeshComponent>();
  if(ForestAlchemy::IsCauldron(Mesh)){Cauldrons.AddUnique(Actor);continue;}
  if(!ForestAlchemy::IsBottle(Mesh) || Bottles.ContainsByPredicate([Actor](const FBottle& B){return B.Actor.Get()==Actor;}))continue;
  FBottle B;B.Actor=Actor;B.Mesh=Mesh;B.Key=ForestAlchemy::StableId(Actor);B.Home=Actor->GetActorTransform();if(auto* Mimic=GetWorld()->GetSubsystem<UPotionMimicWorldSubsystem>())Mimic->GetWorkbenchPotionHomeTransform(Actor,B.Home);B.bHidden=Actor->IsHidden();B.bCollision=Actor->GetActorEnableCollision();Bottles.Add(B);
  if(Save && Save->ClaimedBottles.Contains(B.Key)){Actor->Tags.AddUnique(ForestAlchemy::Collected);Actor->SetActorHiddenInGame(true);Actor->SetActorEnableCollision(false);}
 }
}
void UForestAlchemy::StartInCabin()
{
 auto* PC=Player.Get();APawn* Pawn=PC?PC->GetPawn():nullptr;if(!Pawn)return;APlayerStart* Start=nullptr;
 for(TActorIterator<APlayerStart> It(GetWorld());It;++It)
 {
  if(It->ActorHasTag(TEXT("Forest.CabinSpawn"))){Start=*It;break;}
  if(It->GetLevel()->GetOutermost()->GetName().Contains(TEXT("Witch_House")))Start=*It;
 }
 // Level instances may have generated package names. Only accept a start in the same indoor level as a cauldron.
 if(!Start)for(TActorIterator<APlayerStart> It(GetWorld());It;++It)for(auto& Weak:Cauldrons)if(auto* C=Weak.Get())if(It->GetLevel()==C->GetLevel() && FVector::DistSquared(It->GetActorLocation(),C->GetActorLocation())<FMath::Square(1200.f))Start=*It;
 if(!Start)return;const FRotator Facing(0,Start->GetActorRotation().Yaw,0);
 if(Pawn->TeleportTo(Start->GetActorLocation(),Facing,false,false)){PC->StopMovement();PC->SetControlRotation(Facing);bNeedsCabinSpawn=false;Tell(FString::Printf(TEXT("第 %d 天 · 在小屋醒来\n拿起药瓶可获得今日大牌，按 B 查看。"),GetDay()));}
}
bool UForestAlchemy::OtherMenuOpen() const
{
 if(auto* H=GetWorld()->GetSubsystem<UForestHarvestMagic>();H && H->IsInventoryOpen())return true;
 if(auto* M=Cast<AForestNPCInteraction>(UGameplayStatics::GetActorOfClass(GetWorld(),AForestNPCInteraction::StaticClass()));M && (M->IsDialogueOpen() || M->IsFinderMenuOpen()))return true;
 return false;
}
AActor* UForestAlchemy::FindFocus() const
{
 auto* PC=Player.Get();APawn* Pawn=PC?PC->GetPawn():nullptr;if(!Pawn || OtherMenuOpen() || IsWorkbenchOpen())return nullptr;
 FVector Eye;FRotator Rotation;Pawn->GetActorEyesViewPoint(Eye,Rotation);const FVector End=Eye+Rotation.Vector()*FMath::Clamp(GetDefault<UForestAlchemyRules>()->InteractionDistance,50.f,500.f);
 AActor* Best=nullptr;float BestDistance=MAX_flt;
 auto Consider=[&](AActor* Actor,UStaticMeshComponent* Mesh)
 {
  if(!Actor || !Mesh || Actor->IsHidden())return;
  const FBox Box=Mesh->Bounds.GetBox().ExpandBy(5);if(!FMath::LineBoxIntersection(Box,Eye,End,End-Eye))return;
  const FVector Point=Box.GetClosestPointTo(Eye);const float Distance=FVector::DistSquared(Point,Eye);if(Distance>=BestDistance)return;
  FCollisionQueryParams Params(SCENE_QUERY_STAT(ArcanaBottleSight),false);Params.AddIgnoredActor(Pawn);Params.AddIgnoredActor(Actor);FHitResult Hit;
  if(GetWorld()->LineTraceSingleByChannel(Hit,Eye,Mesh->Bounds.Origin,ECC_Visibility,Params))return;
  Best=Actor;BestDistance=Distance;
 };
 for(const auto& B:Bottles)Consider(B.Actor.Get(),B.Mesh.Get());for(const auto& C:Cauldrons)if(C.IsValid())Consider(C.Get(),C->FindComponentByClass<UStaticMeshComponent>());return Best;
}
void UForestAlchemy::Interact(){AActor* Actor=FindFocus();if(!Actor)return;if(ForestAlchemy::IsBottle(Actor->FindComponentByClass<UStaticMeshComponent>()))TakeBottle(Actor);else OpenWorkbench();}
bool UForestAlchemy::TakeBottle(AActor* Actor)
{
 if(!Save || !SyncDay() || FindFocus()!=Actor)return false;auto* B=Bottles.FindByPredicate([Actor](const FBottle& Row){return Row.Actor.Get()==Actor;});if(!B || Save->ClaimedBottles.Contains(B->Key))return false;
 const auto* Rules=GetDefault<UForestAlchemyRules>();if(Rules->MaxDailyDraws>0 && Save->ClaimedBottles.Num()>=Rules->MaxDailyDraws){Tell(TEXT("今天可抽取的大牌已达到配置上限。"));return false;}
 for(const FName Tag:Rules->RequiredBottleTags)if(!Actor->ActorHasTag(Tag)){Tell(TEXT("这瓶药剂尚不符合抽牌条件。"));return false;}
 auto Pool=GetWorld()->GetGameInstance()->GetSubsystem<UAlchemyGameplaySubsystem>()->GetMajorCards();if(!Rules->AllowedMajorIds.IsEmpty())Pool.RemoveAll([Rules](const auto& Card){return !Rules->AllowedMajorIds.Contains(Card.Id);});if(Pool.IsEmpty()){Tell(TEXT("当前没有配置可抽取的大牌。"));return false;}
 FRandomStream RNG(static_cast<int32>(HashCombine(static_cast<uint32>(Save->Seed),FCrc::StrCrc32(*B->Key))));const auto Card=Pool[RNG.RandRange(0,Pool.Num()-1)];
 FForestDailyMajor Entry;Entry.InstanceId=FGuid::NewGuid().ToString();Entry.CardId=Card.Id;Entry.SourceBottle=B->Key;Entry.Day=GetDay();Save->Majors.Add(Entry);Save->ClaimedBottles.Add(B->Key);
 if(!Persist()){Save->Majors.Pop();Save->ClaimedBottles.Pop();Tell(TEXT("保存失败，药瓶没有被取走，请重试。"));return false;}
 Actor->Tags.AddUnique(ForestAlchemy::Collected);Actor->SetActorHiddenInGame(true);Actor->SetActorEnableCollision(false);
 Tell(FString::Printf(TEXT("你拿起药瓶，瓶中浮出一张大牌。\n获得：%s · %s\n已放入背包「今日大牌」 · 按 B 查看"),*Card.Id.ToString(),*Card.Name));return true;
}
void UForestAlchemy::Tick(float DT)
{
 if(!GetWorld() || !GetWorld()->IsGameWorld())return;if(!bReady){Setup();return;}if(!Player.IsValid() || !Player->GetPawn())return;
 if(!SyncDay())return;ScanTime+=DT;SpawnTime+=DT;if(ScanTime>=.75f){ScanTime=0;Discover();}if(bNeedsCabinSpawn && SpawnTime>=.5f){SpawnTime=0;StartInCabin();}
 Focus=FindFocus();if(Input && Input->KeyBindings.Num())Input->KeyBindings[0].bConsumeInput=Focus.IsValid();
 if(Notice && Notice->Text.IsValid())
 {
  FString Message;if(GetWorld()->GetTimeSeconds()<ToastUntil)Message=Toast;else if(Focus.IsValid())Message=ForestAlchemy::IsBottle(Focus->FindComponentByClass<UStaticMeshComponent>())?TEXT("E　拿起药瓶 · 获得一张今日大牌"):TEXT("E　进入夜间合成台");
  Notice->Text->SetText(FText::FromString(Message));Notice->SetVisibility(Message.IsEmpty() || OtherMenuOpen() || IsWorkbenchOpen()?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
 }
}
void UForestAlchemy::Tell(const FString& Message){Toast=Message;ToastUntil=GetWorld()->GetTimeSeconds()+5;Status=Message;}
TMap<FString,int32> UForestAlchemy::ReadBag() const
{
 TMap<FString,int32> Out;APawn* Pawn=Player.IsValid()?Player->GetPawn():nullptr;if(!Pawn)return Out;
 auto* P=FindFProperty<FMapProperty>(Pawn->GetClass(),TEXT("Backpack"));auto* K=P?CastField<FStrProperty>(P->KeyProp):nullptr;auto* V=P?CastField<FIntProperty>(P->ValueProp):nullptr;if(!K || !V)return Out;FScriptMapHelper H(P,P->ContainerPtrToValuePtr<void>(Pawn));for(int I=0;I<H.GetMaxIndex();++I)if(H.IsValidIndex(I))Out.Add(K->GetPropertyValue(H.GetKeyPtr(I)),V->GetPropertyValue(H.GetValuePtr(I)));return Out;
}
bool UForestAlchemy::WriteBag(const TMap<FString,int32>& Bag)
{
 APawn* Pawn=Player.IsValid()?Player->GetPawn():nullptr;if(!Pawn)return false;auto* P=FindFProperty<FMapProperty>(Pawn->GetClass(),TEXT("Backpack"));if(!P || !CastField<FStrProperty>(P->KeyProp) || !CastField<FIntProperty>(P->ValueProp))return false;
 FScriptMapHelper H(P,P->ContainerPtrToValuePtr<void>(Pawn));H.EmptyValues();for(const auto& Pair:Bag)if(Pair.Value>0){FString K=Pair.Key;int32 V=Pair.Value;H.AddPair(&K,&V);}if(auto* F=Pawn->FindFunction(TEXT("RefreshHUD")))Pawn->ProcessEvent(F,nullptr);return true;
}
TArray<FForestBrewChoice> UForestAlchemy::Choices() const
{
 TArray<FForestBrewChoice> Out;auto* H=GetWorld()->GetSubsystem<UForestHarvestMagic>();auto* A=GetWorld()->GetGameInstance()->GetSubsystem<UAlchemyGameplaySubsystem>();if(!H || !A)return Out;
 for(const auto& Pair:ReadBag())if(Pair.Value>0){FForestMagicItem I;if(!H->FindItem(Pair.Key,I))continue;FForestBrewChoice C;C.Key=TEXT("pip:")+Pair.Key;C.Name=I.Name+FString::Printf(TEXT(" · %s%d"),*UForestHarvestMagic::ElementText(I.Element),I.PipValue);C.Description=I.Description;C.Card.Type=EAlchemyCardType::Pip;C.Card.CardId=FName(*I.Id);C.Card.Element=I.Element;C.Card.PipValue=I.PipValue;Out.Add(C);}
 Out.Sort([](const auto& L,const auto& R){return L.Key<R.Key;});
 for(const auto& I:GetTodayMajors()){FAlchemyCardDefinition D;if(GetMajorDefinition(I.CardId,D)){FForestBrewChoice C;C.Key=TEXT("major:")+I.InstanceId;C.Name=D.Id.ToString()+TEXT(" · ")+D.Name;C.Description=D.Description;C.Card=ForestAlchemy::AsSequence(D);Out.Add(C);}}
 for(const auto& D:A->GetCourtCards()){FForestBrewChoice C;C.Key=TEXT("court:")+D.Id.ToString();C.Name=D.Name;C.Description=D.Description;C.Card=ForestAlchemy::AsSequence(D);Out.Add(C);}return Out;
}
TArray<FAlchemyCommissionDefinition> UForestAlchemy::Commissions() const
{
 TArray<FAlchemyCommissionDefinition> Out;for(const auto& C:GetWorld()->GetGameInstance()->GetSubsystem<UAlchemyGameplaySubsystem>()->GetCommissions())if(C.CustomerOrderIndex==FMath::Clamp(GetDay(),1,3))Out.Add(C);return Out;
}
bool UForestAlchemy::CurrentCommission(FAlchemyCommissionDefinition& Out) const{return GetWorld()->GetGameInstance()->GetSubsystem<UAlchemyGameplaySubsystem>()->GetCommission(CommissionId,Out);}
void UForestAlchemy::ChooseCommission(int32 Id){if(!Commissions().ContainsByPredicate([Id](const auto& C){return C.OrderIndex==Id;}))return;CommissionId=Id;Sequence.Reset();SelectedSlot=INDEX_NONE;bConfirming=false;Status=TEXT("点击素材或大牌，按顺序放入调配槽。移除或退出不会扣除物品。");RefreshUI();}
void UForestAlchemy::AddChoice(const FForestBrewChoice& C)
{
 FAlchemyCommissionDefinition Comm;if(!CurrentCommission(Comm)){Status=TEXT("请先选择今晚的委托。");RefreshUI();return;}
 if(!Choices().ContainsByPredicate([&C](const auto& Available){return Available.Key==C.Key;})){Status=TEXT("物品信息已更新，请重新选择。");RefreshUI();return;}
 int32 Used=0;for(const auto& S:Sequence)if(S.Key==C.Key)++Used;int32 Cost=0;for(const auto& S:Sequence)Cost+=S.Card.Cost;
 if(Sequence.Num()>=Comm.SlotCount)Status=TEXT("调配槽已满，请移除或调整顺序。");
 else if(Cost+C.Card.Cost>5)Status=TEXT("今晚额度不足。元素素材免费，大牌和通用修饰计费。");
 else if(C.Key.StartsWith(TEXT("major:")) && Used>0)Status=TEXT("这一张大牌已经放入槽中。");
 else if(C.Key.StartsWith(TEXT("pip:")) && Used>=ReadBag().FindRef(C.Key.Mid(4)))Status=TEXT("这种元素小物的可用数量不足。");
 else {Sequence.Add(C);SelectedSlot=Sequence.Num()-1;bConfirming=false;Status=TEXT("已放入调配槽；还未扣除物品。");}RefreshUI();
}
void UForestAlchemy::RemoveSlot(){if(Sequence.IsValidIndex(SelectedSlot)){Sequence.RemoveAt(SelectedSlot);SelectedSlot=INDEX_NONE;bConfirming=false;}RefreshUI();}
void UForestAlchemy::MoveSlot(int32 Direction){int32 Other=SelectedSlot+Direction;if(Sequence.IsValidIndex(SelectedSlot) && Sequence.IsValidIndex(Other)){Sequence.Swap(SelectedSlot,Other);SelectedSlot=Other;bConfirming=false;}RefreshUI();}
void UForestAlchemy::ClearSequence(){Sequence.Reset();SelectedSlot=INDEX_NONE;bConfirming=false;RefreshUI();}
bool UForestAlchemy::Evaluate(FAlchemyEvaluationResult& Out) const{if(CommissionId<=0)return false;TArray<FAlchemyCardSequenceItem> Items;for(const auto& C:Sequence)Items.Add(C.Card);return GetWorld()->GetGameInstance()->GetSubsystem<UAlchemyGameplaySubsystem>()->EvaluateAlchemySequence(CommissionId,Items,Out);}
bool UForestAlchemy::IsWorkbenchOpen() const{return Workbench && Workbench->IsInViewport();}
void UForestAlchemy::OpenWorkbench()
{
 if(OtherMenuOpen() || IsWorkbenchOpen() || !Save)return;
 // Opening the new panel replaces the legacy cauldron UI rather than layering two panels.
 if(APawn* Pawn=Player->GetPawn()){if(auto* P=FindFProperty<FObjectPropertyBase>(Pawn->GetClass(),TEXT("CauldronPanel")))if(auto* W=Cast<UUserWidget>(P->GetObjectPropertyValue_InContainer(Pawn)))W->RemoveFromParent();if(auto* P=FindFProperty<FBoolProperty>(Pawn->GetClass(),TEXT("CauldronOpen")))P->SetPropertyValue_InContainer(Pawn,false);}
 if(APawn* Pawn=Player->GetPawn())
 {
  auto* P=FindFProperty<FMapProperty>(Pawn->GetClass(),TEXT("CauldronContents"));auto* K=P?CastField<FStrProperty>(P->KeyProp):nullptr;auto* V=P?CastField<FIntProperty>(P->ValueProp):nullptr;
  if(K && V){FScriptMapHelper OldPot(P,P->ContainerPtrToValuePtr<void>(Pawn));auto Bag=ReadBag();for(int32 I=0;I<OldPot.GetMaxIndex();++I)if(OldPot.IsValidIndex(I))Bag.FindOrAdd(K->GetPropertyValue(OldPot.GetKeyPtr(I)))+=FMath::Max(0,V->GetPropertyValue(OldPot.GetValuePtr(I)));if(WriteBag(Bag))OldPot.EmptyValues();}
 }
 if(!Workbench){Workbench=CreateWidget<UForestBrewWidget>(Player.Get());if(!Workbench)return;Workbench->Owner=this;}ClearSequence();CommissionId=0;Status=TEXT("请先选择今晚的委托。试算不扣料，封瓶交付需要再次确认。");bOldCursor=Player->bShowMouseCursor;Workbench->AddToViewport(110);Workbench->Refresh();FInputModeUIOnly Mode;Mode.SetWidgetToFocus(Workbench->TakeWidget());Player->SetInputMode(Mode);Player->bShowMouseCursor=true;
}
void UForestAlchemy::CloseWorkbench(){if(Workbench)Workbench->RemoveFromParent();Sequence.Reset();SelectedSlot=INDEX_NONE;bConfirming=false;if(Player.IsValid()){Player->SetInputMode(FInputModeGameOnly());Player->bShowMouseCursor=bOldCursor;}}
void UForestAlchemy::RefreshUI(){if(IsWorkbenchOpen())Workbench->Refresh();}
void UForestAlchemy::ConfirmBrew()
{
 if(!IsWorkbenchOpen())return;
 FAlchemyEvaluationResult Result;if(Sequence.IsEmpty() || !Evaluate(Result) || !Result.bValid){Status=TEXT("请先选择委托并放入材料。");RefreshUI();return;}
 FAlchemyCommissionDefinition Comm;if(!CurrentCommission(Comm) || Sequence.Num()>Comm.SlotCount || Result.CostUsed>5){Status=TEXT("槽位或额度超出限制，请重新调配。");bConfirming=false;RefreshUI();return;}
 if(!bConfirming){bConfirming=true;Status=TEXT("确认将消耗槽内的元素小物及大牌，记录本次成品，并进入下一天。再次点击「确认封瓶并换天」继续。");RefreshUI();return;}
 const auto BeforeBag=ReadBag();auto AfterBag=BeforeBag;const auto BeforeCards=Save->Majors;
 for(const auto& C:Sequence)
 {
  if(C.Key.StartsWith(TEXT("pip:"))){const FString K=C.Key.Mid(4);if(AfterBag.FindRef(K)<=0){Status=TEXT("背包数量已变化，请重新调配。");bConfirming=false;RefreshUI();return;}--AfterBag.FindOrAdd(K);}
  if(C.Key.StartsWith(TEXT("major:")) && !Save->Majors.ContainsByPredicate([&C](const auto& M){return M.InstanceId==C.Key.Mid(6);})){Status=TEXT("今日大牌已变化，请重新调配。");bConfirming=false;RefreshUI();return;}
 }
 if(!WriteBag(AfterBag)){Status=TEXT("背包不可写，未完成合成。");bConfirming=false;RefreshUI();return;}
 for(const auto& C:Sequence)if(C.Key.StartsWith(TEXT("major:")))Save->Majors.RemoveAll([&C](const auto& M){return M.InstanceId==C.Key.Mid(6);});
 FForestBrewRecord R;R.Day=GetDay();R.CommissionId=CommissionId;R.Result=Result;for(const auto& C:Sequence)R.Sequence.Add(C.Card);Save->Brews.Add(R);Save->PendingAdvanceFromDay=GetDay();
 if(!Persist() || !GetWorld()->GetGameInstance()->GetSubsystem<UDailyMushroomSubsystem>()->ConfirmDryPotSynthesis())
 {
  WriteBag(BeforeBag);Save->Majors=BeforeCards;Save->Brews.Pop();Save->PendingAdvanceFromDay=0;Persist();Status=TEXT("保存或换天失败，已撤回本次扣料，请重试。");bConfirming=false;RefreshUI();return;
 }
 Save->PendingAdvanceFromDay=0;CloseWorkbench();SyncDay();Tell(TEXT("成品已记录。新的一天将从小屋开始，药瓶与今日大牌已经更新。"));
}
TStatId UForestAlchemy::GetStatId() const{RETURN_QUICK_DECLARE_CYCLE_STAT(UForestAlchemy,STATGROUP_Tickables);}
void UForestAlchemy::Deinitialize(){CloseWorkbench();if(Player.IsValid() && Input)Player->PopInputComponent(Input);if(Notice)Notice->RemoveFromParent();Super::Deinitialize();}
