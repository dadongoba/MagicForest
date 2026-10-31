#include "ForestAlchemy.h"
#include "Widgets/Images/SImage.h"
#include "ForestUITheme.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "InputCoreTypes.h"
namespace BrewUI
{
 TSharedRef<STextBlock> Text(const FString& Value,int32 Size=12){return SNew(STextBlock).Text(FText::FromString(Value)).Font(ForestTheme::Font(FMath::Max(16,Size+4))).ColorAndOpacity(ForestTheme::Ink).AutoWrapText(true);}
 TSharedRef<SWidget> Button(const FString& Label,FOnClicked Click,bool Primary=false){return SNew(SButton).ButtonStyle(Primary?&ForestTheme::PrimaryButton():&ForestTheme::SecondaryButton()).ContentPadding(FMargin(12,10)).OnClicked(Click)[SNew(STextBlock).Text(FText::FromString(Label)).Font(ForestTheme::Font(16)).ColorAndOpacity(Primary?ForestTheme::OnMoss:ForestTheme::Ink).AutoWrapText(true)];}
}
TSharedRef<SWidget> UForestArcanaNotice::RebuildWidget()
{
 return SNew(SOverlay)+SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(FMargin(24,0,24,190))
  [SNew(SBox).MaxDesiredWidth(660)[SNew(SForestPaperBorder).Padding(FMargin(28,18))[SAssignNew(Text,STextBlock).Font(ForestTheme::Font(13)).ColorAndOpacity(ForestTheme::Ink).AutoWrapText(true)]]];
}
TSharedRef<SWidget> UForestBrewWidget::RebuildWidget()
{
 SetIsFocusable(true);return SNew(SOverlay)+SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
  [SAssignNew(Frame,SBox).WidthOverride(1180).HeightOverride(740)
   [SAssignNew(Paper,SForestPaperBorder).Padding(FMargin(32,40,32,44))[SAssignNew(Body,SVerticalBox)]]];
}
void UForestBrewWidget::NativeTick(const FGeometry& Geometry,float DeltaTime)
{
 Super::NativeTick(Geometry,DeltaTime);
 if(!Frame.IsValid())return;
 // Geometry is already in Slate units after DPI: never mix it with physical display pixels.
 const FVector2D Available=Geometry.GetLocalSize();if(Available.X<=0 || Available.Y<=0)return;
 const float Width=FMath::Max(1.f,FMath::Min(1180.f,static_cast<float>(Available.X)-32.f));
 const float Height=FMath::Max(1.f,FMath::Min(740.f,static_cast<float>(Available.Y)-32.f));
 Frame->SetWidthOverride(Width);Frame->SetHeightOverride(Height);
 const bool Compact=Width<900.f;
 Paper->SetPadding(Compact?FMargin(18,32,18,36):FMargin(32,40,32,44));
 if(bCompact!=Compact){bCompact=Compact;Refresh();}
}

void UForestBrewWidget::Refresh()
{
 if(!Body.IsValid() || !Owner.IsValid())return;Body->ClearChildren();auto* A=Owner.Get();int32 Cost=0;for(const auto& C:A->Sequence)Cost+=C.Card.Cost;
 if(const auto* R=A->PendingResult())
 {
  const auto& E=R->Result;
  Body->AddSlot().AutoHeight()[BrewUI::Text(FString::Printf(TEXT("次日破晓 · 第 %d 天的合成结果"),R->Day),23)];
  Body->AddSlot().AutoHeight().Padding(0,8,0,20)[BrewUI::Text(R->Customer+TEXT(" · 结局与世界余波"),13)];
  auto Scores=SNew(SVerticalBox);
  Scores->AddSlot().AutoHeight()[BrewUI::Text(TEXT("偏离与药性"),18)];
  Scores->AddSlot().AutoHeight().Padding(0,16)[BrewUI::Text(FString::Printf(TEXT("目标　火 %d　风 %d　水 %d\n成品　火 %d　风 %d　水 %d\n\n总剂量　%.0f / 24\n灵验　%.2f\n偏离　%.2f\n方向吻合　%.1f%%\n剂量比　%.0f%%\n\n%s\n\n%s"),E.Target.Fire,E.Target.Wind,E.Target.Water,E.Actual.Fire,E.Actual.Wind,E.Actual.Water,E.SumAbsolute,E.Projection,E.PerpendicularCost,E.Cosine*100,E.DoseRatio*100,*E.Verdict,*E.Reason),13)];
  auto Story=SNew(SVerticalBox);
  Story->AddSlot().AutoHeight()[BrewUI::Text(TEXT("推演 · 使用现场与森林余波"),18)];
  Story->AddSlot().AutoHeight().Padding(0,12)[BrewUI::Text(A->bNarrativePending?TEXT("正在依据本次成品推演……"):R->bNarrativeFallback?TEXT("推演暂不可用 · 以下为本地结算记录"):TEXT("本次成品的推演记录"),10)];
  Story->AddSlot().AutoHeight().Padding(0,10)[BrewUI::Text(A->bNarrativePending?TEXT("瓶口的封蜡渐渐冷却。林间尚未传回消息。愿望能否实现，取决于这一锅里真正留下的药性。"):R->Narrative,16)];
  if(bCompact)Body->AddSlot().FillHeight(1)[SNew(SScrollBox).ScrollBarAlwaysVisible(true)
   +SScrollBox::Slot()[Scores]+SScrollBox::Slot().Padding(0,22,0,0)[Story]];
  else Body->AddSlot().FillHeight(1)[SNew(SHorizontalBox)
   +SHorizontalBox::Slot().FillWidth(.34f).Padding(0,0,32,0)[SNew(SScrollBox)+SScrollBox::Slot()[Scores]]
   +SHorizontalBox::Slot().FillWidth(.66f)[SNew(SScrollBox)+SScrollBox::Slot()[Story]]];
  Body->AddSlot().AutoHeight().Padding(0,14)[BrewUI::Text(A->Status,10)];
  Body->AddSlot().AutoHeight()[SNew(SBox).HAlign(HAlign_Right).IsEnabled(!A->bNarrativePending)
   [BrewUI::Button(TEXT("承接新局 · 开始下一天"),FOnClicked::CreateLambda([this](){if(Owner.IsValid())Owner->FinishResult();return FReply::Handled();}),true)]];
  return;
 }
 auto Draft=SNew(SVerticalBox);
 FAlchemyCommissionDefinition Comm;const bool HasCommission=A->CurrentCommission(Comm);
 Body->AddSlot().AutoHeight()[BrewUI::Text(FString::Printf(TEXT("夜间合成台 · 第 %d 天                 今晚额度 %d / 5"),A->GetDay(),5-Cost),bCompact?16:21)];
 Draft->AddSlot().AutoHeight().Padding(0,8)[BrewUI::Text(TEXT("元素小物相加 · 大牌按顺序变换药性 · 副锅独立调配 · 点击槽位可移除或调整顺序"),10)];
 auto Customers=SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(8,6));
 for(const auto& C:A->Commissions())
 {
  const FString Name=C.CharacterId==TEXT("blacksmith")?TEXT("Natta · 铁匠"):C.CharacterId==TEXT("operator")?TEXT("Fawnia · 传信人"):TEXT("森林女巫 · 小说家");
  Customers->AddSlot()[BrewUI::Button(Name,FOnClicked::CreateLambda([this,C](){if(Owner.IsValid())Owner->ChooseCommission(C.OrderIndex);return FReply::Handled();}),HasCommission && C.OrderIndex==Comm.OrderIndex)];
 }
 Draft->AddSlot().AutoHeight().Padding(0,5)[Customers];
 auto Slots=SNew(SHorizontalBox);
 for(int32 I=0;I<(HasCommission?Comm.SlotCount:3);++I)
 {
  const bool Occupied=A->Sequence.IsValidIndex(I);const FString Label=Occupied?FString::Printf(TEXT("%d\n%s\n%s\n额度 %d"),I+1,*A->Sequence[I].Name,*A->Sequence[I].Description,A->Sequence[I].Card.Cost):FString::Printf(TEXT("%d\n空槽"),I+1);
  Slots->AddSlot().AutoWidth().Padding(0,0,8,0)[SNew(SBox).WidthOverride(154).HeightOverride(132)
   [SNew(SButton).ButtonStyle(&ForestTheme::SecondaryButton()).ButtonColorAndOpacity(A->SelectedSlot==I?ForestTheme::SelectedPaper:FLinearColor::White)
    .OnClicked_Lambda([this,I](){if(Owner.IsValid()){Owner->SelectedSlot=I;Owner->RefreshUI();}return FReply::Handled();})
    [SNew(SScrollBox)+SScrollBox::Slot()[BrewUI::Text(Label,10)]]]];
 }
 Draft->AddSlot().AutoHeight().Padding(0,8)[SNew(SBox).HeightOverride(150)[SNew(SScrollBox).Orientation(Orient_Horizontal)+SScrollBox::Slot()[Slots]]];
 auto Tools=SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6,6));
 Tools->AddSlot()[BrewUI::Button(TEXT("← 前移"),FOnClicked::CreateLambda([this](){if(Owner.IsValid())Owner->MoveSlot(-1);return FReply::Handled();}))];
 Tools->AddSlot()[BrewUI::Button(TEXT("后移 →"),FOnClicked::CreateLambda([this](){if(Owner.IsValid())Owner->MoveSlot(1);return FReply::Handled();}))];
 Tools->AddSlot()[BrewUI::Button(TEXT("移除所选"),FOnClicked::CreateLambda([this](){if(Owner.IsValid())Owner->RemoveSlot();return FReply::Handled();}))];
 Tools->AddSlot()[BrewUI::Button(TEXT("清空"),FOnClicked::CreateLambda([this](){if(Owner.IsValid())Owner->ClearSequence();return FReply::Handled();}))];Draft->AddSlot().AutoHeight()[Tools];
 auto Materials=SNew(SVerticalBox);auto Majors=SNew(SVerticalBox);auto Result=SNew(SVerticalBox);
 Materials->AddSlot().AutoHeight().Padding(0,0,0,8)[BrewUI::Text(TEXT("元素小物 · 来自背包"),16)];
 Majors->AddSlot().AutoHeight().Padding(0,0,0,8)[BrewUI::Text(TEXT("今日大牌 · 来自药瓶"),16)];
 const auto Choices=A->Choices();bool HasPips=false,HasMajors=false;
 for(const auto& C:Choices)
 {
  if(C.Key.StartsWith(TEXT("court:")))continue;const bool IsPip=C.Card.Type==EAlchemyCardType::Pip;HasPips|=IsPip;HasMajors|=!IsPip;
  auto ChoiceButton=BrewUI::Button(C.Name+FString::Printf(TEXT("   额度%d\n%s"),C.Card.Cost,*C.Description),FOnClicked::CreateLambda([this,C](){if(Owner.IsValid())Owner->AddChoice(C);return FReply::Handled();}));
  if(IsPip)Materials->AddSlot().AutoHeight().Padding(0,0,0,7)
   [SNew(SHorizontalBox)+SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,8,0)
    [SNew(SBox).WidthOverride(40).HeightOverride(40)[SNew(SImage).Image(ForestTheme::ElementIcon(C.Card.Element==EAlchemyElement::Fire?0:C.Card.Element==EAlchemyElement::Water?2:1))]]
    +SHorizontalBox::Slot().FillWidth(1)[ChoiceButton]];
  else Majors->AddSlot().AutoHeight().Padding(0,0,0,7)[ChoiceButton];
 }
 if(!HasPips)Materials->AddSlot().AutoHeight()[BrewUI::Text(TEXT("背包中还没有可用于炼药的元素小物。"))];
 if(!HasMajors)Majors->AddSlot().AutoHeight()[BrewUI::Text(TEXT("今日还没有大牌。白天拿起小屋药瓶即可抽取。"))];
 Majors->AddSlot().AutoHeight().Padding(0,14,0,8)[BrewUI::Text(TEXT("通用修饰"),14)];
 for(const auto& C:Choices)if(C.Key.StartsWith(TEXT("court:")))Majors->AddSlot().AutoHeight().Padding(0,0,0,5)[BrewUI::Button(C.Name+FString::Printf(TEXT(" · 额度%d\n%s"),C.Card.Cost,*C.Description),FOnClicked::CreateLambda([this,C](){if(Owner.IsValid())Owner->AddChoice(C);return FReply::Handled();}))];
 Result->AddSlot().AutoHeight()[BrewUI::Text(TEXT("今晚的委托"),16)];
 if(HasCommission)Result->AddSlot().AutoHeight().Padding(0,10)[BrewUI::Text(Comm.Wish+TEXT("\n\n")+Comm.ElementRequest+FString::Printf(TEXT("\n槽位 %d · %s"),Comm.SlotCount,Comm.bMustUseAll?TEXT("指定用料需全部使用"):TEXT("可自行选料")),12)];
 else Result->AddSlot().AutoHeight().Padding(0,10)[BrewUI::Text(TEXT("请先选择今晚的委托对象。"))];
 if(bCompact)
 {
  Draft->AddSlot().AutoHeight().Padding(0,14)[Result];
  Draft->AddSlot().AutoHeight().Padding(0,8)[Materials];
  Draft->AddSlot().AutoHeight().Padding(0,14)[Majors];
  Body->AddSlot().FillHeight(1).Padding(0,8)[SNew(SScrollBox).ScrollBarAlwaysVisible(true)+SScrollBox::Slot()[Draft]];
 }
 else
 {
  // The whole draft scrolls when the viewport is short; the action footer stays outside.
  Draft->AddSlot().AutoHeight().Padding(0,14)[SNew(SHorizontalBox)
   +SHorizontalBox::Slot().FillWidth(.32f).Padding(0,0,16,0)[Materials]
   +SHorizontalBox::Slot().FillWidth(.35f).Padding(0,0,16,0)[Majors]
   +SHorizontalBox::Slot().FillWidth(.33f)[Result]];
  Body->AddSlot().FillHeight(1).Padding(0,8)[SNew(SScrollBox).ScrollBarAlwaysVisible(true)+SScrollBox::Slot()[Draft]];
 }
 Body->AddSlot().AutoHeight().Padding(0,0,0,8)[BrewUI::Text(A->Status,10)];
 Body->AddSlot().AutoHeight()
  [SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(8,6))
   +SWrapBox::Slot()[BrewUI::Button(TEXT("收起 · Esc"),FOnClicked::CreateLambda([this](){if(Owner.IsValid())Owner->CloseWorkbench();return FReply::Handled();}))]
   +SWrapBox::Slot()[BrewUI::Button(A->bConfirming?TEXT("确认封瓶"):TEXT("封瓶交付"),FOnClicked::CreateLambda([this](){if(Owner.IsValid())Owner->ConfirmBrew();return FReply::Handled();}),true)]];
}
FReply UForestBrewWidget::NativeOnPreviewKeyDown(const FGeometry& G,const FKeyEvent& E){if(E.GetKey()==EKeys::Escape){if(Owner.IsValid() && !Owner->PendingResult())Owner->CloseWorkbench();return FReply::Handled();}return Super::NativeOnPreviewKeyDown(G,E);}
