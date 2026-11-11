#include "ForestElementCodex.h"
#include "ForestUITheme.h"
#include "ForestHarvestMagic.h"
#include "ForestAlchemy.h"
#include "ForestAlchemyGuide.h"
#include "ForestMajorArt.h"
#include "AlchemyGameplaySubsystem.h"
#include "Engine/GameInstance.h"
#include "ForestNPCInteraction.h"
#include "ForestIsometricPlayer.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "InputCoreTypes.h"

namespace ElementBook
{
 const TCHAR* Keys[]={TEXT("fire"),TEXT("wind"),TEXT("water")};
 const TCHAR* Names[]={TEXT("火"),TEXT("风"),TEXT("水")};
 const FLinearColor Cover(.16f,.105f,.065f),Spine(.095f,.055f,.025f);
 const FSlateBrush* PagePaper()
 {
  // Reuse the paper's texture, cropping its loose-sheet silhouette into bound pages.
  static const FSlateBrush Brush=[](){FSlateBrush B=*ForestTheme::PaperSheet();B.SetUVRegion(FBox2f(FVector2f(.065f,.085f),FVector2f(.935f,.915f)));return B;}();
  return &Brush;
 }
 TSharedRef<STextBlock> Text(const FString& S,int32 Size=18,bool Bold=false)
 {return SNew(STextBlock).Text(FText::FromString(S)).Font(ForestTheme::Font(Size,Bold)).ColorAndOpacity(ForestTheme::Ink).AutoWrapText(true);}
 TSharedRef<SWidget> Button(const FString& S,FOnClicked Click,bool Selected=false)
 {return SNew(SButton).ButtonStyle(&ForestTheme::SecondaryButton()).ContentPadding(FMargin(12,7)).ButtonColorAndOpacity(Selected?ForestTheme::SelectedPaper:FLinearColor::White).OnClicked(Click)[Text(S,16)];}
}
UForestElementCodex::UForestElementCodex()
{
 static ConstructorHelpers::FObjectFinder<USkeletalMesh> Mesh(TEXT("/Game/MimicBookCharacter/Character/Mesh/SK_Mimic-book.SK_Mimic-book"));BookMesh=Mesh.Object;
 static ConstructorHelpers::FObjectFinder<UAnimSequence> Open(TEXT("/Game/MimicBookCharacter/Animations/Mimic-book_Open_Anim.Mimic-book_Open_Anim"));OpenAnimation=Open.Object;
 static ConstructorHelpers::FObjectFinder<UAnimSequence> Fold(TEXT("/Game/MimicBookCharacter/Animations/Mimic-book_Fold_Anim.Mimic-book_Fold_Anim"));FoldAnimation=Fold.Object;
}
void UForestElementCodex::LoadEntries()
{
 Entries.Reset();LoadError.Reset();FString Data;TSharedPtr<FJsonObject> Root;
 if(!FFileHelper::LoadFileToString(Data,*(FPaths::ProjectConfigDir()/TEXT("ForestElementCodex.json"))) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Data),Root) || !Root.IsValid())
 {LoadError=TEXT("书页暂时无法读取。");return;}
 const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
 if(!Root->TryGetStringField(TEXT("introduction"),Introduction) || !Root->TryGetStringField(TEXT("notes"),Notes) || !Root->TryGetArrayField(TEXT("entries"),Rows))
 {LoadError=TEXT("书页内容不完整。");return;}
 TSet<FString> Seen;
 for(const auto& Value:*Rows)
 {
  const TSharedPtr<FJsonObject>* Row=nullptr;FForestCodexEntry E;double Strength=0;
  if(!Value.IsValid() || !Value->TryGetObject(Row) || !Row || !Row->IsValid() || !(*Row)->TryGetStringField(TEXT("element"),E.Element) ||
   !(*Row)->TryGetNumberField(TEXT("strength"),Strength) || Strength<1 || Strength>10 || Strength!=FMath::FloorToDouble(Strength) ||
   !(*Row)->TryGetStringField(TEXT("title"),E.Title) || E.Title.TrimStartAndEnd().IsEmpty() ||
   !(*Row)->TryGetStringField(TEXT("description"),E.Description) || E.Description.TrimStartAndEnd().IsEmpty() ||
   !(*Row)->TryGetStringField(TEXT("reference"),E.Reference) || E.Reference.TrimStartAndEnd().IsEmpty())
  {LoadError=TEXT("书页中有无法辨认的条目。");break;}
  E.Strength=static_cast<int32>(Strength);
  if(E.Element!=TEXT("fire") && E.Element!=TEXT("wind") && E.Element!=TEXT("water")){LoadError=TEXT("书页元素分类有误。");break;}
  const FString Key=E.Element+FString::FromInt(E.Strength);if(Seen.Contains(Key)){LoadError=TEXT("书页条目重复。");break;}
  Seen.Add(Key);Entries.Add(E);
 }
 if(Entries.Num()!=30 || !LoadError.IsEmpty()){Entries.Reset();if(LoadError.IsEmpty())LoadError=TEXT("部分书页遗失，暂时无法查阅。");}
}
void UForestElementCodex::Setup()
{
 auto* PC=UGameplayStatics::GetPlayerController(this,0);if(!PC || !PC->GetPawn())return;Player=PC;
 LoadEntries();Input=NewObject<UInputComponent>(PC);Input->RegisterComponent();Input->Priority=210;
 Input->BindKey(EKeys::J,IE_Pressed,this,&UForestElementCodex::Toggle);
 Input->BindKey(EKeys::K,IE_Pressed,this,&UForestElementCodex::ToggleTaskJournal);PC->PushInputComponent(Input);bReady=true;
}
bool UForestElementCodex::IsOpen() const{return Widget && Widget->IsInViewport();}
bool UForestElementCodex::IsTaskJournalOpen() const{return JournalWidget && JournalWidget->IsInViewport();}
bool UForestElementCodex::OtherMenuOpen() const
{
 if(IsTaskJournalOpen())return true;
 if(auto* H=GetWorld()->GetSubsystem<UForestHarvestMagic>();H && H->IsInventoryOpen())return true;
 if(auto* A=GetWorld()->GetSubsystem<UForestAlchemy>();A && (A->IsCampaignCompleted() || (A->IsWorkbenchOpen() && !A->IsWorkbenchGuideSuspended())))return true;
 if(auto* M=Cast<AForestNPCInteraction>(UGameplayStatics::GetActorOfClass(this,AForestNPCInteraction::StaticClass()));M && (M->IsDialogueOpen() || M->IsFinderMenuOpen()))return true;
 return false;
}
void UForestElementCodex::Open()
{
 if(!bReady)Setup();auto* PC=Player.Get();if(!bReady || !PC || IsOpen() || OtherMenuOpen())return;
 if(APawn* Pawn=PC->GetPawn())if(auto* View=Pawn->FindComponentByClass<UForestIsometricPlayer>();View && (View->bGathering || View->bClimbing))return;
 if(!Widget){Widget=CreateWidget<UForestElementCodexWidget>(PC);if(!Widget)return;Widget->Owner=this;}
 auto* Alchemy=GetWorld()->GetSubsystem<UForestAlchemy>();bReturnToWorkbench=Alchemy && Alchemy->IsWorkbenchGuideSuspended();
 bOldCursor=PC->bShowMouseCursor;PC->StopMovement();Widget->AddToViewport(125);Widget->Refresh();
 OpeningTime=OpenAnimation && BookMesh ? .55f : 0.f;Widget->SetRenderOpacity(OpeningTime>0?0.f:1.f);
 FInputModeUIOnly Mode;Mode.SetWidgetToFocus(Widget->TakeWidget());PC->SetInputMode(Mode);PC->bShowMouseCursor=true;
 if(auto* H=GetWorld()->GetSubsystem<UForestHarvestMagic>())H->DismissNotice();PlayBook(true);
}
void UForestElementCodex::Toggle(){if(IsOpen())Close();else Open();}
void UForestElementCodex::Close()
{
 if(!IsOpen())return;Widget->RemoveFromParent();OpeningTime=0;PlayBook(false);
 if(bReturnToWorkbench)
 {
  bReturnToWorkbench=false;
  if(auto* A=GetWorld()->GetSubsystem<UForestAlchemy>();A && A->IsWorkbenchGuideSuspended()){A->ResumeWorkbenchGuide();return;}
 }
 if(auto* PC=Player.Get())
 {
  PC->bShowMouseCursor=bOldCursor;
  if(APawn* Pawn=PC->GetPawn())if(auto* View=Pawn->FindComponentByClass<UForestIsometricPlayer>()){View->RefreshGameplayInput();return;}
  if(bOldCursor){FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);PC->SetInputMode(Mode);}else PC->SetInputMode(FInputModeGameOnly());
 }
}
void UForestElementCodex::OpenTaskJournal()
{
 if(!bReady)Setup();auto* PC=Player.Get();if(!bReady || !PC || IsOpen() || OtherMenuOpen())return;
 if(!JournalWidget){JournalWidget=CreateWidget<UForestTaskJournalWidget>(PC);if(!JournalWidget)return;JournalWidget->Owner=this;}
 bJournalOldCursor=PC->bShowMouseCursor;PC->StopMovement();JournalWidget->AddToViewport(126);
 FInputModeUIOnly Mode;Mode.SetWidgetToFocus(JournalWidget->TakeWidget());PC->SetInputMode(Mode);PC->bShowMouseCursor=true;
}
void UForestElementCodex::CloseTaskJournal()
{
 if(!IsTaskJournalOpen())return;JournalWidget->RemoveFromParent();
 if(auto* PC=Player.Get())
 {
  PC->bShowMouseCursor=bJournalOldCursor;
  if(APawn* Pawn=PC->GetPawn())if(auto* View=Pawn->FindComponentByClass<UForestIsometricPlayer>()){View->RefreshGameplayInput();return;}
  if(bJournalOldCursor){FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);PC->SetInputMode(Mode);}else PC->SetInputMode(FInputModeGameOnly());
 }
}
void UForestElementCodex::ToggleTaskJournal(){if(IsTaskJournalOpen())CloseTaskJournal();else OpenTaskJournal();}
void UForestElementCodex::Select(int32 E,int32 Strength)
{bDailyMajors=false;SelectedElement=FMath::Clamp(E,0,2);SelectedStrength=FMath::Clamp(Strength,1,10);if(IsOpen())Widget->Refresh();}
void UForestElementCodex::TurnPage(int32 Direction)
{
 if(bDailyMajors){const auto Cards=DailyCards();if(Cards.IsEmpty())return;int32 I=Cards.IndexOfByPredicate([this](const auto& D){return D.Id==SelectedMajor;});SelectMajor(Cards[FMath::Clamp((I==INDEX_NONE?0:I)+Direction,0,Cards.Num()-1)].Id);return;}
 const int32 Index=FMath::Clamp(SelectedElement*10+SelectedStrength-1+Direction,0,29);Select(Index/10,Index%10+1);
}
void UForestElementCodex::OpenTodayMajors(){bDailyMajors=true;Open();if(IsOpen())Widget->Refresh();}
void UForestElementCodex::ShowDailyMajors(bool bDaily){bDailyMajors=bDaily;if(IsOpen())Widget->Refresh();}
void UForestElementCodex::SelectMajor(FName Id){SelectedMajor=Id;bDailyMajors=true;if(IsOpen())Widget->Refresh();}
TArray<FAlchemyCardDefinition> UForestElementCodex::DailyCards() const
{
 TArray<FAlchemyCardDefinition> Out;auto* A=GetWorld()->GetSubsystem<UForestAlchemy>();auto* GI=GetWorld()->GetGameInstance();auto* G=GI?GI->GetSubsystem<UAlchemyGameplaySubsystem>():nullptr;
 if(A && G)for(const auto& D:G->GetMajorCards())Out.Add(D);return Out;
}
int32 UForestElementCodex::DailyCount(FName Id) const
{int32 Count=0;if(auto* A=GetWorld()->GetSubsystem<UForestAlchemy>())for(const auto& C:A->GetTodayMajors())if(C.CardId==Id)++Count;return Count;}
const FForestCodexEntry* UForestElementCodex::CurrentEntry() const
{return Entries.FindByPredicate([this](const auto& E){return E.Element==ElementBook::Keys[SelectedElement] && E.Strength==SelectedStrength;});}
FString UForestElementCodex::ReferenceFor(int32 Element,int32 Strength) const
{if(Element<0 || Element>2)return TEXT("");const auto* E=Entries.FindByPredicate([=](const auto& R){return R.Element==ElementBook::Keys[Element] && R.Strength==Strength;});return E?E->Description+TEXT("\n")+E->Reference:TEXT("");}
void UForestElementCodex::PlayBook(bool bOpening)
{
 if(auto* A=BookVisual.Get())A->Destroy();BookVisual.Reset();auto* PC=Player.Get();if(!PC || !BookMesh)return;
 FVector Eye;FRotator Facing;PC->GetPlayerViewPoint(Eye,Facing);FActorSpawnParameters Params;Params.ObjectFlags|=RF_Transient;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
 auto* A=GetWorld()->SpawnActor<AActor>(AActor::StaticClass(),Eye+Facing.Vector()*130-FVector(0,0,35),FRotator(0,Facing.Yaw+180,0),Params);if(!A)return;
 auto* Mesh=NewObject<USkeletalMeshComponent>(A);A->SetRootComponent(Mesh);Mesh->SetSkeletalMeshAsset(BookMesh);Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mesh->SetCastShadow(false);Mesh->RegisterComponent();A->SetActorLocationAndRotation(Eye+Facing.Vector()*130-FVector(0,0,35),FRotator(0,Facing.Yaw+180,0));Mesh->SetWorldScale3D(FVector(.35f));
 if(auto* Anim=bOpening?OpenAnimation.Get():FoldAnimation.Get())Mesh->PlayAnimation(Anim,false);
 A->SetLifeSpan(bOpening?1.2f:.6f);A->Tags.Add(TEXT("Forest.CodexVisual"));BookVisual=A;
}
void UForestElementCodex::Tick(float DT)
{
 auto* World=GetWorld();if(!World || !World->IsGameWorld() || World->GetNetMode()==NM_DedicatedServer)return;
 if(!bReady){Setup();return;}if(IsOpen() && OpeningTime>0){OpeningTime=FMath::Max(0.f,OpeningTime-DT);Widget->SetRenderOpacity(FMath::Clamp(1-OpeningTime/.55f,0.f,1.f));}
}
TStatId UForestElementCodex::GetStatId() const{RETURN_QUICK_DECLARE_CYCLE_STAT(UForestElementCodex,STATGROUP_Tickables);}
void UForestElementCodex::Deinitialize()
{if(Widget)Widget->RemoveFromParent();if(JournalWidget)JournalWidget->RemoveFromParent();if(auto* A=BookVisual.Get())A->Destroy();if(auto* PC=Player.Get())if(Input)PC->PopInputComponent(Input);Super::Deinitialize();}

TSharedRef<SWidget> UForestTaskJournalWidget::RebuildWidget()
{
 SetIsFocusable(true);
 const auto* A=GetWorld()->GetSubsystem<UForestAlchemy>();
 return SNew(SOverlay)+SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
 [SNew(SBox).WidthOverride(700).HeightOverride(610)
  [SNew(SBorder).BorderImage(ElementBook::PagePaper()).Padding(FMargin(42,36))
   [SNew(SVerticalBox)
    +SVerticalBox::Slot().AutoHeight()[ElementBook::Text(TEXT("委托手札"),28,true)]
    +SVerticalBox::Slot().AutoHeight().Padding(0,8,0,18)[ElementBook::Text(TEXT("与森林里的人谈妥的委托记在这里。晚上回小屋，在工作台交付药剂。"),16)]
    +SVerticalBox::Slot().FillHeight(1)[SNew(SScrollBox)+SScrollBox::Slot()[ElementBook::Text(A?A->TaskJournal():TEXT("正在读取委托记录。"),19)]]
    +SVerticalBox::Slot().AutoHeight().Padding(0,18,0,0)[ElementBook::Button(TEXT("合上手札 · K / Esc"),FOnClicked::CreateLambda([this](){if(Owner.IsValid())Owner->CloseTaskJournal();return FReply::Handled();}))]]]];
}
FReply UForestTaskJournalWidget::NativeOnPreviewKeyDown(const FGeometry& G,const FKeyEvent& E)
{if((E.GetKey()==EKeys::K || E.GetKey()==EKeys::Escape) && Owner.IsValid()){Owner->CloseTaskJournal();return FReply::Handled();}return Super::NativeOnPreviewKeyDown(G,E);}

TSharedRef<SWidget> UForestElementCodexWidget::RebuildWidget()
{
 SetIsFocusable(true);
 return SNew(SOverlay)+SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
 [SAssignNew(Frame,SBox).WidthOverride(1160).HeightOverride(760)
  [SNew(SBorder).BorderImage(ForestTheme::Fill()).BorderBackgroundColor(ElementBook::Cover).Padding(FMargin(10,9,10,13))
   [SNew(SBorder).BorderImage(ForestTheme::Fill()).BorderBackgroundColor(ForestTheme::Edge).Padding(FMargin(3,2,3,6))
    [SAssignNew(Body,SVerticalBox)]]]];
}
void UForestElementCodexWidget::NativeTick(const FGeometry& G,float DT)
{
 Super::NativeTick(G,DT);if(!Frame)return;const auto Size=G.GetLocalSize();if(Size.X<=0 || Size.Y<=0)return;
 const float W=FMath::Max(1.f,FMath::Min(1160.f,static_cast<float>(Size.X)-24.f));
 Frame->SetWidthOverride(W);Frame->SetHeightOverride(FMath::Max(1.f,FMath::Min(760.f,static_cast<float>(Size.Y)-24.f)));
 const bool Single=W<860;if(Single!=bSinglePage){bSinglePage=Single;Refresh();}
}
void UForestElementCodexWidget::Refresh()
{
 if(!Body || !Owner.IsValid())return;Body->ClearChildren();auto* A=Owner.Get();
 const auto Cards=A->DailyCards();
 if(A->bDailyMajors && !Cards.ContainsByPredicate([A](const auto& C){return C.Id==A->SelectedMajor;}))A->SelectedMajor=Cards.IsEmpty()?NAME_None:Cards[0].Id;
 const FString Intro=A->bDailyMajors?ForestAlchemyGuide::Introduction():A->Introduction;
 const FString Notes=A->bDailyMajors?ForestAlchemyGuide::Notes():A->Notes;
 auto MakeMajorTile=[this,A](const FAlchemyCardDefinition& D)->TSharedRef<SWidget>
 {
  const auto* Alchemy=GetWorld()->GetSubsystem<UForestAlchemy>();const bool Unlocked=Alchemy && Alchemy->HasDiscoveredMajor(D.Id);
  return SNew(SButton).ButtonStyle(&ForestTheme::SecondaryButton()).ContentPadding(FMargin(5,6))
   .ButtonColorAndOpacity(A->SelectedMajor==D.Id?ForestTheme::SelectedPaper:FLinearColor::White)
   .OnClicked_Lambda([this,Id=D.Id](){if(Owner.IsValid())Owner->SelectMajor(Id);return FReply::Handled();})
   [SNew(SVerticalBox)
    +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
     [SNew(SBox).WidthOverride(39).HeightOverride(140)
      [SNew(SOverlay)+SOverlay::Slot()[SNew(SImage).Image(ForestMajorArt::BrushFor(D.Id))]
       +SOverlay::Slot()[SNew(SBorder).BorderImage(ForestTheme::Fill()).BorderBackgroundColor(Unlocked?FLinearColor(0,0,0,0):FLinearColor(0,0,0,.83f))]]]
    +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0,4,0,0)
     [ElementBook::Text(Unlocked?D.Id.ToString():TEXT("?"),13,true)]];
 };
 auto Detail=SNew(SVerticalBox);
 if(A->bDailyMajors)
 {
  auto* Alchemy=GetWorld()->GetSubsystem<UForestAlchemy>();
  Detail->AddSlot().AutoHeight().Padding(0,12)[ElementBook::Text(FString::Printf(TEXT("日运牌图鉴 · 已遇见 %d / 22"),([&](){int32 N=0;for(const auto& C:Cards)if(Alchemy && Alchemy->HasDiscoveredMajor(C.Id))++N;return N;})()),24,true)];
  if(bSinglePage)
  {
   Detail->AddSlot().AutoHeight().Padding(0,10)[ElementBook::Text(Intro,18)];
   auto List=SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(5,5));
   for(const auto& D:Cards)List->AddSlot()[MakeMajorTile(D)];
   Detail->AddSlot().AutoHeight().Padding(0,10)[List];
  }
  if(const auto* D=Cards.FindByPredicate([A](const auto& C){return C.Id==A->SelectedMajor;}))
  {
   const bool bUnlocked=Alchemy && Alchemy->HasDiscoveredMajor(D->Id);
   Detail->AddSlot().AutoHeight().Padding(0,12)[ElementBook::Text(bUnlocked?D->Id.ToString()+TEXT(" · ")+D->Name:TEXT("尚未遇见的日运牌"),25,true)];
   Detail->AddSlot().AutoHeight().Padding(0,0,0,18)[ElementBook::Text(bUnlocked?FString::Printf(TEXT("曾经遇见 · 今日持有 %d 张 · 每张占1槽、花%d点额度"),A->DailyCount(D->Id),D->Cost):TEXT("尚未遇见 · 抽到后解锁牌面与说明"),18)];
   // The complete printed card remains visible while the explanation is read.
   // The source art is 142×512; preserve that tall proportion on the page.
   if(const FSlateBrush* Art=ForestMajorArt::BrushFor(D->Id))
    Detail->AddSlot().AutoHeight().Padding(0,0,0,18)
     [SNew(SBox).WidthOverride(112).HeightOverride(404).HAlign(HAlign_Left)[SNew(SOverlay)+SOverlay::Slot()[SNew(SImage).Image(Art)]+SOverlay::Slot()[SNew(SBorder).BorderImage(ForestTheme::Fill()).BorderBackgroundColor(bUnlocked?FLinearColor(0,0,0,0):FLinearColor(0,0,0,.82f))]]];
   FForestMajorGuide Guide;auto* GI=GetWorld()->GetGameInstance();auto* G=GI?GI->GetSubsystem<UAlchemyGameplaySubsystem>():nullptr;
   if(!bUnlocked)Detail->AddSlot().AutoHeight().Padding(0,8)[ElementBook::Text(TEXT("这张牌还没有从药瓶中出现过。"),19)];
   else if(ForestAlchemyGuide::Major(G,D->Id,Guide))
   {
    const FString Titles[]={TEXT("怎么使用"),TEXT("试一次就明白"),TEXT("前面接「恶魔」时"),TEXT("放置提醒")};
    const FString Texts[]={Guide.HowToUse,Guide.Example,Guide.Reverse,Guide.Caution};
    for(int32 I=0;I<4;++I){Detail->AddSlot().AutoHeight().Padding(0,14,0,8)[ElementBook::Text(Titles[I],20,true)];Detail->AddSlot().AutoHeight().Padding(0,0,0,16)[ElementBook::Text(Texts[I],19)];}
   }
   else Detail->AddSlot().AutoHeight()[ElementBook::Text(TEXT("详细书页暂时无法读取。原有运算说明：\n")+D->Description,19)];
  }
  else
  {
   const int32 Drawn=Alchemy?Alchemy->GetTodayDrawCount():0,Limit=Alchemy?Alchemy->GetMaxDailyDraws():3;
   const FString Empty=Alchemy && Alchemy->IsCampaignCompleted()?TEXT("三日旅程已经结束，日运牌随结局退去。"):
    Limit>0 && Drawn>=Limit?TEXT("今天的日运牌已经用在药里，抽取次数不会补回。"):
    FString::Printf(TEXT("工作台药瓶能抽出今天的日运牌。夜里合成时，它可以改变火、风、水的效果。靠近药瓶按 E 抽取；今天已抽 %d/%d 张。"),Drawn,Limit);
   Detail->AddSlot().AutoHeight().Padding(0,18)[ElementBook::Text(Empty,20)];
  }
 }
 else
 {
  auto Tabs=SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6,4));
  for(int32 E=0;E<3;++E)Tabs->AddSlot()[ElementBook::Button(FString(ElementBook::Names[E])+TEXT("之卷"),FOnClicked::CreateLambda([this,E](){if(Owner.IsValid())Owner->Select(E,1);return FReply::Handled();}),A->SelectedElement==E)];
  Detail->AddSlot().AutoHeight().Padding(0,12)[ElementBook::Text(TEXT("元素手札"),24,true)];
  Detail->AddSlot().AutoHeight().Padding(0,10,0,12)[Tabs];
  if(const auto* E=A->CurrentEntry())
  {
   Detail->AddSlot().AutoHeight().Padding(0,8)[SNew(SBox).WidthOverride(66).HeightOverride(66).HAlign(HAlign_Left)[SNew(SImage).Image(ForestTheme::ElementIcon(A->SelectedElement))]];
   Detail->AddSlot().AutoHeight().Padding(0,12)[ElementBook::Text(E->Title,25,true)];
   Detail->AddSlot().AutoHeight().Padding(0,0,0,22)[ElementBook::Text(FString::Printf(TEXT("%s · 效果档位 %d"),ElementBook::Names[A->SelectedElement],E->Strength),16)];
   Detail->AddSlot().AutoHeight().Padding(0,0,0,20)[ElementBook::Text(E->Description,20)];
   Detail->AddSlot().AutoHeight().Padding(0,0,0,26)[ElementBook::Text(E->Reference,18)];
  }
  else Detail->AddSlot().AutoHeight().Padding(0,18)[ElementBook::Text(A->LoadError.IsEmpty()?TEXT("这一页尚未记下内容。"):A->LoadError,20)];
 }
 if(bSinglePage)
 {
  if(!A->bDailyMajors)Detail->AddSlot().AutoHeight().Padding(0,16)[ElementBook::Text(Intro,16)];
  Detail->AddSlot().AutoHeight().Padding(0,14)[ElementBook::Text(Notes,16)];
 }
 auto Right=SNew(SVerticalBox);
 auto Sections=SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6,4));
 Sections->AddSlot()[ElementBook::Button(TEXT("元素效果"),FOnClicked::CreateLambda([this](){if(Owner.IsValid())Owner->ShowDailyMajors(false);return FReply::Handled();}),!A->bDailyMajors)];
 Sections->AddSlot()[ElementBook::Button(TEXT("日运牌图鉴"),FOnClicked::CreateLambda([this](){if(Owner.IsValid())Owner->ShowDailyMajors(true);return FReply::Handled();}),A->bDailyMajors)];
 Right->AddSlot().AutoHeight().Padding(0,0,0,8)[Sections];
 Right->AddSlot().FillHeight(1)[SNew(SScrollBox).ScrollBarAlwaysVisible(true)+SScrollBox::Slot()[Detail]];
 auto PageNavigation=SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(5,5));
 PageNavigation->AddSlot()[ElementBook::Button(TEXT("← 前页"),FOnClicked::CreateLambda([this](){if(Owner.IsValid())Owner->TurnPage(-1);return FReply::Handled();}))];
 PageNavigation->AddSlot()[ElementBook::Button(TEXT("后页 →"),FOnClicked::CreateLambda([this](){if(Owner.IsValid())Owner->TurnPage(1);return FReply::Handled();}))];
 PageNavigation->AddSlot()[ElementBook::Button(GetWorld()->GetSubsystem<UForestAlchemy>()->IsWorkbenchGuideSuspended()?TEXT("回到调配 · J / Esc"):TEXT("合上书 · J / Esc"),FOnClicked::CreateLambda([this](){if(Owner.IsValid())Owner->Close();return FReply::Handled();}))];
 Right->AddSlot().AutoHeight().Padding(0,12)[PageNavigation];
 const int32 MajorPage=Cards.IndexOfByPredicate([A](const auto& C){return C.Id==A->SelectedMajor;})+1;
 const FString Page=A->bDailyMajors?FString::Printf(TEXT("— %d / %d —"),MajorPage,Cards.Num()):FString::Printf(TEXT("— %d —"),A->SelectedElement*10+A->SelectedStrength);
 Right->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0,10,0,0)[ElementBook::Text(Page,16)];
 auto Spread=SNew(SHorizontalBox);
 if(!bSinglePage)
 {
  auto Index=SNew(SVerticalBox);Index->AddSlot().AutoHeight().Padding(0,0,0,18)[ElementBook::Text(TEXT("查阅目录"),24,true)];
  Index->AddSlot().AutoHeight().Padding(0,0,0,16)[ElementBook::Text(Intro,16)];
  if(A->bDailyMajors)
  {auto Shelf=SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(5,5));for(const auto& D:Cards)Shelf->AddSlot()[MakeMajorTile(D)];Index->AddSlot().AutoHeight()[Shelf];}
  if(!A->bDailyMajors)for(const auto& E:A->Entries)if(E.Element==ElementBook::Keys[A->SelectedElement])
  {const int32 Strength=E.Strength;Index->AddSlot().AutoHeight().Padding(0,3)[ElementBook::Button(FString::Printf(TEXT("%d   %s"),Strength,*E.Title),FOnClicked::CreateLambda([this,Strength](){if(Owner.IsValid())Owner->Select(Owner->SelectedElement,Strength);return FReply::Handled();}),Strength==A->SelectedStrength)];}
  Index->AddSlot().AutoHeight().Padding(0,22,0,0)[ElementBook::Text(Notes,16)];
  Spread->AddSlot().FillWidth(.43f)[SNew(SBorder).BorderImage(ElementBook::PagePaper()).Padding(FMargin(34,32,28,24))[SNew(SScrollBox)+SScrollBox::Slot()[Index]]];
  // A dark fold and lighter inner edges give the two paper surfaces a visible binding.
  auto Fold=SNew(SHorizontalBox);
  const FLinearColor Shades[]={FLinearColor(.53f,.43f,.29f),FLinearColor(.32f,.23f,.14f),ElementBook::Spine,FLinearColor(.32f,.23f,.14f),FLinearColor(.53f,.43f,.29f)};
  for(const auto& Shade:Shades)Fold->AddSlot().AutoWidth()[SNew(SBox).WidthOverride(4)[SNew(SBorder).BorderImage(ForestTheme::Fill()).BorderBackgroundColor(Shade)]];
  Spread->AddSlot().AutoWidth()[Fold];
 }
 Spread->AddSlot().FillWidth(.57f)[SNew(SBorder).BorderImage(ElementBook::PagePaper()).Padding(bSinglePage?FMargin(22,28,22,22):FMargin(30,32,34,24))[Right]];
 Body->AddSlot().FillHeight(1)[Spread];
}
FReply UForestElementCodexWidget::NativeOnPreviewKeyDown(const FGeometry& G,const FKeyEvent& E)
{
 if(Owner.IsValid())
 {
  if(E.GetKey()==EKeys::J || E.GetKey()==EKeys::Escape){Owner->Close();return FReply::Handled();}
  if(E.GetKey()==EKeys::Left){Owner->TurnPage(-1);return FReply::Handled();}
  if(E.GetKey()==EKeys::Right){Owner->TurnPage(1);return FReply::Handled();}
 }
 return Super::NativeOnPreviewKeyDown(G,E);
}
