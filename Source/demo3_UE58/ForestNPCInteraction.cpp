#include "ForestNPCInteraction.h"
#include "ForestNPCDialogueSubsystem.h"
#include "ForestRoadAIController.h"
#include "DailyMushroomSubsystem.h"
#include "ForestHarvestMagic.h"
#include "Components/InputComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "UObject/UnrealType.h"
#include "EngineUtils.h"
#include "Engine/GameInstance.h"
#include "InputCoreTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace ForestUI
{
 const FLinearColor Ink(0.012f,0.022f,0.018f,0.97f);
 const FLinearColor Moss(0.055f,0.11f,0.075f,0.98f);
 const FLinearColor Gold(0.72f,0.56f,0.25f,1.f);
 const FLinearColor Parchment(0.93f,0.87f,0.68f,1.f);
 const FLinearColor Muted(0.62f,0.70f,0.58f,1.f);
 const FLinearColor Fire(0.78f,0.28f,0.12f,1.f);
 const FLinearColor Wind(0.28f,0.68f,0.48f,1.f);
 const FLinearColor Water(0.20f,0.48f,0.70f,1.f);
 FSlateFontInfo Font(const int32 Size,const bool bBold=false){return FCoreStyle::GetDefaultFontStyle(bBold?TEXT("Bold"):TEXT("Regular"),Size);}
 const FSlateBrush* Fill(){return FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));}
}

TSharedRef<SWidget> UForestNPCInteractionWidget::RebuildWidget()
{
 SetIsFocusable(true);
 const auto NpcCard=[this](const TCHAR* Role,const TCHAR* Name,const TCHAR* Detail,const FName Persona,const FLinearColor Accent)
 {
  return SNew(SButton).ButtonColorAndOpacity(FLinearColor(0.055f,0.09f,0.065f,1.f)).ContentPadding(FMargin(12,9))
   .OnClicked_Lambda([this,Persona](){if(Manager.IsValid())Manager->SelectNpcForFinding(Persona);return FReply::Handled();})
   [SNew(SHorizontalBox)
    +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Fill).Padding(0,0,12,0)[SNew(SBorder).BorderImage(ForestUI::Fill()).BorderBackgroundColor(Accent).Padding(FMargin(3))]
    +SHorizontalBox::Slot().FillWidth(1.f)[SNew(SVerticalBox)
     +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Role)).Font(ForestUI::Font(10,true)).ColorAndOpacity(ForestUI::Muted)]
     +SVerticalBox::Slot().AutoHeight().Padding(0,2,0,0)[SNew(STextBlock).Text(FText::FromString(Name)).Font(ForestUI::Font(18,true)).ColorAndOpacity(ForestUI::Parchment)]
     +SVerticalBox::Slot().AutoHeight().Padding(0,2,0,0)[SNew(STextBlock).Text(FText::FromString(Detail)).Font(ForestUI::Font(11)).ColorAndOpacity(ForestUI::Muted)]]
    +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(TEXT("选择  ›"))).Font(ForestUI::Font(12,true)).ColorAndOpacity(Accent)]];
 };

 TSharedRef<SOverlay> Built=SNew(SOverlay)
 +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(FMargin(0,0,0,32))
 [SNew(SBox).WidthOverride(760.f)
  [SNew(SBorder).BorderImage(ForestUI::Fill()).BorderBackgroundColor(ForestUI::Gold).Padding(1.f)
   [SNew(SBorder).BorderImage(ForestUI::Fill()).BorderBackgroundColor(ForestUI::Ink).Padding(0.f)
    [SNew(SVerticalBox)
     +SVerticalBox::Slot().AutoHeight()
     [SNew(SBorder).BorderImage(ForestUI::Fill()).BorderBackgroundColor(ForestUI::Moss).Padding(FMargin(20,14))
      [SNew(SHorizontalBox)
       +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,12,0)[SNew(STextBlock).Text(FText::FromString(TEXT("✦"))).Font(ForestUI::Font(20,true)).ColorAndOpacity(ForestUI::Gold)]
       +SHorizontalBox::Slot().FillWidth(1.f)[SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("FOREST LEDGER · 森林手札"))).Font(ForestUI::Font(9,true)).ColorAndOpacity(ForestUI::Muted)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,2,0,0)[SAssignNew(Title,STextBlock).Font(ForestUI::Font(23,true)).ColorAndOpacity(ForestUI::Parchment)]]]]
     +SVerticalBox::Slot().AutoHeight().Padding(FMargin(20,16,20,10))
     [SAssignNew(Conversation,SVerticalBox).Visibility(EVisibility::Collapsed)
      +SVerticalBox::Slot().AutoHeight()[SNew(SBox).HeightOverride(230.f)[SAssignNew(Scroll,SScrollBox)+SScrollBox::Slot()[SAssignNew(Message,STextBlock).Font(ForestUI::Font(15)).ColorAndOpacity(ForestUI::Parchment).AutoWrapText(true)]]]
      +SVerticalBox::Slot().AutoHeight().Padding(0,12,0,8)[SAssignNew(Input,SEditableTextBox).HintText(FText::FromString(TEXT("写下想说的话……"))).Font(ForestUI::Font(14)).ForegroundColor(ForestUI::Parchment).BackgroundColor(FLinearColor(0.03f,0.05f,0.04f,1.f)).OnTextCommitted_Lambda([this](const FText&,ETextCommit::Type Method){if(Method==ETextCommit::OnEnter)SendClicked();})]
      +SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox)
       +SHorizontalBox::Slot().FillWidth(1.f).Padding(0,0,6,0)[SAssignNew(Send,SButton).ButtonColorAndOpacity(ForestUI::Wind).ContentPadding(FMargin(12,7)).OnClicked_Lambda([this](){SendClicked();return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(TEXT("发送  ENTER"))).Font(ForestUI::Font(12,true)).Justification(ETextJustify::Center).ColorAndOpacity(ForestUI::Parchment)]]
       +SHorizontalBox::Slot().FillWidth(1.f).Padding(6,0,0,0)[SNew(SButton).ButtonColorAndOpacity(FLinearColor(0.18f,0.20f,0.16f,1.f)).ContentPadding(FMargin(12,7)).OnClicked_Lambda([this](){CloseClicked();return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(TEXT("结束交谈  ESC"))).Font(ForestUI::Font(12,true)).Justification(ETextJustify::Center).ColorAndOpacity(ForestUI::Parchment)]]]
     ]
     +SVerticalBox::Slot().AutoHeight().Padding(FMargin(20,14,20,8))
     [SAssignNew(FinderSelection,SVerticalBox).Visibility(EVisibility::Collapsed)
      +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,7)[NpcCard(TEXT("NOVELIST"),TEXT("森林女巫"),TEXT("在故事与魔法之间等候"),TEXT("forest_witch"),ForestUI::Wind)]
      +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,7)[NpcCard(TEXT("BLACKSMITH"),TEXT("Natta"),TEXT("炉火旁的铁匠"),TEXT("natta"),ForestUI::Fire)]
      +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,7)[NpcCard(TEXT("MESSENGER"),TEXT("Fawnia"),TEXT("穿行森林的传信人"),TEXT("fawnia"),ForestUI::Water)]
      +SVerticalBox::Slot().AutoHeight().Padding(0,4,0,0)[SNew(SButton).ButtonColorAndOpacity(FLinearColor(0.14f,0.15f,0.13f,1.f)).OnClicked_Lambda([this](){FinderCancelClicked();return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(TEXT("收起手札  ESC"))).Justification(ETextJustify::Center).Font(ForestUI::Font(11,true)).ColorAndOpacity(ForestUI::Muted)]]]
     +SVerticalBox::Slot().AutoHeight().Padding(FMargin(20,14,20,8))
     [SAssignNew(FinderConfirmation,SVerticalBox).Visibility(EVisibility::Collapsed)
      +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,10)[SAssignNew(FinderFee,STextBlock).Font(ForestUI::Font(15,true)).ColorAndOpacity(ForestUI::Parchment).AutoWrapText(true)]
      +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)[SAssignNew(FinderItems,SComboBox<TSharedPtr<FString>>).OptionsSource(&FinderOptions)
       .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item){return SNew(STextBlock).Text(FText::FromString(Item.IsValid()?*Item:TEXT(""))).Font(ForestUI::Font(13)).ColorAndOpacity(ForestUI::Parchment);})
       .OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item,ESelectInfo::Type){if(Manager.IsValid())Manager->SelectFeeItem(Item.IsValid()?*Item:FString());})
       [SNew(STextBlock).Text_Lambda([this](){const auto Item=FinderItems.IsValid()?FinderItems->GetSelectedItem():nullptr;return FText::FromString(Item.IsValid()?*Item:TEXT("选择一件交付物品"));}).Font(ForestUI::Font(13,true)).ColorAndOpacity(ForestUI::Parchment)]]
      +SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox)
       +SHorizontalBox::Slot().FillWidth(1.f).Padding(0,0,6,0)[SAssignNew(FinderConfirm,SButton).ButtonColorAndOpacity(ForestUI::Gold).ContentPadding(FMargin(12,8)).OnClicked_Lambda([this](){FinderConfirmClicked();return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(TEXT("交付并启程"))).Font(ForestUI::Font(12,true)).Justification(ETextJustify::Center).ColorAndOpacity(FLinearColor(0.04f,0.04f,0.025f,1.f))]]
       +SHorizontalBox::Slot().FillWidth(1.f).Padding(6,0,0,0)[SNew(SButton).ButtonColorAndOpacity(FLinearColor(0.16f,0.18f,0.15f,1.f)).ContentPadding(FMargin(12,8)).OnClicked_Lambda([this](){FinderCancelClicked();return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(TEXT("返回"))).Font(ForestUI::Font(12,true)).Justification(ETextJustify::Center).ColorAndOpacity(ForestUI::Parchment)]]]
     ]
     +SVerticalBox::Slot().AutoHeight()
     [SNew(SBorder).BorderImage(ForestUI::Fill()).BorderBackgroundColor(FLinearColor(0.035f,0.06f,0.045f,1.f)).Padding(FMargin(20,9))
      [SAssignNew(Prompt,STextBlock).Font(ForestUI::Font(11)).ColorAndOpacity(ForestUI::Muted).AutoWrapText(true).Justification(ETextJustify::Center)]]
    ]]]];
 Root=Built;
 return Built;
}
void UForestNPCInteractionWidget::ShowMessage(const FString& Name,const FString&,const FString& Hint)
{
 if(!Title.IsValid())return;bFinderVisible=false;FinderSelection->SetVisibility(EVisibility::Collapsed);FinderConfirmation->SetVisibility(EVisibility::Collapsed);Title->SetText(FText::FromString(Name));Prompt->SetText(FText::FromString(Hint));Title->SetVisibility(Name.IsEmpty()?EVisibility::Collapsed:EVisibility::HitTestInvisible);Conversation->SetVisibility(EVisibility::Collapsed);Root->SetVisibility(Hint.IsEmpty()?EVisibility::Collapsed:EVisibility::HitTestInvisible);
}
void UForestNPCInteractionWidget::ShowConversation(const FString& Name,const FString& Body,const FString& Status,bool Waiting)
{
 if(!Title.IsValid())return;bFinderVisible=false;FinderSelection->SetVisibility(EVisibility::Collapsed);FinderConfirmation->SetVisibility(EVisibility::Collapsed);Root->SetVisibility(EVisibility::Visible);Title->SetVisibility(EVisibility::Visible);Conversation->SetVisibility(EVisibility::Visible);
 Title->SetText(FText::FromString(Name));Message->SetText(FText::FromString(Body.IsEmpty()?TEXT("你叫住了对方。可以开始交谈。"):Body));Prompt->SetText(FText::FromString(Status));Input->SetEnabled(!Waiting);Send->SetEnabled(!Waiting);Scroll->ScrollToEnd();
}
void UForestNPCInteractionWidget::ShowFinderSelection()
{
 if(!Title.IsValid())return;bFinderVisible=true;Root->SetVisibility(EVisibility::Visible);Title->SetVisibility(EVisibility::Visible);Title->SetText(FText::FromString(TEXT("选择要寻找的人")));Conversation->SetVisibility(EVisibility::Collapsed);FinderConfirmation->SetVisibility(EVisibility::Collapsed);FinderSelection->SetVisibility(EVisibility::Visible);Prompt->SetText(FText::FromString(TEXT("选择角色后，再从背包中决定由哪件物品带路。")));SetKeyboardFocus();
}
void UForestNPCInteractionWidget::ShowFinderConfirmation(const FString& CharacterName,const TArray<FString>& Items)
{
 if(!Title.IsValid())return;bFinderVisible=true;Root->SetVisibility(EVisibility::Visible);Title->SetVisibility(EVisibility::Visible);Title->SetText(FText::FromString(FString::Printf(TEXT("寻找 %s"),*CharacterName)));Conversation->SetVisibility(EVisibility::Collapsed);FinderSelection->SetVisibility(EVisibility::Collapsed);FinderConfirmation->SetVisibility(EVisibility::Visible);
 FinderItems->ClearSelection();FinderOptions.Reset();for(const FString& Item:Items)FinderOptions.Add(MakeShared<FString>(Item));FinderItems->RefreshOptions();FinderItems->SetEnabled(!Items.IsEmpty());FinderConfirm->SetEnabled(false);
 FinderFee->SetText(FText::FromString(Items.IsEmpty()?TEXT("需要交付 1 个物品。背包为空，暂时无法寻找。"):TEXT("需要交付 1 个物品。请选择背包中要交付的物品：")));
 Prompt->SetText(FText::FromString(TEXT("确认前会检查完整路线；返回不会扣除物品。")));SetKeyboardFocus();
}
void UForestNPCInteractionWidget::ShowSelectedFee(const FString& FeeItem)
{
 if(!FinderFee.IsValid())return;FinderConfirm->SetEnabled(!FeeItem.IsEmpty());FinderFee->SetText(FText::FromString(FeeItem.IsEmpty()?TEXT("请选择要交付的物品。"):FString::Printf(TEXT("将交付 1 个：%s"),*FeeItem)));
}
void UForestNPCInteractionWidget::FocusInput(){if(Input.IsValid() && Input->IsEnabled())FSlateApplication::Get().SetKeyboardFocus(Input,EFocusCause::SetDirectly);}
void UForestNPCInteractionWidget::SendClicked(){if(Manager.IsValid() && Input && Manager->SubmitDialogueText(Input->GetText().ToString()))Input->SetText(FText::GetEmpty());FocusInput();}
void UForestNPCInteractionWidget::CloseClicked(){if(Manager.IsValid())Manager->FinishInteraction();}
void UForestNPCInteractionWidget::FindNovelistClicked(){if(Manager.IsValid())Manager->SelectNpcForFinding(TEXT("forest_witch"));}
void UForestNPCInteractionWidget::FindBlacksmithClicked(){if(Manager.IsValid())Manager->SelectNpcForFinding(TEXT("natta"));}
void UForestNPCInteractionWidget::FindMessengerClicked(){if(Manager.IsValid())Manager->SelectNpcForFinding(TEXT("fawnia"));}
void UForestNPCInteractionWidget::FinderConfirmClicked(){if(Manager.IsValid())Manager->ConfirmNpcFinding();}
void UForestNPCInteractionWidget::FinderCancelClicked(){if(Manager.IsValid())Manager->CancelNpcFinder();}
FReply UForestNPCInteractionWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{if(Event.GetKey()==EKeys::Escape){if(bFinderVisible)FinderCancelClicked();else CloseClicked();return FReply::Handled();}return Super::NativeOnPreviewKeyDown(Geometry,Event);}
AForestNPCInteraction::AForestNPCInteraction(){PrimaryActorTick.bCanEverTick=true;SetActorEnableCollision(false);InputPriority=100;}
FString AForestNPCInteraction::CharacterName(const ACharacter* C) const
{const FString Name=GetNameSafe(C);return Name.Contains(TEXT("Natta"))?TEXT("Natta"):Name.Contains(TEXT("Fawnia"))?TEXT("Fawnia"):TEXT("森林女巫");}
FName AForestNPCInteraction::PersonaId(const ACharacter* C) const
{const FString Name=GetNameSafe(C);if(Name.Contains(TEXT("Natta")))return TEXT("natta");if(Name.Contains(TEXT("Fawnia")))return TEXT("fawnia");if(Name.Contains(TEXT("NPC_WanderTest")) || Name.Contains(TEXT("KindWitch")))return TEXT("forest_witch");return NAME_None;}
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
{for(TActorIterator<ACharacter> It(GetWorld());It;++It)if(!Persona.IsNone() && PersonaId(*It)==Persona)return *It;return nullptr;}
TArray<FString> AForestNPCInteraction::GetFeeItems() const
{
 TArray<FString> Items;const auto* PC=Player.Get();APawn* Pawn=nullptr;if(PC)Pawn=PC->GetPawn();if(!Pawn)return Items;
 auto* Map=FindFProperty<FMapProperty>(Pawn->GetClass(),TEXT("Backpack"));auto* Key=Map?CastField<FStrProperty>(Map->KeyProp):nullptr;auto* Value=Map?CastField<FIntProperty>(Map->ValueProp):nullptr;if(!Key || !Value)return Items;
 FScriptMapHelper Helper(Map,Map->ContainerPtrToValuePtr<void>(Pawn));for(int32 I=0;I<Helper.GetMaxIndex();++I)if(Helper.IsValidIndex(I) && Value->GetPropertyValue(Helper.GetValuePtr(I))>0)Items.Add(Key->GetPropertyValue(Helper.GetKeyPtr(I)));Items.Sort();return Items;
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
void AForestNPCInteraction::RequestTravelNarrative(const FString& ConsumedItem,ACharacter* Target)
{
 auto* Narrative=GetGameInstance()?GetGameInstance()->GetSubsystem<UForestTravelNarrativeSubsystem>():nullptr;if(!Narrative || !Target)return;
 FForestTravelNarrativeInput Input;Input.RequestId=NextTravelNarrativeRequestId++;Input.ConsumedItemId=ConsumedItem;Input.ConsumedItemName=ConsumedItem;Input.TargetNpcId=PersonaId(Target);Input.TargetNpcName=CharacterName(Target);
 FForestMagicItem MagicItem;
 if(auto* Harvest=GetWorld()->GetSubsystem<UForestHarvestMagic>(); Harvest && Harvest->FindItem(ConsumedItem,MagicItem))
 {Input.ConsumedItemId=MagicItem.Id;Input.ConsumedItemName=MagicItem.Name;Input.ConsumedItemDescription=MagicItem.Description;Input.bHasElementValue=true;Input.Element=MagicItem.Element;Input.PipValue=MagicItem.PipValue;Input.Value=MagicItem.Value;Input.bUsesPlaceholderValue=false;Input.ValueSource=TEXT("harvest_snapshot");}
 else if(auto* Daily=GetGameInstance()->GetSubsystem<UDailyMushroomSubsystem>())
 {
  FDailyMushroomCardRow Row;if(Daily->GetDailyMushroom(FName(*ConsumedItem),Row)){Input.bHasElementValue=true;Input.Element=Row.Element;Input.PipValue=Row.PipValue;Input.Value=Row.Value;Input.ValueSource=TEXT("current_day_mushroom_table_placeholder");}
 }
 ActiveTravelNarrativeRequestId=Input.RequestId;const auto Fallback=Narrative->BuildFallback(Input);EmergentTravelItemName=Fallback.EmergentItemName;TravelNarrativeText=Fallback.ShortNarrative;
 FForestTravelNarrativeCompletion Completion;Completion.BindDynamic(this,&AForestNPCInteraction::HandleTravelNarrative);Narrative->GenerateTravelNarrative(Input,Completion);
}
void AForestNPCInteraction::HandleTravelNarrative(const FForestTravelNarrativeResult& Result)
{
 if(Result.RequestId!=ActiveTravelNarrativeRequestId)return;EmergentTravelItemName=Result.EmergentItemName;TravelNarrativeText=Result.ShortNarrative;
}
void AForestNPCInteraction::SelectNpcForFinding(const FName Persona)
{
 if(FinderStage!=EFinderStage::Selecting && FinderStage!=EFinderStage::Confirming)return;PendingFinderPersona=Persona;PendingFeeItem.Reset();FinderStage=EFinderStage::Confirming;
 if(Widget){auto* Character=FindCharacterByPersona(Persona);Widget->ShowFinderConfirmation(Character?CharacterName(Character):(Persona==TEXT("natta")?TEXT("Natta"):Persona==TEXT("fawnia")?TEXT("Fawnia"):TEXT("森林女巫")),GetFeeItems());}
}
void AForestNPCInteraction::SelectFeeItem(const FString& Item)
{
 if(FinderStage!=EFinderStage::Confirming)return;PendingFeeItem=GetFeeItems().Contains(Item)?Item:FString();if(Widget)Widget->ShowSelectedFee(PendingFeeItem);
}
void AForestNPCInteraction::ConfirmNpcFinding()
{
 if(FinderStage!=EFinderStage::Confirming || PendingFeeItem.IsEmpty())return;auto* Target=FindCharacterByPersona(PendingFinderPersona);
 if(!Target){FinderStage=EFinderStage::Closed;CloseNpcFinderMenu();FinderNotice=TEXT("这位 NPC 当前不在场景中，没有扣除物品。");FinderNoticeUntil=GetWorld()->GetTimeSeconds()+4;if(Widget)Widget->ShowMessage(TEXT("无法寻找"),TEXT(""),FinderNotice);return;}
 if(!CanReach(Target) && !HasCompletePathTo(Target)){FinderStage=EFinderStage::Closed;CloseNpcFinderMenu();FinderNotice=TEXT("目前没有可到达这位 NPC 的完整路线，没有扣除物品。");FinderNoticeUntil=GetWorld()->GetTimeSeconds()+4;if(Widget)Widget->ShowMessage(TEXT("无法寻找"),TEXT(""),FinderNotice);return;}
 const FString Fee=PendingFeeItem;if(!GetFeeItems().Contains(Fee) || !ConsumeFeeItem(Fee)){PendingFeeItem.Reset();if(Widget)Widget->ShowFinderConfirmation(CharacterName(Target),GetFeeItems());return;}
 SpentFeeItem=Fee;FinderTarget=Target;FinderStage=EFinderStage::Travelling;FinderNextMove=0;CloseNpcFinderMenu();if(auto* AI=Cast<AForestRoadAIController>(Target->GetController()))AI->CancelPlayerApproach();RequestTravelNarrative(Fee,Target);UpdateNpcFinder();
}
void AForestNPCInteraction::StopNpcFinder(const FString& Notice,const bool bRefundFee)
{
 if(auto* PC=Player.Get())PC->StopMovement();if(bRefundFee && !SpentFeeItem.IsEmpty())RefundFeeItem(SpentFeeItem);SpentFeeItem.Reset();FinderTarget.Reset();FinderStage=EFinderStage::Closed;ActiveTravelNarrativeRequestId=0;EmergentTravelItemName.Reset();TravelNarrativeText.Reset();FinderNotice=Notice;FinderNoticeUntil=GetWorld()?GetWorld()->GetTimeSeconds()+4:0;
}
void AForestNPCInteraction::UpdateNpcFinderFacing(const float DeltaTime)
{
 if(FinderStage!=EFinderStage::Travelling)return;
 auto* PC=Player.Get();APawn* Pawn=nullptr;if(PC)Pawn=PC->GetPawn();if(!PC || !Pawn)return;
 FVector Direction=Pawn->GetVelocity();Direction.Z=0.f;if(Direction.SizeSquared2D()<FMath::Square(8.f))return;
 const float DesiredYaw=Direction.Rotation().Yaw;const float SafeDelta=FMath::Clamp(DeltaTime,0.f,0.1f);
 FRotator PawnTarget=Pawn->GetActorRotation();PawnTarget.Pitch=0.f;PawnTarget.Yaw=DesiredYaw;PawnTarget.Roll=0.f;
 Pawn->SetActorRotation(FMath::RInterpTo(Pawn->GetActorRotation(),PawnTarget,SafeDelta,10.f));
 FRotator ControlTarget=PC->GetControlRotation();ControlTarget.Yaw=DesiredYaw;
 PC->SetControlRotation(FMath::RInterpTo(PC->GetControlRotation(),ControlTarget,SafeDelta,8.f));
}
void AForestNPCInteraction::UpdateNpcFinder()
{
 if(FinderStage!=EFinderStage::Travelling)return;auto* PC=Player.Get();APawn* Pawn=nullptr;if(PC)Pawn=PC->GetPawn();auto* Target=FinderTarget.Get();if(!PC || !Pawn || !Target){StopNpcFinder(TEXT("NPC 已离开，交付的物品已退回。"),true);return;}
 if(PC->WasInputKeyJustPressed(EKeys::W) || PC->WasInputKeyJustPressed(EKeys::A) || PC->WasInputKeyJustPressed(EKeys::S) || PC->WasInputKeyJustPressed(EKeys::D) || PC->WasInputKeyJustPressed(EKeys::Escape)){StopNpcFinder(TEXT("已取消寻找；交付的物品不会退回。"),false);return;}
 if(CanReach(Target))
 {
  PC->StopMovement();FinderStage=EFinderStage::Closed;FinderTarget.Reset();SpentFeeItem.Reset();ActiveTravelNarrativeRequestId=0;if(TryInteractWith(Target))return;FinderNotice=FString::Printf(TEXT("已经找到 %s。"),*CharacterName(Target));FinderNoticeUntil=GetWorld()->GetTimeSeconds()+4;return;
 }
 const double Now=GetWorld()->GetTimeSeconds();if(Now>=FinderNextMove)
 {
  if(!HasCompletePathTo(Target)){StopNpcFinder(TEXT("路线中断，交付的物品已退回。"),true);return;}
  UAIBlueprintHelperLibrary::SimpleMoveToActor(PC,Target);FinderNextMove=Now+1;
 }
 const FString Emergence=EmergentTravelItemName.IsEmpty()?TEXT("路引正在形成……"):FString::Printf(TEXT("%s：%s"),*EmergentTravelItemName,*TravelNarrativeText);
 if(Widget)Widget->ShowMessage(FString::Printf(TEXT("正在寻找 %s"),*CharacterName(Target)),TEXT(""),FString::Printf(TEXT("已交付：%s\n%s\nWASD 或 Esc 取消"),*SpentFeeItem,*Emergence));
}
bool AForestNPCInteraction::OpenInteraction(ACharacter* C,const bool bFirstDayTutorial)
{
 auto* PC=Player.Get();auto* AI=C?Cast<AAIController>(C->GetController()):nullptr;auto* RoadAI=Cast<AForestRoadAIController>(AI);
 if(!PC || Active.IsValid() || !AI || !Dialogue || PersonaId(C).IsNone() || !CanReach(C))return false;
 if(RoadAI){if(!RoadAI->BeginInteraction(PC->GetPawn()))return false;}else AI->StopMovement();
 Dialogue->SetOfflineTestMode(bOfflineTestMode);const bool Began=bFirstDayTutorial?Dialogue->BeginFirstDayMessengerTutorial():Dialogue->BeginConversation(PersonaId(C));if(!Began){if(RoadAI)RoadAI->EndInteraction();return false;}
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
 if(Dialogue && Active.IsValid())Dialogue->EndConversation();if(auto* AI=Cast<AForestRoadAIController>(Active.Get()))AI->EndInteraction();Active.Reset();bSuppressInteractUntilRelease=true;
 if(bLockedMovement){if(auto* PC=Player.Get()){PC->SetIgnoreMoveInput(false);PC->SetIgnoreLookInput(false);PC->bShowMouseCursor=bPreviousMouse;PC->SetInputMode(FInputModeGameOnly());}bLockedMovement=false;}
 if(Widget)Widget->ShowMessage(TEXT(""),TEXT(""),TEXT("V：寻找 NPC"));
}
void AForestNPCInteraction::Tick(float DT)
{
 Super::Tick(DT);auto* PC=Player.Get();if(!PC){PC=UGameplayStatics::GetPlayerController(this,0);Player=PC;}if(!PC || !PC->GetPawn())return;
 if(!Dialogue){Dialogue=GetGameInstance()->GetSubsystem<UForestNPCDialogueSubsystem>();if(Dialogue)Dialogue->OnConversationUpdated.AddDynamic(this,&AForestNPCInteraction::RefreshConversation);}
 if(!Widget){Widget=CreateWidget<UForestNPCInteractionWidget>(PC);if(Widget){Widget->Manager=this;Widget->AddToViewport(20);Widget->ShowMessage(TEXT(""),TEXT(""),TEXT("V：寻找 NPC"));}}if(!Widget)return;
 EnableInput(PC);if(InputComponent && !bInputBound){auto& InteractBinding=InputComponent->BindKey(EKeys::E,IE_Pressed,this,&AForestNPCInteraction::InteractNearby);InteractBinding.bConsumeInput=false;auto& FinderBinding=InputComponent->BindKey(EKeys::V,IE_Pressed,this,&AForestNPCInteraction::ToggleNpcFinder);FinderBinding.bConsumeInput=true;bInputBound=true;}
 if(FinderStage==EFinderStage::Travelling){UpdateNpcFinder();UpdateNpcFinderFacing(DT);return;}
 if(FinderStage==EFinderStage::Selecting || FinderStage==EFinderStage::Confirming)return;
 UpdateFirstDayTutorial();
 if(Active.IsValid()){if(!Cast<AForestRoadAIController>(Active.Get()))Active->StopMovement();if(PC->WasInputKeyJustPressed(EKeys::Escape) || PC->GetViewTarget()!=PC->GetPawn() || !Active->GetPawn() || FVector::Dist(Active->GetPawn()->GetActorLocation(),PC->GetPawn()->GetActorLocation())>InteractionRange*1.8f)FinishInteraction();return;}
 if(bLockedMovement)FinishInteraction();if(bSuppressInteractUntilRelease){if(!PC->IsInputKeyDown(EKeys::E))bSuppressInteractUntilRelease=false;else return;}
 ACharacter* Nearest=nullptr;float Distance=InteractionRange;for(TActorIterator<ACharacter> It(GetWorld());It;++It)if(!PersonaId(*It).IsNone() && Cast<AAIController>(It->GetController())){const float D=FVector::Dist(It->GetActorLocation(),PC->GetPawn()->GetActorLocation());if(D<Distance && CanReach(*It)){Distance=D;Nearest=*It;}}
 Nearby=Nearest;
 const double Now=GetWorld()->GetTimeSeconds();if(Now<FinderNoticeUntil){Widget->ShowMessage(TEXT("寻找 NPC"),TEXT(""),FinderNotice);return;}
 Widget->ShowMessage(TEXT(""),TEXT(""),Nearest?FString::Printf(TEXT("E：与 %s 交谈　V：寻找 NPC"),*CharacterName(Nearest)):TEXT("V：寻找 NPC"));
}
void AForestNPCInteraction::EndPlay(const EEndPlayReason::Type Reason)
{if(auto* Messenger=TutorialMessenger.Get())if(auto* AI=Cast<AForestRoadAIController>(Messenger->GetController()))AI->CancelPlayerApproach();if(FinderStage==EFinderStage::Travelling)StopNpcFinder(TEXT(""),false);else if(FinderStage!=EFinderStage::Closed){FinderStage=EFinderStage::Closed;CloseNpcFinderMenu();}FinishInteraction();if(Dialogue)Dialogue->OnConversationUpdated.RemoveDynamic(this,&AForestNPCInteraction::RefreshConversation);if(Widget)Widget->RemoveFromParent();Super::EndPlay(Reason);}

void UForestNPCInteractionWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
 Super::OnWorldBeginPlay(InWorld);
 if(InWorld.IsGameWorld() && InWorld.GetGameInstance() && InWorld.GetGameInstance()->GetWorld()==&InWorld && InWorld.GetNetMode()!=NM_DedicatedServer && !UGameplayStatics::GetActorOfClass(&InWorld,AForestNPCInteraction::StaticClass()))
 {
  FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
  InWorld.SpawnActor<AForestNPCInteraction>(AForestNPCInteraction::StaticClass(),FTransform::Identity,Params);
 }
}

void UForestNPCInteractionWorldSubsystem::Tick(float DeltaTime)
{
 UWorld* World=GetWorld();if(!World || !World->IsGameWorld() || !World->GetGameInstance() || World->GetGameInstance()->GetWorld()!=World || World->GetNetMode()==NM_DedicatedServer)return;
 BootstrapDelay+=DeltaTime;if(BootstrapDelay<1.f)return;BootstrapDelay=0;
 if(!UGameplayStatics::GetActorOfClass(World,AForestNPCInteraction::StaticClass()))
 {FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;World->SpawnActor<AForestNPCInteraction>(AForestNPCInteraction::StaticClass(),FTransform::Identity,Params);}
 // Existing level blueprints may still have the old generic controller. Bind the actual
 // scene NPCs to the controller used by approach, tutorial, stop/resume and dialogue.
 for(TActorIterator<ACharacter> It(World);It;++It)
 {
  ACharacter* Character=*It;const FString Name=Character->GetName();
  const bool Supported=Name.Contains(TEXT("NPC_WanderTest")) || Name.Contains(TEXT("NPC_Natta_Wander")) || Name.Contains(TEXT("NPC_Fawnia_Wander"));
  if(!Supported || Character->IsPlayerControlled() || Cast<AForestRoadAIController>(Character->GetController()))continue;
  FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
  auto* Controller=World->SpawnActor<AForestRoadAIController>(AForestRoadAIController::StaticClass(),Character->GetActorTransform(),Params);
  if(!Controller)continue;
  if(auto* Previous=Character->GetController())Previous->UnPossess();
  Controller->Possess(Character);
  UE_LOG(LogTemp,Log,TEXT("Forest NPC runtime connected: %s -> %s"),*Name,*Controller->GetName());
 }
}
TStatId UForestNPCInteractionWorldSubsystem::GetStatId() const
{RETURN_QUICK_DECLARE_CYCLE_STAT(UForestNPCInteractionWorldSubsystem,STATGROUP_Tickables);}
