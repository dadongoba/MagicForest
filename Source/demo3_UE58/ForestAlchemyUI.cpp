#include "ForestAlchemy.h"
#include "ForestUITheme.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "InputCoreTypes.h"
namespace BrewUI
{
 TSharedRef<STextBlock> Text(const FString& Value,int32 Size=12){return SNew(STextBlock).Text(FText::FromString(Value)).Font(ForestTheme::Font(Size)).ColorAndOpacity(ForestTheme::Ink).AutoWrapText(true);}
 TSharedRef<SWidget> Button(const FString& Label,FOnClicked Click,bool Primary=false){return SNew(SButton).ButtonStyle(Primary?&ForestTheme::PrimaryButton():&ForestTheme::SecondaryButton()).ContentPadding(FMargin(12,7)).OnClicked(Click)[SNew(STextBlock).Text(FText::FromString(Label)).Font(ForestTheme::Font(11)).ColorAndOpacity(Primary?ForestTheme::OnMoss:ForestTheme::Ink).AutoWrapText(true)];}
}
TSharedRef<SWidget> UForestArcanaNotice::RebuildWidget()
{
 return SNew(SOverlay)+SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(FMargin(24,0,24,190))
  [SNew(SBox).MaxDesiredWidth(660)[SNew(SForestPaperBorder).Padding(FMargin(28,18))[SAssignNew(Text,STextBlock).Font(ForestTheme::Font(13)).ColorAndOpacity(ForestTheme::Ink).AutoWrapText(true)]]];
}
TSharedRef<SWidget> UForestBrewWidget::RebuildWidget()
{
 SetIsFocusable(true);return SNew(SOverlay)+SOverlay::Slot().Padding(FMargin(24))
  [SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
   [SNew(SBox).WidthOverride(1180).HeightOverride(740)
    [SNew(SForestPaperBorder).Padding(FMargin(42,32,42,52))[SAssignNew(Body,SVerticalBox)]]]];
}
void UForestBrewWidget::Refresh()
{
 if(!Body.IsValid() || !Owner.IsValid())return;Body->ClearChildren();auto* A=Owner.Get();int32 Cost=0;for(const auto& C:A->Sequence)Cost+=C.Card.Cost;
 FAlchemyCommissionDefinition Comm;const bool HasCommission=A->CurrentCommission(Comm);
 Body->AddSlot().AutoHeight()[BrewUI::Text(FString::Printf(TEXT("夜间合成台 · 第 %d 天                 今晚额度 %d / 5"),A->GetDay(),5-Cost),21)];
 Body->AddSlot().AutoHeight().Padding(0,8)[BrewUI::Text(TEXT("元素小物相加 · 大牌按顺序变换药性 · 副锅独立调配 · 点击槽位可移除或调整顺序"),10)];
 auto Customers=SNew(SHorizontalBox);
 for(const auto& C:A->Commissions())
 {
  const FString Name=C.CharacterId==TEXT("blacksmith")?TEXT("Natta · 铁匠"):C.CharacterId==TEXT("operator")?TEXT("Fawnia · 传信人"):TEXT("森林女巫 · 小说家");
  Customers->AddSlot().AutoWidth().Padding(0,0,10,0)[BrewUI::Button(Name,FOnClicked::CreateLambda([this,C](){if(Owner.IsValid())Owner->ChooseCommission(C.OrderIndex);return FReply::Handled();}),HasCommission && C.OrderIndex==Comm.OrderIndex)];
 }
 Body->AddSlot().AutoHeight().Padding(0,5)[Customers];
 auto Slots=SNew(SHorizontalBox);
 for(int32 I=0;I<(HasCommission?Comm.SlotCount:3);++I)
 {
  const bool Occupied=A->Sequence.IsValidIndex(I);const FString Label=Occupied?FString::Printf(TEXT("%d\n%s\n%s\n额度 %d"),I+1,*A->Sequence[I].Name,*A->Sequence[I].Description,A->Sequence[I].Card.Cost):FString::Printf(TEXT("%d\n空槽"),I+1);
  Slots->AddSlot().AutoWidth().Padding(0,0,8,0)[SNew(SBox).WidthOverride(154).HeightOverride(132)
   [SNew(SButton).ButtonStyle(&ForestTheme::SecondaryButton()).ButtonColorAndOpacity(A->SelectedSlot==I?ForestTheme::SelectedPaper:FLinearColor::White)
    .OnClicked_Lambda([this,I](){if(Owner.IsValid()){Owner->SelectedSlot=I;Owner->RefreshUI();}return FReply::Handled();})
    [SNew(SScrollBox)+SScrollBox::Slot()[BrewUI::Text(Label,10)]]]];
 }
 Body->AddSlot().AutoHeight().Padding(0,8)[SNew(SBox).HeightOverride(150)[SNew(SScrollBox).Orientation(Orient_Horizontal)+SScrollBox::Slot()[Slots]]];
 auto Tools=SNew(SHorizontalBox);
 Tools->AddSlot().AutoWidth().Padding(0,0,6,0)[BrewUI::Button(TEXT("← 前移"),FOnClicked::CreateLambda([this](){if(Owner.IsValid())Owner->MoveSlot(-1);return FReply::Handled();}))];
 Tools->AddSlot().AutoWidth().Padding(0,0,6,0)[BrewUI::Button(TEXT("后移 →"),FOnClicked::CreateLambda([this](){if(Owner.IsValid())Owner->MoveSlot(1);return FReply::Handled();}))];
 Tools->AddSlot().AutoWidth().Padding(0,0,6,0)[BrewUI::Button(TEXT("移除所选"),FOnClicked::CreateLambda([this](){if(Owner.IsValid())Owner->RemoveSlot();return FReply::Handled();}))];
 Tools->AddSlot().AutoWidth()[BrewUI::Button(TEXT("清空"),FOnClicked::CreateLambda([this](){if(Owner.IsValid())Owner->ClearSequence();return FReply::Handled();}))];Body->AddSlot().AutoHeight()[Tools];
 auto Materials=SNew(SVerticalBox);auto Majors=SNew(SVerticalBox);auto Result=SNew(SVerticalBox);
 Materials->AddSlot().AutoHeight().Padding(0,0,0,8)[BrewUI::Text(TEXT("元素小物 · 来自背包"),16)];
 Majors->AddSlot().AutoHeight().Padding(0,0,0,8)[BrewUI::Text(TEXT("今日大牌 · 来自药瓶"),16)];
 const auto Choices=A->Choices();bool HasPips=false,HasMajors=false;
 for(const auto& C:Choices)
 {
  if(C.Key.StartsWith(TEXT("court:")))continue;const bool IsPip=C.Card.Type==EAlchemyCardType::Pip;HasPips|=IsPip;HasMajors|=!IsPip;
  (IsPip?Materials:Majors)->AddSlot().AutoHeight().Padding(0,0,0,7)
   [BrewUI::Button(C.Name+FString::Printf(TEXT("   额度%d\n%s"),C.Card.Cost,*C.Description),FOnClicked::CreateLambda([this,C](){if(Owner.IsValid())Owner->AddChoice(C);return FReply::Handled();}))];
 }
 if(!HasPips)Materials->AddSlot().AutoHeight()[BrewUI::Text(TEXT("背包中还没有可用于炼药的元素小物。"))];
 if(!HasMajors)Majors->AddSlot().AutoHeight()[BrewUI::Text(TEXT("今日还没有大牌。白天拿起小屋药瓶即可抽取。"))];
 Majors->AddSlot().AutoHeight().Padding(0,14,0,8)[BrewUI::Text(TEXT("通用修饰"),14)];
 for(const auto& C:Choices)if(C.Key.StartsWith(TEXT("court:")))Majors->AddSlot().AutoHeight().Padding(0,0,0,5)[BrewUI::Button(C.Name+FString::Printf(TEXT(" · 额度%d\n%s"),C.Card.Cost,*C.Description),FOnClicked::CreateLambda([this,C](){if(Owner.IsValid())Owner->AddChoice(C);return FReply::Handled();}))];
 Result->AddSlot().AutoHeight()[BrewUI::Text(TEXT("委托与试算"),16)];
 if(HasCommission)Result->AddSlot().AutoHeight().Padding(0,10)[BrewUI::Text(Comm.Wish+TEXT("\n\n")+Comm.ElementRequest+FString::Printf(TEXT("\n槽位 %d · %s"),Comm.SlotCount,Comm.bMustUseAll?TEXT("指定用料需全部使用"):TEXT("可自行选料")),12)];
 else Result->AddSlot().AutoHeight().Padding(0,10)[BrewUI::Text(TEXT("请先选择今晚的委托对象。"))];
 FAlchemyEvaluationResult E;if(A->Evaluate(E))Result->AddSlot().AutoHeight().Padding(0,10)
  [BrewUI::Text(FString::Printf(TEXT("当前药性\n火 %d　风 %d　水 %d\n\n总剂量 %.0f / 24\n灵验 %.2f　偏离 %.2f\n剂量比 %.0f%%\n\n%s\n%s"),E.Actual.Fire,E.Actual.Wind,E.Actual.Water,E.SumAbsolute,E.Projection,E.PerpendicularCost,E.DoseRatio*100,*E.Verdict,*E.Reason),12)];
 Body->AddSlot().FillHeight(1).Padding(0,14)
  [SNew(SHorizontalBox)
   +SHorizontalBox::Slot().FillWidth(.32f).Padding(0,0,16,0)[SNew(SScrollBox)+SScrollBox::Slot()[Materials]]
   +SHorizontalBox::Slot().FillWidth(.35f).Padding(0,0,16,0)[SNew(SScrollBox)+SScrollBox::Slot()[Majors]]
   +SHorizontalBox::Slot().FillWidth(.33f)[SNew(SScrollBox)+SScrollBox::Slot()[Result]]];
 Body->AddSlot().AutoHeight().Padding(0,0,0,8)[BrewUI::Text(A->Status,10)];
 Body->AddSlot().AutoHeight()
  [SNew(SHorizontalBox)
   +SHorizontalBox::Slot().AutoWidth().Padding(0,0,10,0)[BrewUI::Button(TEXT("收起 · Esc"),FOnClicked::CreateLambda([this](){if(Owner.IsValid())Owner->CloseWorkbench();return FReply::Handled();}))]
   +SHorizontalBox::Slot().AutoWidth()[BrewUI::Button(A->bConfirming?TEXT("确认封瓶并换天"):TEXT("封瓶交付"),FOnClicked::CreateLambda([this](){if(Owner.IsValid())Owner->ConfirmBrew();return FReply::Handled();}),true)]];
}
FReply UForestBrewWidget::NativeOnPreviewKeyDown(const FGeometry& G,const FKeyEvent& E){if(E.GetKey()==EKeys::Escape){if(Owner.IsValid())Owner->CloseWorkbench();return FReply::Handled();}return Super::NativeOnPreviewKeyDown(G,E);}
