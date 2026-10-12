#include "ForestNPCInteraction.h"
#include "ForestNPCDialogueSubsystem.h"
#include "ForestRoadAIController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/InputComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "UObject/UnrealType.h"
#include "EngineUtils.h"
#include "InputCoreTypes.h"
TSharedRef<SWidget> UForestNPCInteractionWidget::RebuildWidget()
{
 SetIsFocusable(true);
 if(!WidgetTree)WidgetTree=NewObject<UWidgetTree>(this,TEXT("EncounterWidgetTree"));
 auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=Canvas;
 auto* Border=WidgetTree->ConstructWidget<UBorder>();Border->SetBrushColor(FLinearColor(0.02f,0.03f,0.025f,0.95f));Border->SetPadding(FMargin(20,14));
 auto* CanvasSlot=Canvas->AddChildToCanvas(Border);CanvasSlot->SetAnchors(FAnchors(0.5f,1.f));CanvasSlot->SetAlignment(FVector2D(0.5f,1.f));CanvasSlot->SetPosition(FVector2D(0,-30));CanvasSlot->SetAutoSize(true);
 auto* Width=WidgetTree->ConstructWidget<USizeBox>();Width->SetWidthOverride(600);Border->SetContent(Width);
 auto* Box=WidgetTree->ConstructWidget<UVerticalBox>();Width->SetContent(Box);
 Title=WidgetTree->ConstructWidget<UTextBlock>();Box->AddChildToVerticalBox(Title);
 Conversation=WidgetTree->ConstructWidget<UVerticalBox>();Box->AddChildToVerticalBox(Conversation);
 auto* Height=WidgetTree->ConstructWidget<USizeBox>();Height->SetHeightOverride(250);Conversation->AddChildToVerticalBox(Height);
 Scroll=WidgetTree->ConstructWidget<UScrollBox>();Height->SetContent(Scroll);
 Message=WidgetTree->ConstructWidget<UTextBlock>();Message->SetWrapTextAt(550);Scroll->AddChild(Message);
 Input=WidgetTree->ConstructWidget<UEditableTextBox>();Input->SetHintText(FText::FromString(TEXT("输入想说的话（最多 500 字）")));Conversation->AddChildToVerticalBox(Input);
 Input->OnTextCommitted.AddDynamic(this,&UForestNPCInteractionWidget::TextCommitted);
 auto* Buttons=WidgetTree->ConstructWidget<UHorizontalBox>();Conversation->AddChildToVerticalBox(Buttons);
 Send=WidgetTree->ConstructWidget<UButton>();auto* SendLabel=WidgetTree->ConstructWidget<UTextBlock>();SendLabel->SetText(FText::FromString(TEXT("发送 Enter")));Send->SetContent(SendLabel);Buttons->AddChildToHorizontalBox(Send);Send->OnClicked.AddDynamic(this,&UForestNPCInteractionWidget::SendClicked);
 Close=WidgetTree->ConstructWidget<UButton>();auto* CloseLabel=WidgetTree->ConstructWidget<UTextBlock>();CloseLabel->SetText(FText::FromString(TEXT("结束交谈 Esc")));Close->SetContent(CloseLabel);Buttons->AddChildToHorizontalBox(Close);Close->OnClicked.AddDynamic(this,&UForestNPCInteractionWidget::CloseClicked);
 FinderSelection=WidgetTree->ConstructWidget<UVerticalBox>();Box->AddChildToVerticalBox(FinderSelection);FinderSelection->SetVisibility(ESlateVisibility::Collapsed);
 auto AddFinderChoice=[this](const TCHAR* Label)
 {
  auto* Button=WidgetTree->ConstructWidget<UButton>();auto* Text=WidgetTree->ConstructWidget<UTextBlock>();Text->SetText(FText::FromString(Label));Button->SetContent(Text);FinderSelection->AddChildToVerticalBox(Button);return Button;
 };
 auto* NovelistChoice=AddFinderChoice(TEXT("寻找小说家（森林女巫）"));NovelistChoice->OnClicked.AddDynamic(this,&UForestNPCInteractionWidget::FindNovelistClicked);
 auto* BlacksmithChoice=AddFinderChoice(TEXT("寻找铁匠 Natta"));BlacksmithChoice->OnClicked.AddDynamic(this,&UForestNPCInteractionWidget::FindBlacksmithClicked);
 auto* MessengerChoice=AddFinderChoice(TEXT("寻找传信人 Fawnia"));MessengerChoice->OnClicked.AddDynamic(this,&UForestNPCInteractionWidget::FindMessengerClicked);
 auto* SelectionCancel=WidgetTree->ConstructWidget<UButton>();auto* SelectionCancelText=WidgetTree->ConstructWidget<UTextBlock>();SelectionCancelText->SetText(FText::FromString(TEXT("取消")));SelectionCancel->SetContent(SelectionCancelText);FinderSelection->AddChildToVerticalBox(SelectionCancel);SelectionCancel->OnClicked.AddDynamic(this,&UForestNPCInteractionWidget::FinderCancelClicked);
 FinderConfirmation=WidgetTree->ConstructWidget<UVerticalBox>();Box->AddChildToVerticalBox(FinderConfirmation);FinderConfirmation->SetVisibility(ESlateVisibility::Collapsed);
 FinderFee=WidgetTree->ConstructWidget<UTextBlock>();FinderFee->SetWrapTextAt(550);FinderConfirmation->AddChildToVerticalBox(FinderFee);
 auto* FinderButtons=WidgetTree->ConstructWidget<UHorizontalBox>();FinderConfirmation->AddChildToVerticalBox(FinderButtons);
 FinderConfirm=WidgetTree->ConstructWidget<UButton>();auto* ConfirmText=WidgetTree->ConstructWidget<UTextBlock>();ConfirmText->SetText(FText::FromString(TEXT("确认交付并寻找")));FinderConfirm->SetContent(ConfirmText);FinderButtons->AddChildToHorizontalBox(FinderConfirm);FinderConfirm->OnClicked.AddDynamic(this,&UForestNPCInteractionWidget::FinderConfirmClicked);
 auto* FinderCancel=WidgetTree->ConstructWidget<UButton>();auto* FinderCancelText=WidgetTree->ConstructWidget<UTextBlock>();FinderCancelText->SetText(FText::FromString(TEXT("返回")));FinderCancel->SetContent(FinderCancelText);FinderButtons->AddChildToHorizontalBox(FinderCancel);FinderCancel->OnClicked.AddDynamic(this,&UForestNPCInteractionWidget::FinderCancelClicked);
 Prompt=WidgetTree->ConstructWidget<UTextBlock>();Prompt->SetWrapTextAt(550);Box->AddChildToVerticalBox(Prompt);
 auto Font=Title->GetFont();Font.Size=22;Title->SetFont(Font);Font.Size=18;Message->SetFont(Font);Prompt->SetFont(Font);
 Prompt->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f,0.9f,0.65f)));return Super::RebuildWidget();
}
void UForestNPCInteractionWidget::ShowMessage(const FString& Name,const FString&,const FString& Hint)
{
 if(!Title)return;bFinderVisible=false;FinderSelection->SetVisibility(ESlateVisibility::Collapsed);FinderConfirmation->SetVisibility(ESlateVisibility::Collapsed);Title->SetText(FText::FromString(Name));Prompt->SetText(FText::FromString(Hint));Title->SetVisibility(Name.IsEmpty()?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);Conversation->SetVisibility(ESlateVisibility::Collapsed);SetVisibility(Hint.IsEmpty()?ESlateVisibility::Hidden:ESlateVisibility::HitTestInvisible);
}
void UForestNPCInteractionWidget::ShowConversation(const FString& Name,const FString& Body,const FString& Status,bool Waiting)
{
 if(!Title)return;bFinderVisible=false;FinderSelection->SetVisibility(ESlateVisibility::Collapsed);FinderConfirmation->SetVisibility(ESlateVisibility::Collapsed);SetVisibility(ESlateVisibility::Visible);Title->SetVisibility(ESlateVisibility::Visible);Conversation->SetVisibility(ESlateVisibility::Visible);
 Title->SetText(FText::FromString(Name));Message->SetText(FText::FromString(Body.IsEmpty()?TEXT("你叫住了对方。可以开始交谈。"):Body));Prompt->SetText(FText::FromString(Status));Input->SetIsEnabled(!Waiting);Send->SetIsEnabled(!Waiting);Scroll->ScrollToEnd();
}
void UForestNPCInteractionWidget::ShowFinderSelection()
{
 if(!Title)return;bFinderVisible=true;SetVisibility(ESlateVisibility::Visible);Title->SetVisibility(ESlateVisibility::Visible);Title->SetText(FText::FromString(TEXT("要寻找谁？")));Conversation->SetVisibility(ESlateVisibility::Collapsed);FinderConfirmation->SetVisibility(ESlateVisibility::Collapsed);FinderSelection->SetVisibility(ESlateVisibility::Visible);Prompt->SetText(FText::FromString(TEXT("选择一位 NPC。下一步会说明需要交付的物品。")));SetKeyboardFocus();
}
void UForestNPCInteractionWidget::ShowFinderConfirmation(const FString& CharacterName,const FString& FeeItem)
{
 if(!Title)return;bFinderVisible=true;SetVisibility(ESlateVisibility::Visible);Title->SetVisibility(ESlateVisibility::Visible);Title->SetText(FText::FromString(FString::Printf(TEXT("寻找 %s"),*CharacterName)));Conversation->SetVisibility(ESlateVisibility::Collapsed);FinderSelection->SetVisibility(ESlateVisibility::Collapsed);FinderConfirmation->SetVisibility(ESlateVisibility::Visible);
 const bool bCanPay=!FeeItem.IsEmpty();FinderFee->SetText(FText::FromString(bCanPay?FString::Printf(TEXT("需要交付背包中的 1 个物品。\n本次将交付：%s"),*FeeItem):TEXT("需要交付背包中的 1 个物品。\n背包为空，暂时无法寻找。")));FinderConfirm->SetIsEnabled(bCanPay);Prompt->SetText(FText::FromString(TEXT("确认后才会检查路线并扣除物品；取消不会扣除。")));SetKeyboardFocus();
}
void UForestNPCInteractionWidget::FocusInput(){if(Input && Input->GetIsEnabled())Input->SetKeyboardFocus();}
void UForestNPCInteractionWidget::SendClicked(){if(Manager.IsValid() && Input && Manager->SubmitDialogueText(Input->GetText().ToString()))Input->SetText(FText::GetEmpty());FocusInput();}
void UForestNPCInteractionWidget::CloseClicked(){if(Manager.IsValid())Manager->FinishInteraction();}
void UForestNPCInteractionWidget::FindNovelistClicked(){if(Manager.IsValid())Manager->SelectNpcForFinding(TEXT("forest_witch"));}
void UForestNPCInteractionWidget::FindBlacksmithClicked(){if(Manager.IsValid())Manager->SelectNpcForFinding(TEXT("natta"));}
void UForestNPCInteractionWidget::FindMessengerClicked(){if(Manager.IsValid())Manager->SelectNpcForFinding(TEXT("fawnia"));}
void UForestNPCInteractionWidget::FinderConfirmClicked(){if(Manager.IsValid())Manager->ConfirmNpcFinding();}
void UForestNPCInteractionWidget::FinderCancelClicked(){if(Manager.IsValid())Manager->CancelNpcFinder();}
void UForestNPCInteractionWidget::TextCommitted(const FText&,ETextCommit::Type Method){if(Method==ETextCommit::OnEnter)SendClicked();}
FReply UForestNPCInteractionWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{if(Event.GetKey()==EKeys::Escape){if(bFinderVisible)FinderCancelClicked();else CloseClicked();return FReply::Handled();}return Super::NativeOnPreviewKeyDown(Geometry,Event);}
AForestNPCInteraction::AForestNPCInteraction(){PrimaryActorTick.bCanEverTick=true;SetActorEnableCollision(false);InputPriority=100;}
FString AForestNPCInteraction::CharacterName(const ACharacter* C) const
{const FString Name=GetNameSafe(C);return Name.Contains(TEXT("Natta"))?TEXT("Natta"):Name.Contains(TEXT("Fawnia"))?TEXT("Fawnia"):TEXT("森林女巫");}
FName AForestNPCInteraction::PersonaId(const ACharacter* C) const
{const FString Name=GetNameSafe(C);return Name.Contains(TEXT("Natta"))?TEXT("natta"):Name.Contains(TEXT("Fawnia"))?TEXT("fawnia"):TEXT("forest_witch");}
bool AForestNPCInteraction::CanReach(ACharacter* C) const
{
 auto* PC=Player.Get();if(!PC || !PC->GetPawn() || !C || PC->GetViewTarget()!=PC->GetPawn())return false;
 if(FVector::Dist(PC->GetPawn()->GetActorLocation(),C->GetActorLocation())>InteractionRange)return false;
 FCollisionQueryParams Params(SCENE_QUERY_STAT(ForestEncounterVisibility),false);Params.AddIgnoredActor(PC->GetPawn());Params.AddIgnoredActor(C);
 FHitResult Hit;return !GetWorld()->LineTraceSingleByChannel(Hit,PC->GetPawn()->GetActorLocation()+FVector(0,0,40),C->GetActorLocation()+FVector(0,0,40),ECC_Camera,Params);
}
bool AForestNPCInteraction::TryInteractWith(ACharacter* C)
{return OpenInteraction(C,Dialogue && C && PersonaId(C)==TEXT("fawnia") && Dialogue->IsFirstDayMessengerTutorialPending());}
void AForestNPCInteraction::OpenNpcFinder()
{
 auto* PC=Player.Get();if(!PC || !PC->GetPawn() || Active.IsValid() || FinderStage==EFinderStage::Travelling || !Widget)return;
 if(auto* Messenger=TutorialMessenger.Get())if(auto* AI=Cast<AForestRoadAIController>(Messenger->GetController()))AI->CancelPlayerApproach();
 TutorialMessenger.Reset();bTutorialSearchIssued=false;FinderStage=EFinderStage::Selecting;PendingFinderPersona=NAME_None;PendingFeeItem.Reset();
 bFinderPreviousMouse=PC->bShowMouseCursor;PC->bShowMouseCursor=true;FInputModeUIOnly Mode;Mode.SetWidgetToFocus(Widget->TakeWidget());Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);PC->SetInputMode(Mode);Widget->ShowFinderSelection();
}
void AForestNPCInteraction::ToggleNpcFinder()
{if(FinderStage==EFinderStage::Selecting || FinderStage==EFinderStage::Confirming)CancelNpcFinder();else OpenNpcFinder();}
void AForestNPCInteraction::CloseNpcFinderMenu()
{
 if(auto* PC=Player.Get()){PC->bShowMouseCursor=bFinderPreviousMouse;PC->SetInputMode(FInputModeGameOnly());}
 PendingFinderPersona=NAME_None;PendingFeeItem.Reset();
}
void AForestNPCInteraction::CancelNpcFinder()
{
 if(FinderStage==EFinderStage::Travelling){StopNpcFinder(TEXT("已取消寻找；交付的物品不会退回。"),false);return;}
 if(FinderStage==EFinderStage::Confirming){FinderStage=EFinderStage::Selecting;PendingFinderPersona=NAME_None;PendingFeeItem.Reset();if(Widget)Widget->ShowFinderSelection();return;}
 FinderStage=EFinderStage::Closed;CloseNpcFinderMenu();if(Widget)Widget->ShowMessage(TEXT(""),TEXT(""),TEXT("V：寻找 NPC"));
}
ACharacter* AForestNPCInteraction::FindCharacterByPersona(const FName Persona) const
{for(TActorIterator<ACharacter> It(GetWorld());It;++It)if(PersonaId(*It)==Persona && Cast<AForestRoadAIController>(It->GetController()))return *It;return nullptr;}
bool AForestNPCInteraction::FindFeeItem(FString& OutItem) const
{
 OutItem.Reset();const auto* PC=Player.Get();APawn* Pawn=nullptr;if(PC)Pawn=PC->GetPawn();if(!Pawn)return false;
 auto* Map=FindFProperty<FMapProperty>(Pawn->GetClass(),TEXT("Backpack"));auto* Key=Map?CastField<FStrProperty>(Map->KeyProp):nullptr;auto* Value=Map?CastField<FIntProperty>(Map->ValueProp):nullptr;if(!Map || !Key || !Value)return false;
 FScriptMapHelper Helper(Map,Map->ContainerPtrToValuePtr<void>(Pawn));TArray<FString> Available;
 for(int32 I=0;I<Helper.GetMaxIndex();++I)if(Helper.IsValidIndex(I) && Value->GetPropertyValue(Helper.GetValuePtr(I))>0)Available.Add(Key->GetPropertyValue(Helper.GetKeyPtr(I)));
 if(Available.IsEmpty())return false;
 if(auto* Last=FindFProperty<FStrProperty>(Pawn->GetClass(),TEXT("LastPicked"))){const FString Preferred=Last->GetPropertyValue_InContainer(Pawn);if(Available.Contains(Preferred)){OutItem=Preferred;return true;}}
 Available.Sort();OutItem=Available[0];return true;
}
void AForestNPCInteraction::RebuildBackpackList() const
{
 const auto* PC=Player.Get();APawn* Pawn=nullptr;if(PC)Pawn=PC->GetPawn();if(!Pawn)return;
 auto* Map=FindFProperty<FMapProperty>(Pawn->GetClass(),TEXT("Backpack"));auto* Key=Map?CastField<FStrProperty>(Map->KeyProp):nullptr;auto* Value=Map?CastField<FIntProperty>(Map->ValueProp):nullptr;auto* List=FindFProperty<FStrProperty>(Pawn->GetClass(),TEXT("BackpackList"));if(!Map || !Key || !Value || !List)return;
 FScriptMapHelper Helper(Map,Map->ContainerPtrToValuePtr<void>(Pawn));TArray<FString> Lines;
 for(int32 I=0;I<Helper.GetMaxIndex();++I)if(Helper.IsValidIndex(I)){const int32 Count=Value->GetPropertyValue(Helper.GetValuePtr(I));if(Count>0)Lines.Add(FString::Printf(TEXT("%s × %d"),*Key->GetPropertyValue(Helper.GetKeyPtr(I)),Count));}
 Lines.Sort();List->SetPropertyValue_InContainer(Pawn,FString::Join(Lines,TEXT("\n")));if(auto* Refresh=Pawn->FindFunction(TEXT("RefreshHUD")))Pawn->ProcessEvent(Refresh,nullptr);
}
bool AForestNPCInteraction::ConsumeFeeItem(const FString& Item)
{
 const auto* PC=Player.Get();APawn* Pawn=nullptr;if(PC)Pawn=PC->GetPawn();if(!Pawn || Item.IsEmpty())return false;
 auto* Map=FindFProperty<FMapProperty>(Pawn->GetClass(),TEXT("Backpack"));auto* Key=Map?CastField<FStrProperty>(Map->KeyProp):nullptr;auto* Value=Map?CastField<FIntProperty>(Map->ValueProp):nullptr;if(!Map || !Key || !Value)return false;
 FScriptMapHelper Helper(Map,Map->ContainerPtrToValuePtr<void>(Pawn));
 for(int32 I=0;I<Helper.GetMaxIndex();++I)if(Helper.IsValidIndex(I) && Key->GetPropertyValue(Helper.GetKeyPtr(I))==Item){const int32 Count=Value->GetPropertyValue(Helper.GetValuePtr(I));if(Count<=0)return false;if(Count==1){Helper.RemoveAt(I);Helper.Rehash();}else Value->SetPropertyValue(Helper.GetValuePtr(I),Count-1);RebuildBackpackList();return true;}
 return false;
}
void AForestNPCInteraction::RefundFeeItem(const FString& Item)
{
 const auto* PC=Player.Get();APawn* Pawn=nullptr;if(PC)Pawn=PC->GetPawn();if(!Pawn || Item.IsEmpty())return;
 auto* Map=FindFProperty<FMapProperty>(Pawn->GetClass(),TEXT("Backpack"));auto* Key=Map?CastField<FStrProperty>(Map->KeyProp):nullptr;auto* Value=Map?CastField<FIntProperty>(Map->ValueProp):nullptr;if(!Map || !Key || !Value)return;
 FScriptMapHelper Helper(Map,Map->ContainerPtrToValuePtr<void>(Pawn));
 for(int32 I=0;I<Helper.GetMaxIndex();++I)if(Helper.IsValidIndex(I) && Key->GetPropertyValue(Helper.GetKeyPtr(I))==Item){Value->SetPropertyValue(Helper.GetValuePtr(I),Value->GetPropertyValue(Helper.GetValuePtr(I))+1);RebuildBackpackList();return;}
 int32 One=1;FString NewKey=Item;Helper.AddPair(&NewKey,&One);RebuildBackpackList();
}
bool AForestNPCInteraction::HasCompletePathTo(ACharacter* Character) const
{
 const auto* PC=Player.Get();APawn* Pawn=nullptr;if(PC)Pawn=PC->GetPawn();if(!Pawn || !Character)return false;
 const auto* Path=UNavigationSystemV1::FindPathToActorSynchronously(GetWorld(),Pawn->GetActorLocation(),Character,InteractionRange*0.7f,Pawn);return Path && Path->IsValid() && !Path->IsPartial() && Path->PathPoints.Num()>1;
}
void AForestNPCInteraction::SelectNpcForFinding(const FName Persona)
{
 if(FinderStage!=EFinderStage::Selecting && FinderStage!=EFinderStage::Confirming)return;PendingFinderPersona=Persona;FindFeeItem(PendingFeeItem);FinderStage=EFinderStage::Confirming;
 if(Widget){auto* Character=FindCharacterByPersona(Persona);Widget->ShowFinderConfirmation(Character?CharacterName(Character):(Persona==TEXT("natta")?TEXT("Natta"):Persona==TEXT("fawnia")?TEXT("Fawnia"):TEXT("森林女巫")),PendingFeeItem);}
}
void AForestNPCInteraction::ConfirmNpcFinding()
{
 if(FinderStage!=EFinderStage::Confirming)return;auto* Target=FindCharacterByPersona(PendingFinderPersona);
 if(!Target){FinderStage=EFinderStage::Closed;CloseNpcFinderMenu();FinderNotice=TEXT("这位 NPC 当前不在场景中，没有扣除物品。");FinderNoticeUntil=GetWorld()->GetTimeSeconds()+4;if(Widget)Widget->ShowMessage(TEXT("无法寻找"),TEXT(""),FinderNotice);return;}
 if(!CanReach(Target) && !HasCompletePathTo(Target)){FinderStage=EFinderStage::Closed;CloseNpcFinderMenu();FinderNotice=TEXT("目前没有可到达这位 NPC 的完整路线，没有扣除物品。");FinderNoticeUntil=GetWorld()->GetTimeSeconds()+4;if(Widget)Widget->ShowMessage(TEXT("无法寻找"),TEXT(""),FinderNotice);return;}
 FString Fee;if(!FindFeeItem(Fee) || !ConsumeFeeItem(Fee)){PendingFeeItem.Reset();if(Widget)Widget->ShowFinderConfirmation(CharacterName(Target),PendingFeeItem);return;}
 SpentFeeItem=Fee;FinderTarget=Target;FinderStage=EFinderStage::Travelling;FinderNextMove=0;CloseNpcFinderMenu();if(auto* AI=Cast<AForestRoadAIController>(Target->GetController()))AI->CancelPlayerApproach();UpdateNpcFinder();
}
void AForestNPCInteraction::StopNpcFinder(const FString& Notice,const bool bRefundFee)
{
 if(auto* PC=Player.Get())PC->StopMovement();if(bRefundFee && !SpentFeeItem.IsEmpty())RefundFeeItem(SpentFeeItem);SpentFeeItem.Reset();FinderTarget.Reset();FinderStage=EFinderStage::Closed;FinderNotice=Notice;FinderNoticeUntil=GetWorld()?GetWorld()->GetTimeSeconds()+4:0;
}
void AForestNPCInteraction::UpdateNpcFinder()
{
 if(FinderStage!=EFinderStage::Travelling)return;auto* PC=Player.Get();APawn* Pawn=nullptr;if(PC)Pawn=PC->GetPawn();auto* Target=FinderTarget.Get();if(!PC || !Pawn || !Target){StopNpcFinder(TEXT("NPC 已离开，交付的物品已退回。"),true);return;}
 if(PC->WasInputKeyJustPressed(EKeys::W) || PC->WasInputKeyJustPressed(EKeys::A) || PC->WasInputKeyJustPressed(EKeys::S) || PC->WasInputKeyJustPressed(EKeys::D) || PC->WasInputKeyJustPressed(EKeys::Escape)){StopNpcFinder(TEXT("已取消寻找；交付的物品不会退回。"),false);return;}
 if(CanReach(Target))
 {
  PC->StopMovement();FinderStage=EFinderStage::Closed;FinderTarget.Reset();SpentFeeItem.Reset();if(TryInteractWith(Target))return;FinderNotice=FString::Printf(TEXT("已经找到 %s。"),*CharacterName(Target));FinderNoticeUntil=GetWorld()->GetTimeSeconds()+4;return;
 }
 const double Now=GetWorld()->GetTimeSeconds();if(Now>=FinderNextMove)
 {
  if(!HasCompletePathTo(Target)){StopNpcFinder(TEXT("路线中断，交付的物品已退回。"),true);return;}
  UAIBlueprintHelperLibrary::SimpleMoveToActor(PC,Target);FinderNextMove=Now+1;
 }
 if(Widget)Widget->ShowMessage(FString::Printf(TEXT("正在寻找 %s"),*CharacterName(Target)),TEXT(""),FString::Printf(TEXT("已交付：%s　WASD 或 Esc 取消"),*SpentFeeItem));
}
bool AForestNPCInteraction::OpenInteraction(ACharacter* C,const bool bFirstDayTutorial)
{
 auto* PC=Player.Get();auto* AI=C?Cast<AForestRoadAIController>(C->GetController()):nullptr;
 if(!PC || Active.IsValid() || !AI || !Dialogue || !CanReach(C) || !AI->BeginInteraction(PC->GetPawn()))return false;
 Dialogue->SetOfflineTestMode(bOfflineTestMode);const bool Began=bFirstDayTutorial?Dialogue->BeginFirstDayMessengerTutorial():Dialogue->BeginConversation(PersonaId(C));if(!Began){AI->EndInteraction();return false;}
 Active=AI;PC->SetIgnoreMoveInput(true);PC->SetIgnoreLookInput(true);bLockedMovement=true;bPreviousMouse=PC->bShowMouseCursor;PC->bShowMouseCursor=true;
 RefreshConversation();FInputModeUIOnly Mode;Mode.SetWidgetToFocus(Widget->TakeWidget());Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);PC->SetInputMode(Mode);Widget->FocusInput();return true;
}
void AForestNPCInteraction::UpdateFirstDayTutorial()
{
 auto* PC=Player.Get();if(!PC || !PC->GetPawn() || !Dialogue || Active.IsValid() || !Dialogue->IsFirstDayMessengerTutorialPending())return;
 const double Now=GetWorld()->GetTimeSeconds();if(TutorialSearchAfter<=0)TutorialSearchAfter=Now+1.5;if(Now<TutorialSearchAfter)return;
 if(!TutorialMessenger.IsValid())
 {
  for(TActorIterator<ACharacter> It(GetWorld());It;++It)if(PersonaId(*It)==TEXT("fawnia") && Cast<AForestRoadAIController>(It->GetController())){TutorialMessenger=*It;break;}
 }
 auto* Messenger=TutorialMessenger.Get();auto* AI=Messenger?Cast<AForestRoadAIController>(Messenger->GetController()):nullptr;if(!Messenger || !AI)return;
 if(!bTutorialSearchIssued){bTutorialSearchIssued=AI->BeginPlayerApproach(PC->GetPawn());if(!bTutorialSearchIssued){TutorialSearchAfter=Now+1;return;}}
 if(CanReach(Messenger) && OpenInteraction(Messenger,true)){AI->CancelPlayerApproach();TutorialMessenger.Reset();}
}
bool AForestNPCInteraction::SubmitDialogueText(const FString& Text){return Active.IsValid() && Dialogue && Dialogue->SendPlayerMessage(Text);}
FString AForestNPCInteraction::GetDialogueStatus() const{return Dialogue?Dialogue->GetStatus():TEXT("对话系统尚未初始化");}
void AForestNPCInteraction::RefreshConversation(){if(Widget && Dialogue && Active.IsValid()){Widget->ShowConversation(CharacterName(Cast<ACharacter>(Active->GetPawn())),Dialogue->GetTranscript(),Dialogue->GetStatus(),Dialogue->IsWaiting());if(Dialogue->IsWaiting())Widget->SetKeyboardFocus();else Widget->FocusInput();}}
void AForestNPCInteraction::InteractNearby(){if(!bSuppressInteractUntilRelease && Nearby.IsValid() && !Active.IsValid())TryInteractWith(Nearby.Get());}
void AForestNPCInteraction::FinishInteraction()
{
 if(Dialogue && Active.IsValid())Dialogue->EndConversation();if(auto* AI=Active.Get())AI->EndInteraction();Active.Reset();bSuppressInteractUntilRelease=true;
 if(bLockedMovement){if(auto* PC=Player.Get()){PC->SetIgnoreMoveInput(false);PC->SetIgnoreLookInput(false);PC->bShowMouseCursor=bPreviousMouse;PC->SetInputMode(FInputModeGameOnly());}bLockedMovement=false;}
 if(Widget)Widget->ShowMessage(TEXT(""),TEXT(""),TEXT("V：寻找 NPC"));
}
void AForestNPCInteraction::Tick(float DT)
{
 Super::Tick(DT);auto* PC=Player.Get();if(!PC){PC=UGameplayStatics::GetPlayerController(this,0);Player=PC;}if(!PC || !PC->GetPawn())return;
 if(!Dialogue){Dialogue=GetGameInstance()->GetSubsystem<UForestNPCDialogueSubsystem>();if(Dialogue)Dialogue->OnConversationUpdated.AddDynamic(this,&AForestNPCInteraction::RefreshConversation);}
 if(!Widget){Widget=CreateWidget<UForestNPCInteractionWidget>(PC);if(Widget){Widget->Manager=this;Widget->AddToViewport(20);Widget->ShowMessage(TEXT(""),TEXT(""),TEXT("V：寻找 NPC"));}}if(!Widget)return;
 EnableInput(PC);if(InputComponent && !bInputBound){auto& InteractBinding=InputComponent->BindKey(EKeys::E,IE_Pressed,this,&AForestNPCInteraction::InteractNearby);InteractBinding.bConsumeInput=true;auto& FinderBinding=InputComponent->BindKey(EKeys::V,IE_Pressed,this,&AForestNPCInteraction::ToggleNpcFinder);FinderBinding.bConsumeInput=true;bInputBound=true;}
 if(FinderStage==EFinderStage::Travelling){UpdateNpcFinder();return;}
 if(FinderStage==EFinderStage::Selecting || FinderStage==EFinderStage::Confirming)return;
 UpdateFirstDayTutorial();
 if(Active.IsValid()){if(PC->WasInputKeyJustPressed(EKeys::Escape) || PC->GetViewTarget()!=PC->GetPawn() || !Active->GetPawn() || FVector::Dist(Active->GetPawn()->GetActorLocation(),PC->GetPawn()->GetActorLocation())>InteractionRange*1.8f)FinishInteraction();return;}
 if(bLockedMovement)FinishInteraction();if(bSuppressInteractUntilRelease){if(!PC->IsInputKeyDown(EKeys::E))bSuppressInteractUntilRelease=false;else return;}
 ACharacter* Nearest=nullptr;float Distance=InteractionRange;for(TActorIterator<ACharacter> It(GetWorld());It;++It)if(Cast<AForestRoadAIController>(It->GetController())){const float D=FVector::Dist(It->GetActorLocation(),PC->GetPawn()->GetActorLocation());if(D<Distance && CanReach(*It)){Distance=D;Nearest=*It;}}
 Nearby=Nearest;
 const double Now=GetWorld()->GetTimeSeconds();if(Now<FinderNoticeUntil){Widget->ShowMessage(TEXT("寻找 NPC"),TEXT(""),FinderNotice);return;}
 Widget->ShowMessage(TEXT(""),TEXT(""),Nearest?FString::Printf(TEXT("E：与 %s 交谈　V：寻找 NPC"),*CharacterName(Nearest)):TEXT("V：寻找 NPC"));
}
void AForestNPCInteraction::EndPlay(const EEndPlayReason::Type Reason)
{if(auto* Messenger=TutorialMessenger.Get())if(auto* AI=Cast<AForestRoadAIController>(Messenger->GetController()))AI->CancelPlayerApproach();if(FinderStage==EFinderStage::Travelling)StopNpcFinder(TEXT(""),false);else if(FinderStage!=EFinderStage::Closed){FinderStage=EFinderStage::Closed;CloseNpcFinderMenu();}FinishInteraction();if(Dialogue)Dialogue->OnConversationUpdated.RemoveDynamic(this,&AForestNPCInteraction::RefreshConversation);if(Widget)Widget->RemoveFromParent();Super::EndPlay(Reason);}
