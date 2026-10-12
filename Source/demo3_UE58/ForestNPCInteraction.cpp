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
#include "Kismet/GameplayStatics.h"
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
 Prompt=WidgetTree->ConstructWidget<UTextBlock>();Prompt->SetWrapTextAt(550);Box->AddChildToVerticalBox(Prompt);
 auto Font=Title->GetFont();Font.Size=22;Title->SetFont(Font);Font.Size=18;Message->SetFont(Font);Prompt->SetFont(Font);
 Prompt->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f,0.9f,0.65f)));return Super::RebuildWidget();
}
void UForestNPCInteractionWidget::ShowMessage(const FString& Name,const FString&,const FString& Hint)
{
 if(!Title)return;Title->SetText(FText::FromString(Name));Prompt->SetText(FText::FromString(Hint));Title->SetVisibility(Name.IsEmpty()?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);Conversation->SetVisibility(ESlateVisibility::Collapsed);SetVisibility(Hint.IsEmpty()?ESlateVisibility::Hidden:ESlateVisibility::HitTestInvisible);
}
void UForestNPCInteractionWidget::ShowConversation(const FString& Name,const FString& Body,const FString& Status,bool Waiting)
{
 if(!Title)return;SetVisibility(ESlateVisibility::Visible);Title->SetVisibility(ESlateVisibility::Visible);Conversation->SetVisibility(ESlateVisibility::Visible);
 Title->SetText(FText::FromString(Name));Message->SetText(FText::FromString(Body.IsEmpty()?TEXT("你叫住了对方。可以开始交谈。"):Body));Prompt->SetText(FText::FromString(Status));Input->SetIsEnabled(!Waiting);Send->SetIsEnabled(!Waiting);Scroll->ScrollToEnd();
}
void UForestNPCInteractionWidget::FocusInput(){if(Input && Input->GetIsEnabled())Input->SetKeyboardFocus();}
void UForestNPCInteractionWidget::SendClicked(){if(Manager.IsValid() && Input && Manager->SubmitDialogueText(Input->GetText().ToString()))Input->SetText(FText::GetEmpty());FocusInput();}
void UForestNPCInteractionWidget::CloseClicked(){if(Manager.IsValid())Manager->FinishInteraction();}
void UForestNPCInteractionWidget::TextCommitted(const FText&,ETextCommit::Type Method){if(Method==ETextCommit::OnEnter)SendClicked();}
FReply UForestNPCInteractionWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{if(Event.GetKey()==EKeys::Escape){CloseClicked();return FReply::Handled();}return Super::NativeOnPreviewKeyDown(Geometry,Event);}
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
 if(Widget)Widget->ShowMessage(TEXT(""),TEXT(""),TEXT(""));
}
void AForestNPCInteraction::Tick(float DT)
{
 Super::Tick(DT);auto* PC=Player.Get();if(!PC){PC=UGameplayStatics::GetPlayerController(this,0);Player=PC;}if(!PC || !PC->GetPawn())return;
 if(!Dialogue){Dialogue=GetGameInstance()->GetSubsystem<UForestNPCDialogueSubsystem>();if(Dialogue)Dialogue->OnConversationUpdated.AddDynamic(this,&AForestNPCInteraction::RefreshConversation);}
 if(!Widget){Widget=CreateWidget<UForestNPCInteractionWidget>(PC);if(Widget){Widget->Manager=this;Widget->AddToViewport(20);Widget->ShowMessage(TEXT(""),TEXT(""),TEXT(""));}}if(!Widget)return;
 UpdateFirstDayTutorial();
 if(Active.IsValid()){if(PC->WasInputKeyJustPressed(EKeys::Escape) || PC->GetViewTarget()!=PC->GetPawn() || !Active->GetPawn() || FVector::Dist(Active->GetPawn()->GetActorLocation(),PC->GetPawn()->GetActorLocation())>InteractionRange*1.8f)FinishInteraction();return;}
 if(bLockedMovement)FinishInteraction();if(bSuppressInteractUntilRelease){if(!PC->IsInputKeyDown(EKeys::E))bSuppressInteractUntilRelease=false;else return;}
 ACharacter* Nearest=nullptr;float Distance=InteractionRange;for(TActorIterator<ACharacter> It(GetWorld());It;++It)if(Cast<AForestRoadAIController>(It->GetController())){const float D=FVector::Dist(It->GetActorLocation(),PC->GetPawn()->GetActorLocation());if(D<Distance && CanReach(*It)){Distance=D;Nearest=*It;}}
 Nearby=Nearest;
 if(Nearest){EnableInput(PC);if(InputComponent && !bInputBound){auto& Binding=InputComponent->BindKey(EKeys::E,IE_Pressed,this,&AForestNPCInteraction::InteractNearby);Binding.bConsumeInput=true;bInputBound=true;}}
 else DisableInput(PC);
 Widget->ShowMessage(TEXT(""),TEXT(""),Nearest?FString::Printf(TEXT("E：与 %s 交谈"),*CharacterName(Nearest)):TEXT(""));
}
void AForestNPCInteraction::EndPlay(const EEndPlayReason::Type Reason)
{if(auto* Messenger=TutorialMessenger.Get())if(auto* AI=Cast<AForestRoadAIController>(Messenger->GetController()))AI->CancelPlayerApproach();FinishInteraction();if(Dialogue)Dialogue->OnConversationUpdated.RemoveDynamic(this,&AForestNPCInteraction::RefreshConversation);if(Widget)Widget->RemoveFromParent();Super::EndPlay(Reason);}
