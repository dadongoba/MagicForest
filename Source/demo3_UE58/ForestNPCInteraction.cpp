#include "ForestNPCInteraction.h"
#include "ForestUITheme.h"
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
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SNullWidget.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace ForestUI
{
 using ForestTheme::Ink;using ForestTheme::Muted;using ForestTheme::Moss;
 using ForestTheme::Font;using ForestTheme::Fill;
 FSlateFontInfo FinderFont(int32 Size){return Font(FMath::Max(16,Size+4));}

 struct FItemView
 {
  FString Name,Attribute,Description;
  int32 Icon=-1,Count=0;
 };
 FItemView ReadItem(UWorld* World,const FString& Key)
 {
  FItemView View;View.Name=Key;View.Description=TEXT("这件物品还没有留下详细记录，也可以交给引路的魔法。");
  if(auto* Harvest=World?World->GetSubsystem<UForestHarvestMagic>():nullptr)
  {
   View.Count=Harvest->ReadBag().FindRef(Key);
   FForestMagicItem Item;
   if(Harvest->FindItem(Key,Item))
   {
    View.Name=Item.Name;View.Description=Item.Description;
    View.Attribute=FString::Printf(TEXT("%s%d"),*UForestHarvestMagic::ElementText(Item.Element),Item.PipValue);
    View.Icon=Item.Element==EAlchemyElement::Fire?0:Item.Element==EAlchemyElement::Water?2:1;
   }
  }
  return View;
 }
 TSharedRef<SWidget> Portrait(float Width,float Height)
 {
  static const FSlateRoundedBoxBrush Frame(FLinearColor(1,1,1,.08f),FVector4(35,35,4,4),ForestTheme::Edge,1.f);
  return SNew(SBox).WidthOverride(Width).HeightOverride(Height)
   [SNew(SBorder).BorderImage(&Frame).HAlign(HAlign_Center).VAlign(VAlign_Center)
    [SNew(STextBlock).Text(FText::FromString(TEXT("头像待补"))).Font(FinderFont(12)).ColorAndOpacity(Muted).WrapTextAt(FMath::Max(1.f,Width-10.f)).Justification(ETextJustify::Center)]];
 }
}

void UForestNPCInteractionWidget::SetPanel(EPanel Mode)
{
 Panel=Mode;bFinderVisible=Mode==EPanel::Selecting || Mode==EPanel::Confirming;
 Switcher->SetActiveWidgetIndex(static_cast<int32>(Mode));
 Root->SetVisibility(bFinderVisible || Mode==EPanel::Conversation?EVisibility::Visible:EVisibility::HitTestInvisible);
 AdaptivePanel->SetVAlign(bFinderVisible?VAlign_Center:VAlign_Bottom);
 Title->SetVisibility(Mode==EPanel::Travelling?EVisibility::Collapsed:EVisibility::HitTestInvisible);
 Subtitle->SetVisibility(bFinderVisible?EVisibility::HitTestInvisible:EVisibility::Collapsed);
 Prompt->SetVisibility(Mode==EPanel::Travelling?EVisibility::Collapsed:EVisibility::HitTestInvisible);
}
TSharedRef<SWidget> UForestNPCInteractionWidget::RebuildWidget()
{
 SetIsFocusable(true);
 auto Text=[this](const FString& Value,int32 Size){return SNew(STextBlock).Text(FText::FromString(Value))
  .Font_Lambda([this,Size](){return Panel==EPanel::Selecting || Panel==EPanel::Confirming || Panel==EPanel::Travelling?ForestUI::FinderFont(Size):ForestUI::Font(Size);})
  .ColorAndOpacity(ForestUI::Ink).AutoWrapText(true);};
 auto Secondary=[&Text](const TCHAR* Label,FOnClicked Action)->TSharedRef<SWidget>{return SNew(SButton).ButtonStyle(&ForestTheme::SecondaryButton()).ContentPadding(FMargin(16,10)).OnClicked(Action)[Text(Label,12)];};
 auto Cancel=FOnClicked::CreateLambda([this](){FinderCancelClicked();return FReply::Handled();});
 auto NpcCard=[this](const TCHAR* Role,const TCHAR* Name,const TCHAR* Detail,FName Persona)->TSharedRef<SWidget>
 {
  const FString SafeName(Name);
  return SNew(SBox).WidthOverride_Lambda([this](){return FMath::Max(1.f,FMath::Min(260.f,LayoutWidth-64.f));})
  [SNew(SButton).ButtonStyle(&ForestTheme::SecondaryButton()).ContentPadding(FMargin(16))
   .ButtonColorAndOpacity_Lambda([this,Persona](){return SelectedFinderPersona==Persona?ForestTheme::SelectedPaper:FLinearColor::White;})
   .OnClicked_Lambda([this,Persona,SafeName](){SelectedFinderPersona=Persona;Prompt->SetText(FText::FromString(FString::Printf(TEXT("想去见 %s · 还未消耗物品"),*SafeName)));return FReply::Handled();})
   [SNew(SVerticalBox)
    +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0,0,0,12)[ForestUI::Portrait(100,112)]
    +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Role)).Font(ForestUI::FinderFont(10)).ColorAndOpacity(ForestUI::Muted).Justification(ETextJustify::Center).AutoWrapText(true)]
    +SVerticalBox::Slot().AutoHeight().Padding(0,6)[SNew(STextBlock).Text(FText::FromString(Name)).Font(ForestUI::FinderFont(18)).ColorAndOpacity(ForestUI::Ink).Justification(ETextJustify::Center).AutoWrapText(true)]
    +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Detail)).Font(ForestUI::FinderFont(10)).ColorAndOpacity(ForestUI::Muted).Justification(ETextJustify::Center).AutoWrapText(true)]]];
 };
 TSharedRef<SWidget> Selection=SNew(SVerticalBox)
  +SVerticalBox::Slot().FillHeight(1).Padding(0,16)
   [SNew(SScrollBox).ScrollBarAlwaysVisible(true)+SScrollBox::Slot()
    [SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(16,16))
     +SWrapBox::Slot()[NpcCard(TEXT("小说家"),TEXT("森林女巫"),TEXT("在故事与魔法之间等候"),TEXT("forest_witch"))]
     +SWrapBox::Slot()[NpcCard(TEXT("铁匠"),TEXT("Natta"),TEXT("有话直说，也愿意帮你想办法"),TEXT("natta"))]
     +SWrapBox::Slot()[NpcCard(TEXT("传信人"),TEXT("Fawnia"),TEXT("穿行森林的传信人"),TEXT("fawnia"))]]]
  +SVerticalBox::Slot().AutoHeight()
   [SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(10,8))
    +SWrapBox::Slot()[Secondary(TEXT("收起"),Cancel)]
    +SWrapBox::Slot()[SNew(SButton).ButtonStyle(&ForestTheme::PrimaryButton()).ContentPadding(FMargin(16,10)).IsEnabled_Lambda([this](){return !SelectedFinderPersona.IsNone();})
     .OnClicked_Lambda([this](){if(Manager.IsValid() && !SelectedFinderPersona.IsNone())Manager->SelectNpcForFinding(SelectedFinderPersona);return FReply::Handled();})
     [SNew(STextBlock).Text(FText::FromString(TEXT("选择带路的小物 →"))).Font(ForestUI::FinderFont(12)).ColorAndOpacity(ForestTheme::OnMoss).AutoWrapText(true)]]];
 TSharedRef<SWidget> Confirmation=SNew(SVerticalBox)
  +SVerticalBox::Slot().FillHeight(1).Padding(0,16)
   [SNew(SScrollBox).ScrollBarAlwaysVisible(true)+SScrollBox::Slot()
    [SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(24,18))
     +SWrapBox::Slot()[SNew(SBox).WidthOverride_Lambda([this](){const float Inner=FMath::Max(1.f,LayoutWidth-84.f);return LayoutWidth<880.f?Inner:(Inner-24.f)*.55f;})
      [SAssignNew(FinderGrid,SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(9,9))]]
     +SWrapBox::Slot()[SNew(SBox).WidthOverride_Lambda([this](){const float Inner=FMath::Max(1.f,LayoutWidth-84.f);return LayoutWidth<880.f?Inner:(Inner-24.f)*.45f;})
      [SNew(SVerticalBox)
       +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,16)
        [SNew(SHorizontalBox)
         +SHorizontalBox::Slot().AutoWidth().Padding(0,0,12,0)[ForestUI::Portrait(52,60)]
         +SHorizontalBox::Slot().FillWidth(1)[SNew(SVerticalBox)
          +SVerticalBox::Slot().AutoHeight()[Text(TEXT("这次要去见"),10)]
          +SVerticalBox::Slot().AutoHeight().Padding(0,4)[SAssignNew(FinderTargetName,STextBlock).Font(ForestUI::FinderFont(16)).ColorAndOpacity(ForestUI::Ink).AutoWrapText(true)]]]
       +SVerticalBox::Slot().AutoHeight()[SAssignNew(FinderItemName,STextBlock).Font(ForestUI::FinderFont(20)).ColorAndOpacity(ForestUI::Ink).AutoWrapText(true)]
       +SVerticalBox::Slot().AutoHeight().Padding(0,6)[SAssignNew(FinderItemAttribute,STextBlock).Font(ForestUI::FinderFont(11)).ColorAndOpacity(ForestUI::Muted).AutoWrapText(true)]
       +SVerticalBox::Slot().AutoHeight().Padding(0,8)[SAssignNew(FinderItemDescription,STextBlock).Font(ForestUI::FinderFont(16)).ColorAndOpacity(ForestUI::Ink).AutoWrapText(true)]
       +SVerticalBox::Slot().AutoHeight().Padding(0,10,0,0)[SAssignNew(FinderFee,STextBlock).Font(ForestUI::FinderFont(12)).ColorAndOpacity(ForestUI::Moss).AutoWrapText(true)]]]]]
  +SVerticalBox::Slot().AutoHeight()
   [SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(10,8))
    +SWrapBox::Slot()[Secondary(TEXT("返回选人"),Cancel)]
    +SWrapBox::Slot()[SAssignNew(FinderConfirm,SButton).ButtonStyle(&ForestTheme::PrimaryButton()).ContentPadding(FMargin(16,10)).OnClicked_Lambda([this](){FinderConfirmClicked();return FReply::Handled();})
     [SNew(STextBlock).Text(FText::FromString(TEXT("交付 1 个并启程"))).Font(ForestUI::FinderFont(12)).ColorAndOpacity(ForestTheme::OnMoss).AutoWrapText(true)]]];
 TSharedRef<SWidget> Travel=SNew(SVerticalBox)
  +SVerticalBox::Slot().AutoHeight()
   [SNew(SHorizontalBox)
    +SHorizontalBox::Slot().AutoWidth().Padding(0,0,14,0)[ForestUI::Portrait(52,60)]
    +SHorizontalBox::Slot().FillWidth(1)[SNew(SVerticalBox)
     +SVerticalBox::Slot().AutoHeight()[Text(TEXT("小物正在为你带路"),10)]
     +SVerticalBox::Slot().AutoHeight().Padding(0,5)[SAssignNew(TravelTarget,STextBlock).Font(ForestUI::FinderFont(19)).ColorAndOpacity(ForestUI::Ink).AutoWrapText(true)]]]
  +SVerticalBox::Slot().FillHeight(1).Padding(0,14)
   [SNew(SScrollBox)+SScrollBox::Slot()[SAssignNew(TravelStory,STextBlock).Font(ForestUI::FinderFont(16)).ColorAndOpacity(ForestUI::Ink).AutoWrapText(true)]]
  +SVerticalBox::Slot().AutoHeight()[SAssignNew(TravelFee,STextBlock).Font(ForestUI::FinderFont(10)).ColorAndOpacity(ForestUI::Muted).AutoWrapText(true)]
  +SVerticalBox::Slot().AutoHeight().Padding(0,4,0,0)[Text(TEXT("WASD / Esc　停止跟随"),10)];
 TSharedRef<SWidget> DialoguePanel=SNew(SVerticalBox)
  +SVerticalBox::Slot().FillHeight(1).Padding(0,12,0,10)
   [SAssignNew(Scroll,SScrollBox)+SScrollBox::Slot()[SAssignNew(Message,STextBlock).Font(ForestUI::Font(15)).ColorAndOpacity(ForestUI::Ink).AutoWrapText(true)]]
  +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,10)
   [SAssignNew(Input,SEditableTextBox).Style(&ForestTheme::InputStyle()).HintText(FText::FromString(TEXT("写下想说的话……"))).Font(ForestUI::Font(14))
    .OnTextCommitted_Lambda([this](const FText&,ETextCommit::Type Method){if(Method==ETextCommit::OnEnter)SendClicked();})]
  +SVerticalBox::Slot().AutoHeight()
   [SNew(SHorizontalBox)
    +SHorizontalBox::Slot().AutoWidth().Padding(0,0,10,0)
     [SAssignNew(Send,SButton).ButtonStyle(&ForestTheme::PrimaryButton()).ContentPadding(FMargin(20,9)).OnClicked_Lambda([this](){SendClicked();return FReply::Handled();})
      [SNew(STextBlock).Text(FText::FromString(TEXT("发送　Enter"))).Font(ForestUI::Font(12)).ColorAndOpacity(ForestTheme::OnMoss)]]
    +SHorizontalBox::Slot().AutoWidth()[Secondary(TEXT("结束交谈　Esc"),FOnClicked::CreateLambda([this](){CloseClicked();return FReply::Handled();}))]];
 TSharedRef<SOverlay> Built=SNew(SOverlay)
  +SOverlay::Slot().Padding(FMargin(24))
   [SAssignNew(AdaptivePanel,SBox).HAlign(HAlign_Center).VAlign(VAlign_Bottom)
    [SAssignNew(PanelFrame,SBox).WidthOverride(760)
     [SNew(SForestPaperBorder).Padding(FMargin(32,30,32,42))
      [SNew(SVerticalBox)
       +SVerticalBox::Slot().AutoHeight()[SAssignNew(Title,STextBlock).Font_Lambda([this](){return bFinderVisible?ForestUI::FinderFont(21):ForestUI::Font(21);}).ColorAndOpacity(ForestUI::Ink).AutoWrapText(true)]
       +SVerticalBox::Slot().AutoHeight().Padding(0,8,0,0)[SAssignNew(Subtitle,STextBlock).Font(ForestUI::FinderFont(11)).ColorAndOpacity(ForestUI::Muted).AutoWrapText(true)]
       +SVerticalBox::Slot().FillHeight(1)[SAssignNew(Switcher,SWidgetSwitcher)
        +SWidgetSwitcher::Slot()[SNullWidget::NullWidget]
        +SWidgetSwitcher::Slot()[DialoguePanel]
        +SWidgetSwitcher::Slot()[Selection]
        +SWidgetSwitcher::Slot()[Confirmation]
        +SWidgetSwitcher::Slot()[Travel]]
       +SVerticalBox::Slot().AutoHeight().Padding(0,12,0,0)[SAssignNew(Prompt,STextBlock).Font_Lambda([this](){return bFinderVisible?ForestUI::FinderFont(10):ForestUI::Font(10);}).ColorAndOpacity(ForestUI::Muted).AutoWrapText(true)]]]]];
 Root=Built;SetPanel(EPanel::Notice);return Built;
}
void UForestNPCInteractionWidget::NativeTick(const FGeometry& Geometry,float DeltaTime)
{
 Super::NativeTick(Geometry,DeltaTime);if(!PanelFrame.IsValid())return;
 const FVector2D Size=Geometry.GetLocalSize();if(Size.X<=0 || Size.Y<=0)return;
 const float Limit=bFinderVisible?1100.f:Panel==EPanel::Travelling?1000.f:760.f;
 LayoutWidth=FMath::Max(1.f,FMath::Min(Limit,static_cast<float>(Size.X)-48.f));PanelFrame->SetWidthOverride(LayoutWidth);
 const float Height=Panel==EPanel::Travelling?(LayoutWidth<700.f?600.f:400.f):bFinderVisible?640.f:540.f;
 PanelFrame->SetHeightOverride(Panel==EPanel::Notice?FOptionalSize():FOptionalSize(FMath::Max(1.f,FMath::Min(Height,static_cast<float>(Size.Y)-48.f))));
}
void UForestNPCInteractionWidget::ShowMessage(const FString& Name,const FString&,const FString& Hint)
{
 if(!Title.IsValid())return;SetPanel(EPanel::Notice);
 Title->SetText(FText::FromString(Name));Title->SetVisibility(Name.IsEmpty()?EVisibility::Collapsed:EVisibility::HitTestInvisible);
 Prompt->SetText(FText::FromString(Hint));Root->SetVisibility(Hint.IsEmpty()?EVisibility::Collapsed:EVisibility::HitTestInvisible);
}
void UForestNPCInteractionWidget::ShowConversation(const FString& Name,const FString& Body,const FString& Status,bool Waiting)
{
 if(!Title.IsValid())return;SetPanel(EPanel::Conversation);
 Title->SetText(FText::FromString(Name));Message->SetText(FText::FromString(Body.IsEmpty()?TEXT("你叫住了对方。可以开始交谈。"):Body));Prompt->SetText(FText::FromString(Status));Input->SetEnabled(!Waiting);Send->SetEnabled(!Waiting);Scroll->ScrollToEnd();
}
void UForestNPCInteractionWidget::ShowFinderSelection()
{
 if(!Title.IsValid())return;SetPanel(EPanel::Selecting);SelectedFinderPersona=NAME_None;SelectedFee.Reset();
 Title->SetText(FText::FromString(TEXT("想去见谁？")));Subtitle->SetText(FText::FromString(TEXT("选一位同伴，再交出一件随身小物，让它为你带路。")));
 Prompt->SetText(FText::FromString(TEXT("请先选择一位同伴。还未消耗物品。")));SetKeyboardFocus();
}
void UForestNPCInteractionWidget::ShowFinderConfirmation(const FString& CharacterName,const TArray<FString>& Items)
{
 if(!Title.IsValid())return;SetPanel(EPanel::Confirming);SelectedFee.Reset();FinderGrid->ClearChildren();
 Title->SetText(FText::FromString(TEXT("让一件小物为你带路")));Subtitle->SetText(FText::FromString(TEXT("从背包中选一件。确认启程后，将消耗其中 1 个。")));
 FinderTargetName->SetText(FText::FromString(CharacterName+(CharacterName==TEXT("Natta")?TEXT(" · 铁匠"):CharacterName==TEXT("Fawnia")?TEXT(" · 传信人"):TEXT(" · 小说家"))));ShowSelectedFee(TEXT(""));
 for(const FString& Key:Items)
 {
  const auto View=ForestUI::ReadItem(GetWorld(),Key);
  FinderGrid->AddSlot()[SNew(SBox).WidthOverride_Lambda([this](){return FMath::Max(1.f,FMath::Min(190.f,LayoutWidth-84.f));}).HeightOverride(180)
   [SNew(SButton).ButtonStyle(&ForestTheme::SecondaryButton()).ContentPadding(FMargin(8,7))
    .ButtonColorAndOpacity_Lambda([this,Key](){return SelectedFee==Key?ForestTheme::SelectedPaper:FLinearColor::White;})
    .OnClicked_Lambda([this,Key](){if(Manager.IsValid())Manager->SelectFeeItem(Key);return FReply::Handled();})
    [SNew(SVerticalBox)
     +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[SNew(SBox).WidthOverride(54).HeightOverride(42)[SNew(SImage).Image(ForestTheme::ElementIcon(View.Icon))]]
     +SVerticalBox::Slot().FillHeight(1).VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(View.Name)).Font(ForestUI::FinderFont(12)).ColorAndOpacity(ForestUI::Ink).AutoWrapText(true).Justification(ETextJustify::Center)]
     +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[SNew(STextBlock).Text(FText::FromString(View.Attribute+(View.Count>0?FString::Printf(TEXT(" · 持有%d个"),View.Count):TEXT(" · 可交付1个")))).Font(ForestUI::FinderFont(9)).ColorAndOpacity(ForestUI::Muted).AutoWrapText(true)]]]];
 }
 for(int32 I=Items.Num();I<6;++I)FinderGrid->AddSlot()[SNew(SBox).WidthOverride_Lambda([this](){return FMath::Max(1.f,FMath::Min(190.f,LayoutWidth-84.f));}).HeightOverride(180)[SNew(SBorder).BorderImage(ForestUI::Fill()).BorderBackgroundColor(ForestTheme::Edge.CopyWithNewOpacity(.10f))]];
 if(Items.IsEmpty())FinderItemDescription->SetText(FText::FromString(TEXT("背包里还没有可交付的物品。采集一些小物后，再来寻找同伴吧。")));
 Prompt->SetText(FText::FromString(TEXT("返回不会扣除物品；启程前会检查路线。小物启程后会随着引路的魔法消散。")));SetKeyboardFocus();
}
void UForestNPCInteractionWidget::ShowSelectedFee(const FString& FeeItem)
{
 if(!FinderFee.IsValid())return;SelectedFee=FeeItem;FinderConfirm->SetEnabled(!FeeItem.IsEmpty());
 if(FeeItem.IsEmpty())
 {
  FinderItemName->SetText(FText::FromString(TEXT("选择一件小物")));FinderItemAttribute->SetText(FText::GetEmpty());
  FinderItemDescription->SetText(FText::FromString(TEXT("点击物品，查看描述与交付数量。")));FinderFee->SetText(FText::FromString(TEXT("尚未选择，不会扣除物品。")));return;
 }
 const auto View=ForestUI::ReadItem(GetWorld(),FeeItem);
 FinderItemName->SetText(FText::FromString(View.Name));FinderItemAttribute->SetText(FText::FromString(View.Attribute));FinderItemDescription->SetText(FText::FromString(View.Description));
 FinderFee->SetText(FText::FromString(FString::Printf(TEXT("本次交付：%s ×1"),*View.Name)));
}
void UForestNPCInteractionWidget::ShowTravel(const FString& CharacterName,const FString& FeeItem,const FString& Story)
{
 if(!TravelTarget.IsValid())return;
 if(Panel==EPanel::Travelling && LastTravelTarget==CharacterName && LastTravelFee==FeeItem && LastTravelStory==Story)return;
 LastTravelTarget=CharacterName;LastTravelFee=FeeItem;LastTravelStory=Story;SetPanel(EPanel::Travelling);
 TravelTarget->SetText(FText::FromString(FString::Printf(TEXT("去见 %s"),*CharacterName)));
 TravelStory->SetText(FText::FromString(Story.IsEmpty()?TEXT("小物轻轻一颤，正在为你辨认林间的方向……"):Story));
 TravelFee->SetText(FText::FromString(FString::Printf(TEXT("已交付　%s ×1"),*ForestUI::ReadItem(GetWorld(),FeeItem).Name)));
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
 if(Widget)Widget->ShowTravel(CharacterName(Target),SpentFeeItem,TravelNarrativeText);
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
