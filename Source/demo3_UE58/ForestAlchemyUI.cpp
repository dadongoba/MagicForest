#include "ForestAlchemy.h"
#include "Widgets/Images/SImage.h"
#include "ForestUITheme.h"
#include "ForestAlchemyGuide.h"
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
  Body->AddSlot().AutoHeight()[BrewUI::Text(R->Day==3?FString(TEXT("第三夜 · 最终合成结果")):FString::Printf(TEXT("次日破晓 · 第 %d 天的合成结果"),R->Day),23)];
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
   [BrewUI::Button(!A->IsDayResolved(R->Day)?TEXT("记下结果 · 继续今日委托"):R->Day==3?TEXT("完成今日委托 · 查看结局"):TEXT("今日委托已结算 · 开始下一天"),FOnClicked::CreateLambda([this](){if(Owner.IsValid())Owner->FinishResult();return FReply::Handled();}),true)]];
  return;
 }
 if(A->IsCampaignCompleted())
 {
  Body->AddSlot().AutoHeight()[BrewUI::Text(TEXT("三日旅程 · 结局回顾"),23)];
  auto Records=SNew(SVerticalBox);const auto Brews=A->GetBrewRecords();
  for(int32 Day=1;Day<=3;++Day)
  {
   Records->AddSlot().AutoHeight().Padding(0,18,0,8)[BrewUI::Text(FString::Printf(TEXT("第 %d 天"),Day),18)];
   bool Found=false;for(const auto& R:Brews)if(R.Day==Day)
   {Found=true;Records->AddSlot().AutoHeight()[BrewUI::Text(R.Customer+TEXT(" · ")+R.Result.Verdict,16)];Records->AddSlot().AutoHeight().Padding(0,8)[BrewUI::Text(R.Narrative.IsEmpty()?TEXT("此记录没有保存推演文本。"):R.Narrative,16)];}
   if(!Found)Records->AddSlot().AutoHeight()[BrewUI::Text(TEXT("旧存档中没有这一天的交付记录。"))];
  }
  Body->AddSlot().FillHeight(1).Padding(0,12)[SNew(SScrollBox).ScrollBarAlwaysVisible(true)+SScrollBox::Slot()[Records]];
  Body->AddSlot().AutoHeight()[BrewUI::Button(TEXT("结束旅程 · 退出游戏"),FOnClicked::CreateLambda([this](){if(Owner.IsValid())Owner->ExitJourney();return FReply::Handled();}),true)];
  return;
 }
 auto Draft=SNew(SVerticalBox);
 FAlchemyCommissionDefinition Comm;const bool HasCommission=A->CurrentCommission(Comm);
 if(HasCommission && GuideCommission!=Comm.OrderIndex){GuideCommission=Comm.OrderIndex;GuidePage=1;bGuideSawMaterial=false;}
 else if(!HasCommission && GuideCommission!=0){GuideCommission=0;GuidePage=0;bGuideSawMaterial=false;}
 if(!bGuideSawMaterial && A->Sequence.ContainsByPredicate([](const auto& C){return C.Card.Type==EAlchemyCardType::Pip;})){bGuideSawMaterial=true;GuidePage=2;}
 if(A->bConfirming && !bGuideWasConfirming)GuidePage=3;
 bGuideWasConfirming=A->bConfirming;
 Body->AddSlot().AutoHeight()[BrewUI::Text(FString::Printf(TEXT("夜间合成台 · 第 %d 天                 今晚额度 %d / 5"),A->GetDay(),5-Cost),bCompact?16:21)];
 // Help lives inside the scrolling draft; the action footer remains outside it.
 auto Guide=SNew(SVerticalBox);
 Guide->AddSlot().AutoHeight()[BrewUI::Button(bGuideExpanded?TEXT("调配指引 · 收起"):TEXT("调配指引 · 重新查看"),FOnClicked::CreateLambda([this](){bGuideExpanded=!bGuideExpanded;Refresh();return FReply::Handled();}))];
 if(bGuideExpanded)
 {
  const FString Titles[]={TEXT("1 / 4 · 先想清楚为谁调药"),TEXT("2 / 4 · 用元素小物凑出药性"),TEXT("3 / 4 · 大牌改变调配的方式"),TEXT("4 / 4 · 检查后，再封瓶交付")};
  const FString Copy[]={
   TEXT("先选已接取的委托对象。回想白天对话里提到的用途：需要热量、气流还是水分？需要多强？元素图鉴说明各个档位能做到什么，委托的标准值要等交付后才揭示。切换委托会清空本次选料。"),
   TEXT("点击背包中的元素小物，依次放入槽位。物品名字不同，也可以贡献相同的点数：两件火3，在没有其他变换时会合成火6。火、风、水分别相加；不是把所有数字加成同一种元素。下方「当前试配」显示这套排列实际留下的药性。选料、移除都不会扣物品。"),
   TEXT("大牌通常作用于排在它前面、已经入锅的药性；恶魔影响后续投料，副锅则把后面一段单独调配。比如火3 → 皇帝 → 火1得到火7；火3 → 火1 → 皇帝得到火8。选中槽位后，可前移、后移或移除。每个项目占1槽，元素小物不花额度，大牌与通用修饰合计最多5点。大牌并非必用，查「今日大牌」可看当天各张的用法。"),
   TEXT("查看当前试配，判断它是否符合客人描述的用途。正数表示给予，负数表示取走；0表示不额外改变这一种性质。三种元素的绝对值合计超过24会使药剂报废。第一次点「封瓶交付」只弹出确认提示，第二次确认才消耗槽内物品与大牌。之后揭示标准、成品和偏离，并显示推演；前两夜确认结算后换天，第三夜进入结局。")};
  GuidePage=FMath::Clamp(GuidePage,0,3);
  Guide->AddSlot().AutoHeight().Padding(0,12,0,8)[BrewUI::Text(Titles[GuidePage],15)];
  Guide->AddSlot().AutoHeight().Padding(0,0,0,10)[BrewUI::Text(Copy[GuidePage],14)];
  auto Steps=SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6,5));
  if(GuidePage>0)Steps->AddSlot()[BrewUI::Button(TEXT("上一步"),FOnClicked::CreateLambda([this](){--GuidePage;Refresh();return FReply::Handled();}))];
  if(GuidePage<3)Steps->AddSlot()[BrewUI::Button(TEXT("下一步"),FOnClicked::CreateLambda([this](){++GuidePage;Refresh();return FReply::Handled();}))];
  Steps->AddSlot()[BrewUI::Button(TEXT("查阅今日大牌"),FOnClicked::CreateLambda([this](){if(Owner.IsValid())Owner->OpenMajorGuide();return FReply::Handled();}))];
  Guide->AddSlot().AutoHeight()[Steps];
 }
 Draft->AddSlot().AutoHeight().Padding(0,12)[SNew(SBorder).BorderImage(ForestTheme::Fill()).BorderBackgroundColor(ForestTheme::InsetPaper).Padding(16)[Guide]];
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
 Majors->AddSlot().AutoHeight().Padding(0,0,0,10)[BrewUI::Button(TEXT("翻书 · 查看今日大牌用法"),FOnClicked::CreateLambda([this](){if(Owner.IsValid())Owner->OpenMajorGuide();return FReply::Handled();}))];
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
 if(HasCommission)Result->AddSlot().AutoHeight().Padding(0,10)[BrewUI::Text(Comm.Wish+FString::Printf(TEXT("\n\n槽位 %d · %s\n委托标准在交付后揭示，请根据对话与图鉴判断。"),Comm.SlotCount,Comm.bMustUseAll?TEXT("指定用料需全部使用"):TEXT("可自行选料")),12)];
 else Result->AddSlot().AutoHeight().Padding(0,10)[BrewUI::Text(TEXT("只有已接取且尚未结算的委托会列在这里。与 NPC 交谈并答应承担委托，系统自动登记后，再回小屋的合成台调配。"))];
 FAlchemyVector Preview;int32 PreviewCost=0;
 Result->AddSlot().AutoHeight().Padding(0,18,0,8)[BrewUI::Text(TEXT("当前试配 · 尚未交付"),16)];
 if(A->PreviewDraft(Preview,PreviewCost))
 {
  Result->AddSlot().AutoHeight()[BrewUI::Text(ForestAlchemyGuide::VectorText(Preview.Fire,Preview.Wind,Preview.Water),16)];
  const int64 Total=FMath::Abs(static_cast<int64>(Preview.Fire))+FMath::Abs(static_cast<int64>(Preview.Wind))+FMath::Abs(static_cast<int64>(Preview.Water));
  Result->AddSlot().AutoHeight().Padding(0,8)[BrewUI::Text(FString::Printf(TEXT("槽位 %d / %d · 额度 %d / 5\n药性总量 %lld / 24\n%s"),A->Sequence.Num(),HasCommission?Comm.SlotCount:0,Cost,Total,Total>24?TEXT("总量超过24，封瓶会得到报废药剂，请调整。"):TEXT("这是你的配方结果，不代表客人的标准或评分。")),11)];
 }
 else Result->AddSlot().AutoHeight()[BrewUI::Text(TEXT("暂时无法读取试配结果。"))];
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
