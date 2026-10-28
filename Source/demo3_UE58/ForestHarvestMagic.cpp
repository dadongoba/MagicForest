#include "ForestHarvestMagic.h"
#include "ForestIsometricPlayer.h"
#include "ForestNPCInteraction.h"
#include "ForestUITheme.h"
#include "ForestAlchemy.h"
#include "Widgets/Layout/SScaleBox.h"
#include "DailyMushroomSubsystem.h"
#include "DialogueAIServiceSubsystem.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "TimerManager.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "UObject/UnrealType.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "InputCoreTypes.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace HarvestMagic
{
 FString BaseMushroom(FString Name)
 {
  int32 Position=Name.Find(TEXT("_LOD"));if(Position!=INDEX_NONE)Name=Name.Left(Position);
  Position=Name.Find(TEXT("_Alt"));if(Position!=INDEX_NONE)Name=Name.Left(Position);
  Name.ReplaceInline(TEXT("_Small_Group_"),TEXT("_Small_"));return Name;
 }
 const TCHAR* Slot=TEXT("ForestMagicItemLedger");
 const TCHAR* BookMesh=TEXT("/Game/MimicBookCharacter/Character/Mesh/SK_Mimic-book.SK_Mimic-book");
 UAnimSequence* Anim(const TCHAR* Name){return LoadObject<UAnimSequence>(nullptr,*(FString(TEXT("/Game/MimicBookCharacter/Animations/"))+Name+TEXT(".")+Name));}
 UNiagaraComponent* FX(UWorld* World,const TCHAR* Name,const FVector& Location,float Scale)
 {
  const FString Path=FString(TEXT("/Game/Biomes/PNB_Enchanted_Forest/FX/"))+Name+TEXT(".")+Name;
  auto* System=LoadObject<UNiagaraSystem>(nullptr,*Path);
  return System?UNiagaraFunctionLibrary::SpawnSystemAtLocation(World,System,Location,FRotator::ZeroRotator,FVector(Scale),true,true):nullptr;
 }
 using ForestTheme::Paper;using ForestTheme::HeaderPaper;using ForestTheme::InsetPaper;
 using ForestTheme::Edge;using ForestTheme::Ink;using ForestTheme::Muted;using ForestTheme::Moss;
 using ForestTheme::Fire;using ForestTheme::Wind;using ForestTheme::Water;
 using ForestTheme::Font;using ForestTheme::Fill;

}
void UForestHarvestRequest::Completed(bool Success,const FString& Text){if(Owner.IsValid())Owner->HandleResponse(this,Success,Text);}
FString UForestHarvestMagic::ElementText(EAlchemyElement E){return E==EAlchemyElement::Wind?TEXT("风"):E==EAlchemyElement::Water?TEXT("水"):TEXT("火");}
TSharedRef<SWidget> UForestFieldHUDWidget::RebuildWidget()
{
 return SNew(SOverlay)
 +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(FMargin(28,24))
 [SNew(SForestPaperBorder).BorderImage(HarvestMagic::Fill()).BorderBackgroundColor(HarvestMagic::Paper).Padding(FMargin(16,10))
  [SNew(SVerticalBox)
   +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("森林手札 · 采集记录"))).Font(HarvestMagic::Font(10,true)).ColorAndOpacity(HarvestMagic::Moss)]
   +SVerticalBox::Slot().AutoHeight().Padding(0,5,0,0)[SAssignNew(Counts,STextBlock).Font(HarvestMagic::Font(15,true)).ColorAndOpacity(HarvestMagic::Ink)]]]
 +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(FMargin(0,0,0,115))
 [SNew(SForestPaperBorder).BorderImage(HarvestMagic::Fill()).BorderBackgroundColor(HarvestMagic::Paper).Padding(FMargin(18,9)).Visibility_Lambda([this](){return Action.IsValid() && !Action->GetText().IsEmpty()?EVisibility::HitTestInvisible:EVisibility::Collapsed;})[SAssignNew(Action,STextBlock).Font(HarvestMagic::Font(14)).ColorAndOpacity(HarvestMagic::Ink)]];
}
void UForestHarvestMagic::RefreshFieldUI()
{
 if(!FieldHUD || !FieldHUD->Counts.IsValid())return;
 int32 Total=0,Kinds=0;for(const auto& Pair:ReadBag())if(Pair.Value>0){Total+=Pair.Value;++Kinds;}
 FieldHUD->Counts->SetText(FText::FromString(FString::Printf(TEXT("背包  %d 件  ·  %d 种"),Total,Kinds)));
 FString Action;
 TArray<UUserWidget*> Widgets;UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(),Widgets,UUserWidget::StaticClass(),false);
 for(auto* Widget:Widgets)
 {
  if(!Widget || !Widget->WidgetTree || Widget==Inventory || Widget==NoticeWidget || Widget==FieldHUD || Widget->IsA<UForestNPCInteractionWidget>())continue;
  Widget->WidgetTree->ForEachWidget([&](UWidget* Child)
  {
   if(auto* Text=Cast<UTextBlock>(Child))
   {
    const FString Value=Text->GetText().ToString();
    if(Value.Contains(TEXT("已采蘑菇"))){Text->SetVisibility(ESlateVisibility::Collapsed);return;}
    if(Value.Contains(TEXT("对准蘑菇"))){if(Text->GetVisibility()!=ESlateVisibility::Hidden)Action=Value.Replace(TEXT("\\n"),TEXT(" · "));Text->SetRenderOpacity(0.f);return;}
    if(Widget->GetClass()->GetName().Contains(TEXT("Cauldron"))){Text->SetColorAndOpacity(HarvestMagic::Ink);auto Font=Text->GetFont();Font.Size=16;Text->SetFont(Font);}
   }
   if(Widget->GetClass()->GetName().Contains(TEXT("Cauldron")))
   {
    if(auto* Border=Cast<UBorder>(Child))Border->SetBrushColor(HarvestMagic::Paper);
    if(auto* Button=Cast<UButton>(Child))Button->SetStyle(ForestTheme::SecondaryButton());
   }
  });
 }
 if(auto* Pawn=BagOwner.Get();Pawn && Pawn->FindComponentByClass<UForestIsometricPlayer>())Action=TEXT("WASD 移动  ·  E 交互  ·  B 背包  ·  V 找人  ·  M 地图  ·  右键拖动观察  ·  滚轮缩放");
 FieldHUD->Action->SetText(FText::FromString(Action));
}
TSharedRef<SWidget> UForestHarvestNoticeWidget::RebuildWidget()
{
 // Full-viewport anchor; only the paper has a preferred width. Height follows the text.
 return SNew(SOverlay)
 +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(FMargin(24))
 [SAssignNew(Frame,SBox).WidthOverride(960)
  [SNew(SForestPaperBorder).Padding(FMargin(32,28,32,36))
   [SNew(SVerticalBox)
    +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("魔法书捎来一件小物"))).Font(HarvestMagic::Font(16,true)).ColorAndOpacity(HarvestMagic::Moss).AutoWrapText(true)]
    +SVerticalBox::Slot().AutoHeight().Padding(0,12,0,0)[SAssignNew(Text,STextBlock).Font(HarvestMagic::Font(26)).ColorAndOpacity(HarvestMagic::Ink).AutoWrapText(true)]]]];
}
void UForestHarvestNoticeWidget::NativeTick(const FGeometry& Geometry,float DeltaTime)
{
 Super::NativeTick(Geometry,DeltaTime);
 const float Width=static_cast<float>(Geometry.GetLocalSize().X);
 if(Frame.IsValid() && Width>0)Frame->SetWidthOverride(FMath::Max(1.f,FMath::Min(960.f,Width-48.f)));
}
TSharedRef<SWidget> UForestMagicInventoryWidget::RebuildWidget()
{
 SetIsFocusable(true);
 return SNew(SOverlay)
 +SOverlay::Slot().Padding(FMargin(24))
 [SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
 [SNew(SBox).WidthOverride(940.f).HeightOverride(600.f)
  [SNew(SBorder).BorderImage(HarvestMagic::Fill()).BorderBackgroundColor(FLinearColor::Transparent).Padding(0.f)
   [SNew(SForestPaperBorder).BorderImage(HarvestMagic::Fill()).BorderBackgroundColor(HarvestMagic::Paper).Padding(0.f)
    [SNew(SVerticalBox)
     +SVerticalBox::Slot().AutoHeight()
     [SNew(SBorder).BorderImage(HarvestMagic::Fill()).BorderBackgroundColor(FLinearColor::Transparent).Padding(FMargin(42,32,42,6))
      [SNew(SHorizontalBox)
       +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,12,0)[SNew(STextBlock).Text(FText::FromString(TEXT(""))).Font(HarvestMagic::Font(19)).ColorAndOpacity(HarvestMagic::Moss)]
       +SHorizontalBox::Slot().FillWidth(1.f)[SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("随身物品"))).Font(HarvestMagic::Font(9,true)).ColorAndOpacity(HarvestMagic::Muted)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,2,0,0)[SNew(STextBlock).Text(FText::FromString(TEXT("背包"))).Font(HarvestMagic::Font(19)).ColorAndOpacity(HarvestMagic::Ink)]]
       +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(TEXT("B / ESC 关闭"))).Font(HarvestMagic::Font(10,true)).ColorAndOpacity(HarvestMagic::Muted)]]]
     +SVerticalBox::Slot().AutoHeight().Padding(FMargin(42,4,42,0))
      [SNew(SHorizontalBox)
       +SHorizontalBox::Slot().AutoWidth().Padding(0,0,10,0)[SNew(SButton).ButtonStyle(&ForestTheme::SecondaryButton()).OnClicked_Lambda([this](){bMajorsTab=false;Refresh();return FReply::Handled();})[SNew(STextBlock).Text(FText::FromString(TEXT("元素小物"))).Font(HarvestMagic::Font(12)).ColorAndOpacity(HarvestMagic::Ink)]]
       +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ButtonStyle(&ForestTheme::SecondaryButton()).OnClicked_Lambda([this](){bMajorsTab=true;Refresh();return FReply::Handled();})[SNew(STextBlock).Text_Lambda([this](){auto* A=GetWorld()->GetSubsystem<UForestAlchemy>();return FText::FromString(FString::Printf(TEXT("今日大牌 · 第%d天（%d）"),A?A->GetDay():1,A?A->GetTodayMajors().Num():0));}).Font(HarvestMagic::Font(12)).ColorAndOpacity(HarvestMagic::Ink)]]]
     +SVerticalBox::Slot().FillHeight(1.f).Padding(FMargin(42,12,42,4))
     [SNew(SHorizontalBox)
      +SHorizontalBox::Slot().FillWidth(.59f).Padding(0,0,12,0)
      [SNew(SBorder).BorderImage(HarvestMagic::Fill()).BorderBackgroundColor(FLinearColor::Transparent).Padding(FMargin(0))
       [SNew(SScrollBox)+SScrollBox::Slot()[SAssignNew(List,SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(8,8))]]]
      +SHorizontalBox::Slot().FillWidth(.41f).Padding(12,0,0,0)
      [SNew(SBorder).BorderImage(HarvestMagic::Fill()).BorderBackgroundColor(FLinearColor::Transparent).Padding(FMargin(20,12))
       [SNew(SVerticalBox)
        +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("物品记录"))).Font(HarvestMagic::Font(10,true)).ColorAndOpacity(HarvestMagic::Moss)]
        +SVerticalBox::Slot().AutoHeight().Padding(0,12,0,0)[SAssignNew(Detail,STextBlock).Font(HarvestMagic::Font(16)).ColorAndOpacity(HarvestMagic::Ink).AutoWrapText(true)]]]]
     +SVerticalBox::Slot().AutoHeight().Padding(FMargin(42,0,42,35))
     [SNew(SButton).ButtonStyle(&ForestTheme::SecondaryButton()).ButtonColorAndOpacity(FLinearColor::White).ContentPadding(FMargin(12,8)).OnClicked_Lambda([this](){Close();return FReply::Handled();})
      [SNew(STextBlock).Text(FText::FromString(TEXT("合上背包"))).Font(HarvestMagic::Font(12,true)).Justification(ETextJustify::Center).ColorAndOpacity(HarvestMagic::Ink)]]
    ]]]]];
}
void UForestMagicInventoryWidget::Refresh()
{
 if(!List.IsValid() || !Owner.IsValid())return;List->ClearChildren();
 if(bMajorsTab)
 {
  auto* A=GetWorld()->GetSubsystem<UForestAlchemy>();const auto Cards=A?A->GetTodayMajors():TArray<FForestDailyMajor>();
  for(const auto& C:Cards){FAlchemyCardDefinition D;if(!A->GetMajorDefinition(C.CardId,D))continue;
   List->AddSlot()[SNew(SBox).WidthOverride(110).HeightOverride(156)
    [SNew(SButton).ButtonStyle(&ForestTheme::SecondaryButton()).ButtonColorAndOpacity_Lambda([this,C](){return SelectedMajor==C.InstanceId?ForestTheme::SelectedPaper:FLinearColor::White;})
     .OnClicked_Lambda([this,C](){SelectMajor(C.InstanceId);return FReply::Handled();})
     [SNew(SVerticalBox)
      +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[SNew(STextBlock).Text(FText::FromName(D.Id)).Font(HarvestMagic::Font(24)).ColorAndOpacity(HarvestMagic::Moss)]
      +SVerticalBox::Slot().FillHeight(1).VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(D.Name+TEXT("\n")+D.ShortExpression)).Font(HarvestMagic::Font(12)).ColorAndOpacity(HarvestMagic::Ink).AutoWrapText(true).Justification(ETextJustify::Center)]
      +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("额度 %d"),D.Cost))).Font(HarvestMagic::Font(10)).ColorAndOpacity(HarvestMagic::Muted)]]]];
  }
  if(Cards.IsEmpty())Detail->SetText(FText::FromString(TEXT("今日大牌\n\n拿起小屋里的药瓶，每瓶每天可抽取一张大牌。\n大牌只在当天可用，夜间放入合成台改变元素药性。")));
  else if(!Cards.ContainsByPredicate([this](const auto& C){return C.InstanceId==SelectedMajor;}))SelectMajor(Cards[0].InstanceId);else SelectMajor(SelectedMajor);
  return;
 }
 auto Bag=Owner->ReadBag();TArray<FString> AllKeys;Bag.GetKeys(AllKeys);AllKeys.Sort();TArray<FString> Keys;
 for(const auto& Key:AllKeys)if(Bag[Key]>0)
 {
  Keys.Add(Key);FForestMagicItem Item;const bool Known=Owner->FindItem(Key,Item);
  const FString Name=Known?Item.Name:Key;
  const FString Element=Known?UForestHarvestMagic::ElementText(Item.Element):TEXT("物");
  const FLinearColor Accent=Known?(Item.Element==EAlchemyElement::Fire?HarvestMagic::Fire:Item.Element==EAlchemyElement::Water?HarvestMagic::Water:HarvestMagic::Wind):HarvestMagic::Muted;
  const FString Attribute=Known?FString::Printf(TEXT("%s%d"),*Element,Item.PipValue):TEXT("随身物品");
  List->AddSlot()[SNew(SBox).WidthOverride(110.f).HeightOverride(120.f)
   [SNew(SButton).ButtonStyle(&ForestTheme::SecondaryButton()).ContentPadding(FMargin(10,9))
    .ButtonColorAndOpacity_Lambda([this,Key](){return FSlateColor(Selected==Key?ForestTheme::SelectedPaper:FLinearColor::White);})
    .ToolTipText(FText::FromString(Key)).OnClicked_Lambda([this,Key](){Select(Key);return FReply::Handled();})
    [SNew(SVerticalBox)
     +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
      [SNew(SBox).WidthOverride(48).HeightOverride(40)[SNew(SImage).Image(ForestTheme::ElementIcon(Known?(Item.Element==EAlchemyElement::Fire?0:Item.Element==EAlchemyElement::Water?2:1):-1))]]
     +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
      [SNew(SBorder).BorderImage(HarvestMagic::Fill()).BorderBackgroundColor(Accent.CopyWithNewOpacity(.12f)).Padding(FMargin(8,1))
       [SNew(STextBlock).Text(FText::FromString(Attribute)).Font(HarvestMagic::Font(12,true)).ColorAndOpacity(HarvestMagic::Ink)]]
     +SVerticalBox::Slot().FillHeight(1.f).VAlign(VAlign_Center).Padding(0,5)
      [SNew(STextBlock).Text(FText::FromString(Name)).Font(HarvestMagic::Font(10)).ColorAndOpacity(HarvestMagic::Ink).WrapTextAt(92).Justification(ETextJustify::Center)]
     +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
      [SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("×%d"),Bag[Key]))).Font(HarvestMagic::Font(10)).ColorAndOpacity(HarvestMagic::Muted)]]]];
 }
 for(int32 I=Keys.Num();I<12;++I)List->AddSlot()[SNew(SBox).WidthOverride(110.f).HeightOverride(120.f)[SNew(SBorder).BorderImage(HarvestMagic::Fill()).BorderBackgroundColor(HarvestMagic::Edge.CopyWithNewOpacity(.10f))]];
 if(Keys.IsEmpty()){Detail->SetText(FText::FromString(TEXT("背包里还没有物品。\n\n采摘蘑菇后，魔法书会把其中的火、风或水凝成一件可以交付、查看和记住的小物。")));Selected.Reset();}
 else if(Selected.IsEmpty() || !Bag.Contains(Selected))Select(Keys[0]);else Select(Selected);
}
void UForestMagicInventoryWidget::SelectMajor(const FString& Id)
{
 SelectedMajor=Id;auto* A=GetWorld()->GetSubsystem<UForestAlchemy>();if(!A)return;
 for(const auto& C:A->GetTodayMajors())if(C.InstanceId==Id){FAlchemyCardDefinition D;if(A->GetMajorDefinition(C.CardId,D))Detail->SetText(FText::FromString(FString::Printf(TEXT("%s · %s\n\n今日大牌 · 第%d天\n额度开销：%d\n\n%s\n\n只在今日可用；放入夜间合成台后按顺序生效。"),*D.Id.ToString(),*D.Name,C.Day,D.Cost,*D.Description)));return;}
}
void UForestMagicInventoryWidget::Select(const FString& Key)
{
 Selected=Key;FForestMagicItem Item;if(Owner.IsValid() && Owner->FindItem(Key,Item))Detail->SetText(FText::FromString(FString::Printf(TEXT("%s\n\n%s元素 · %d 点\n采集于第 %d 天\n\n%s"),*Item.Name,*UForestHarvestMagic::ElementText(Item.Element),Item.PipValue,Item.HarvestDay,*Item.Description)));else Detail->SetText(FText::FromString(Key+TEXT("\n\n这件物品尚未留下魔法书的详细记录。")));
}
void UForestMagicInventoryWidget::Close(){if(Owner.IsValid())Owner->ToggleInventory();}
FReply UForestMagicInventoryWidget::NativeOnPreviewKeyDown(const FGeometry& G,const FKeyEvent& E){if(E.GetKey()==EKeys::Escape || E.GetKey()==EKeys::B){Close();return FReply::Handled();}return Super::NativeOnPreviewKeyDown(G,E);}
TMap<FString,int32> UForestHarvestMagic::ReadBag() const
{
 TMap<FString,int32> Result;auto* Pawn=BagOwner.Get();if(!Pawn)return Result;auto* Map=FindFProperty<FMapProperty>(Pawn->GetClass(),TEXT("Backpack"));auto* Key=Map?CastField<FStrProperty>(Map->KeyProp):nullptr;auto* Value=Map?CastField<FIntProperty>(Map->ValueProp):nullptr;if(!Key || !Value)return Result;FScriptMapHelper H(Map,Map->ContainerPtrToValuePtr<void>(Pawn));for(int I=0;I<H.GetMaxIndex();++I)if(H.IsValidIndex(I))Result.Add(Key->GetPropertyValue(H.GetKeyPtr(I)),Value->GetPropertyValue(H.GetValuePtr(I)));return Result;
}
void UForestHarvestMagic::Adjust(const FString& Name,int32 Delta)
{
 auto* Pawn=BagOwner.Get();if(!Pawn)return;auto* Map=FindFProperty<FMapProperty>(Pawn->GetClass(),TEXT("Backpack"));auto* Key=Map?CastField<FStrProperty>(Map->KeyProp):nullptr;auto* Value=Map?CastField<FIntProperty>(Map->ValueProp):nullptr;if(!Key || !Value)return;FScriptMapHelper H(Map,Map->ContainerPtrToValuePtr<void>(Pawn));
 for(int I=0;I<H.GetMaxIndex();++I)if(H.IsValidIndex(I) && Key->GetPropertyValue(H.GetKeyPtr(I))==Name){const int N=Value->GetPropertyValue(H.GetValuePtr(I))+Delta;if(N<=0){H.RemoveAt(I);H.Rehash();}else Value->SetPropertyValue(H.GetValuePtr(I),N);return;}if(Delta>0){FString K=Name;H.AddPair(&K,&Delta);}
}
void UForestHarvestMagic::RefreshLegacyHUD()
{
 auto* Pawn=BagOwner.Get();if(!Pawn)return;const auto Bag=ReadBag();TArray<FString> Keys;Bag.GetKeys(Keys);Keys.Sort();FString Text;for(const auto& K:Keys)if(Bag[K]>0)Text+=FString::Printf(TEXT("%s ×%d\n"),*K,Bag[K]);if(auto* P=FindFProperty<FStrProperty>(Pawn->GetClass(),TEXT("BackpackList")))P->SetPropertyValue_InContainer(Pawn,Text);if(auto* F=Pawn->FindFunction(TEXT("RefreshHUD")))Pawn->ProcessEvent(F,nullptr);if(Inventory && Inventory->IsInViewport())Inventory->Refresh();
}
bool UForestHarvestMagic::FindItem(const FString& Key,FForestMagicItem& Out) const{if(Ledger)for(const auto& Item:Ledger->Items)if(Item.InventoryKey==Key){Out=Item;return true;}return false;}
void UForestHarvestMagic::Persist(){if(Ledger)UGameplayStatics::SaveGameToSlot(Ledger,HarvestMagic::Slot,0);}
void UForestHarvestMagic::ConfigureMushroomCollision(UStaticMeshComponent* Mesh)
{
 if(!Mesh || !Mesh->GetStaticMesh() || !Mesh->GetStaticMesh()->GetName().StartsWith(TEXT("SM_Env_Mushroom")))return;
 if(Mesh->CanEverAffectNavigation())Mesh->SetCanEverAffectNavigation(false);
 // ForestDecoration retains Visibility queries, while ignoring Pawn, ForestNPC
 // and Camera. Actor-level hiding after harvest can still disable all queries.
 if(Mesh->GetCollisionProfileName()!=TEXT("ForestDecoration") || Mesh->GetCollisionEnabled()!=ECollisionEnabled::QueryOnly)
  Mesh->SetCollisionProfileName(TEXT("ForestDecoration"));
}
void UForestHarvestMagic::Setup()
{
 auto* PC=UGameplayStatics::GetPlayerController(GetWorld(),0);if(!PC || !PC->GetPawn())return;Player=PC;BagOwner=PC->GetPawn();if(!FindFProperty<FMapProperty>(BagOwner->GetClass(),TEXT("Backpack")))return;
 Ledger=Cast<UForestMagicItemSave>(UGameplayStatics::LoadGameFromSlot(HarvestMagic::Slot,0));if(!Ledger)Ledger=NewObject<UForestMagicItemSave>(this);
 Input=NewObject<UInputComponent>(PC);Input->RegisterComponent();Input->Priority=50;Input->BindKey(EKeys::B,IE_Pressed,this,&UForestHarvestMagic::ToggleInventory);PC->PushInputComponent(Input);Previous=ReadBag();bReady=true;
 FieldHUD=CreateWidget<UForestFieldHUDWidget>(PC);if(FieldHUD){FieldHUD->AddToViewport(10);FieldHUD->SetVisibility(ESlateVisibility::HitTestInvisible);}
 for(TActorIterator<AActor> It(GetWorld());It;++It)if(auto* M=It->FindComponentByClass<UStaticMeshComponent>(); M && M->GetStaticMesh() && M->GetStaticMesh()->GetName().StartsWith(TEXT("SM_Env_Mushroom"))){ConfigureMushroomCollision(M);MushroomVisibility.Add(M,It->IsHidden());}
 auto* NewNotice=CreateWidget<UForestHarvestNoticeWidget>(PC);
 if(NewNotice){NoticeWidget=NewNotice;NoticeWidget->AddToViewport(30);Notice=NewNotice->Text;NoticeWidget->SetVisibility(ESlateVisibility::Collapsed);}
}
void UForestHarvestMagic::ToggleInventory()
{
 auto* PC=Player.Get();if(!PC)return;if(auto* A=GetWorld()->GetSubsystem<UForestAlchemy>();A && A->IsWorkbenchOpen())return;if(Inventory && Inventory->IsInViewport()){Inventory->RemoveFromParent();PC->SetInputMode(FInputModeGameOnly());PC->bShowMouseCursor=bOldCursor;return;}
 if(auto* Manager=Cast<AForestNPCInteraction>(UGameplayStatics::GetActorOfClass(GetWorld(),AForestNPCInteraction::StaticClass()));Manager && (Manager->IsDialogueOpen() || Manager->IsFinderMenuOpen()))return;
 if(!Inventory){Inventory=CreateWidget<UForestMagicInventoryWidget>(PC);Inventory->Owner=this;}bOldCursor=PC->bShowMouseCursor;Inventory->AddToViewport(100);Inventory->Refresh();FInputModeUIOnly Mode;Mode.SetWidgetToFocus(Inventory->TakeWidget());PC->SetInputMode(Mode);PC->bShowMouseCursor=true;
}
FString UForestHarvestMagic::Prompt(const FForestMagicItem& I)
{
 return FString::Printf(TEXT("森林魔法书把刚采下的蘑菇炼尽，将其中的%s元素（强度%d，范围1至9）凝成一件能拿在手里的奇异物件。生成名称与道具描述。\n规则：名称4至12个汉字，必须具象，包含具体物件，如纽扣、胡桃、线轴、铜铃、蜡烛、怀表、钥匙、梳子、小瓶、石子；可以创造其他日常小物。名称体现物件形态和一个魔法异常，禁止仅用余响、灵质、暖意等抽象词作物品。避免重复套用模板。描述25至65个汉字，写材质、触感、外形和一个可感知的奇异性质，像童话道具说明；不写实验室用语，不引用哈利波特，不新增伤害、治疗、增益、任务或任何实际机制。低强度魔法细微，高强度鲜明。不要继续绑定蘑菇外观。元素与数字由游戏决定，你不得修改或输出数值。只输出JSON：{\"name\":\"物品名\",\"description\":\"一句道具描述\"}"),*ElementText(I.Element),I.PipValue);
}
void UForestHarvestMagic::Harvest(const FString& Source,int32 Count)
{
 auto* Daily=GetWorld()->GetGameInstance()->GetSubsystem<UDailyMushroomSubsystem>();FDailyMushroomCardRow Row;if(!Daily || !Daily->GetDailyMushroom(FName(*HarvestMagic::BaseMushroom(Source)),Row))return;
 for(int N=0;N<Count;++N)
 {
  FForestMagicItem I;I.Id=FGuid::NewGuid().ToString();I.SourceMushroom=Source;I.Element=Row.Element;I.PipValue=Row.PipValue;I.Value=Row.Value;I.HarvestDay=Row.Day;
  const int Variant=FCrc::StrCrc32(*I.Id)%3;
  static const TCHAR* Names[3][3]={{TEXT("炉火纽扣"),TEXT("藏火胡桃"),TEXT("不肯熄灭的短蜡烛")},{TEXT("留住叹息的线轴"),TEXT("没有舌头的铜铃"),TEXT("追风的小梳子")},{TEXT("雨珠玻璃扣"),TEXT("装着小雨的瓶子"),TEXT("潮汐怀表")}};
  static const TCHAR* Descriptions[3][3]={{TEXT("黄铜边缘始终温热，中央的小火苗偶尔探出头，仿佛还在等一件合身的外套。"),TEXT("胡桃壳缝中漏出暖光，贴近耳边，能听见里面细小的柴火声。"),TEXT("蜡烛只有指节长，每当火苗垂下头，烛芯便悄悄把它扶起来。")},{TEXT("苍白的细线会自行松开一小截，随后传来一声不知属于谁的叹息。"),TEXT("铃内空空如也，风经过时，却会响起三声清楚的铃音。"),TEXT("木梳的齿间总有细风穿过，握在手里，发梢便朝森林深处轻轻偏去。")},{TEXT("透明扣子里封着一扇小窗，窗外总在下雨，窗内却没有人。"),TEXT("软木塞下飘着一片小云，瓶壁上不时滑过一滴刚落下的雨。"),TEXT("表盘没有指针，只有一片缓慢涨落的海，浪花会轻轻碰响银色刻度。")}};
  const int E=I.Element==EAlchemyElement::Wind?1:I.Element==EAlchemyElement::Water?2:0;I.Name=Names[E][Variant];I.Description=Descriptions[E][Variant];I.InventoryKey=FString::Printf(TEXT("%s · %s%d"),*I.Name,*ElementText(I.Element),I.PipValue);
  // Do not merge different descriptions or generation requests under the same legacy key.
  int Serial=1;const FString BaseName=I.Name;while(Ledger->Items.ContainsByPredicate([&](const auto& Existing){return Existing.InventoryKey==I.InventoryKey;})){I.Name=FString::Printf(TEXT("%s（%d）"),*BaseName,++Serial);I.InventoryKey=FString::Printf(TEXT("%s · %s%d"),*I.Name,*ElementText(I.Element),I.PipValue);}
  Adjust(Source,-1);Adjust(I.InventoryKey,1);Ledger->Items.Add(I);if(auto* P=FindFProperty<FStrProperty>(BagOwner->GetClass(),TEXT("LastPicked")))P->SetPropertyValue_InContainer(BagOwner.Get(),I.InventoryKey);ShowResult(I);BeginVisual(Source,I.Element);
  auto* AI=GetWorld()->GetGameInstance()->GetSubsystem<UDialogueAIServiceSubsystem>();if(AI && AI->HasApiKey()){auto* Request=NewObject<UForestHarvestRequest>(this);Request->Owner=this;Request->ItemId=I.Id;Requests.Add(Request);FDialogueAICompletion Completion;Completion.BindDynamic(Request,&UForestHarvestRequest::Completed);FDialogueMessage System;System.Role=TEXT("system");System.Content=TEXT("你为森林童话游戏设计具象的魔法道具。遵守输入事实与JSON格式，名称富有变化，描述具体自然。");FDialogueMessage User;User.Role=TEXT("user");User.Content=Prompt(I);AI->RequestChatCompletion({System,User},.95f,260,Completion);}
 }
 RefreshLegacyHUD();Persist();
}
void UForestHarvestMagic::HandleResponse(UForestHarvestRequest* Request,bool Success,const FString& Text)
{
 if(!Request || !Requests.Contains(Request))return;Requests.Remove(Request);if(!Success || !Ledger)return;FString Clean=Text;Clean.TrimStartAndEndInline();if(Clean.StartsWith(TEXT("```"))){int End=Clean.Find(TEXT("\n"));if(End!=INDEX_NONE)Clean=Clean.Mid(End+1);Clean.RemoveFromEnd(TEXT("```"));}TSharedPtr<FJsonObject> JSON;if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Clean),JSON) || !JSON)return;FString Name,Description;if(!JSON->TryGetStringField(TEXT("name"),Name) || !JSON->TryGetStringField(TEXT("description"),Description))return;Name.TrimStartAndEndInline();Description.TrimStartAndEndInline();if(Name.Len()<2 || Name.Len()>16 || Description.Len()<10 || Description.Len()>100 || Name.Contains(TEXT("\n")) || Name.Contains(TEXT("·")))return;
 for(auto& I:Ledger->Items)if(I.Id==Request->ItemId)
 {
  // Freeze identity once the item has been moved or spent. A late reply must never re-create it.
  const FString Old=I.InventoryKey;const auto Bag=ReadBag();if(!Bag.Contains(Old) || Bag[Old]<=0)return;const FString New=FString::Printf(TEXT("%s · %s%d"),*Name,*ElementText(I.Element),I.PipValue);if(Ledger->Items.ContainsByPredicate([&](const auto& Other){return Other.Id!=I.Id && Other.InventoryKey==New;}))return;
  Adjust(Old,-Bag[Old]);Adjust(New,Bag[Old]);if(auto* P=FindFProperty<FStrProperty>(BagOwner->GetClass(),TEXT("LastPicked")))if(P->GetPropertyValue_InContainer(BagOwner.Get())==Old)P->SetPropertyValue_InContainer(BagOwner.Get(),New);I.Name=Name;I.Description=Description;I.InventoryKey=New;I.bGeneratedByAI=true;Previous=ReadBag();RefreshLegacyHUD();Persist();ShowResult(I);return;
 }
}
void UForestHarvestMagic::ShowResult(const FForestMagicItem& I)
{
 if(!Notice.IsValid())return;Notice->SetText(FText::FromString(FString::Printf(TEXT("蘑菇散作星尘，魔法书将其中的%s凝成了一件小物。\n获得：%s · %s%d\n%s\n已收入背包 · 按 B 查看"),*ElementText(I.Element),*I.Name,*ElementText(I.Element),I.PipValue,*I.Description)));NoticeUntil=GetWorld()->GetTimeSeconds()+8;NoticeWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
}
void UForestHarvestMagic::BeginVisual(const FString& Source,EAlchemyElement Element)
{
 auto* PC=Player.Get();if(!PC)return;UStaticMeshComponent* Original=JustHarvested.FindRef(HarvestMagic::BaseMushroom(Source)).Get();JustHarvested.Remove(HarvestMagic::BaseMushroom(Source));
 if(!Original)return;FActorSpawnParameters Params;Params.ObjectFlags|=RF_Transient;auto* A=GetWorld()->SpawnActor<AActor>(AActor::StaticClass(),Original->GetComponentLocation(),FRotator::ZeroRotator,Params);if(!A)return;
 auto* Mushroom=NewObject<UStaticMeshComponent>(A);A->SetRootComponent(Mushroom);Mushroom->SetStaticMesh(Original->GetStaticMesh());for(int J=0;J<Original->GetNumMaterials();++J)Mushroom->SetMaterial(J,Original->GetMaterial(J));Mushroom->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mushroom->RegisterComponent();Mushroom->SetWorldTransform(Original->GetComponentTransform());
 auto* Book=NewObject<USkeletalMeshComponent>(A);Book->SetupAttachment(Mushroom);Book->SetSkeletalMeshAsset(LoadObject<USkeletalMesh>(nullptr,HarvestMagic::BookMesh));Book->SetCollisionEnabled(ECollisionEnabled::NoCollision);Book->RegisterComponent();Book->SetWorldScale3D(FVector(.35));Book->PlayAnimation(HarvestMagic::Anim(TEXT("Mimic-book_Open_Anim")),false);FLinearColor Color=Element==EAlchemyElement::Fire?FLinearColor(3,.3,.04):Element==EAlchemyElement::Wind?FLinearColor(.3,2,1):FLinearColor(.1,.5,3);for(int J=0;J<Book->GetNumMaterials();++J)if(auto* MID=Book->CreateDynamicMaterialInstance(J)){MID->SetVectorParameterValue(TEXT("EmissionColor"),Color);MID->SetScalarParameterValue(TEXT("EmissionPower"),4);}
 FVisual V;V.Actor=A;V.Mushroom=Mushroom;V.Book=Book;V.Origin=Original->GetComponentLocation();V.Scale=Original->GetComponentScale();V.Element=Element;Visuals.Add(V);
}
void UForestHarvestMagic::UpdateVisuals(float DT)
{
 for(int J=Visuals.Num()-1;J>=0;--J){auto& V=Visuals[J];V.Age+=DT;if(!V.Actor.IsValid()){Visuals.RemoveAt(J);continue;}auto* M=V.Mushroom.Get();auto* Book=V.Book.Get();float T=FMath::Clamp((V.Age-.4f)/1.3f,0.f,1.f);if(M){M->SetWorldLocation(V.Origin+FVector(0,0,50*FMath::Clamp(V.Age/.5f,0.f,1.f)));M->SetWorldScale3D(V.Scale*FMath::Max(.001f,1-T));if(T>=1)M->SetVisibility(false,false);}if(Book){Book->SetWorldLocation(V.Origin+FVector(45*FMath::Cos(V.Age*2),45*FMath::Sin(V.Age*2),80));Book->SetWorldScale3D(FVector(.35));Book->SetWorldRotation(FRotator(0,V.Age*50,0));}
 if(V.Age>=1.7f && !V.bBurst){V.bBurst=true;if(Book)Book->PlayAnimation(HarvestMagic::Anim(TEXT("Mimic-book_Unfold_Anim")),false);auto* F=HarvestMagic::FX(GetWorld(),V.Element==EAlchemyElement::Water?TEXT("FXS_Water_Splash_01"):V.Element==EAlchemyElement::Wind?TEXT("FXS_Leaves_01"):TEXT("FXS_Sparkles"),V.Origin+FVector(0,0,85),.12);if(F){F->SetAutoDestroy(true);FTimerHandle Handle;TWeakObjectPtr<UNiagaraComponent> Weak=F;GetWorld()->GetTimerManager().SetTimer(Handle,[Weak](){if(Weak.IsValid())Weak->Deactivate();},1.2f,false);}if(V.Element==EAlchemyElement::Fire){auto* Sparks=UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),LoadObject<UParticleSystem>(nullptr,TEXT("/Game/Witch_House/Demo/StarterContent/Particles/P_Sparks.P_Sparks")),V.Origin+FVector(0,0,85),FRotator::ZeroRotator,FVector(.12));if(Sparks){FTimerHandle H;TWeakObjectPtr<UParticleSystemComponent> Weak=Sparks;GetWorld()->GetTimerManager().SetTimer(H,[Weak](){if(Weak.IsValid())Weak->DeactivateSystem();},1.f,false);}}}
 if(V.Age>3){V.Actor->Destroy();Visuals.RemoveAt(J);}
 }
}
void UForestHarvestMagic::Tick(float DT)
{
 if(!GetWorld() || !GetWorld()->IsGameWorld())return;if(!bReady){Setup();return;}UpdateVisuals(DT);if(NoticeWidget && GetWorld()->GetTimeSeconds()>NoticeUntil)NoticeWidget->SetVisibility(ESlateVisibility::Collapsed);if(!BagOwner.IsValid())return;
 FieldUITime+=DT;if(FieldUITime>=.25f){FieldUITime=0;RefreshFieldUI();}
 PollTime+=DT;if(PollTime<.05f)return;PollTime=0;
 JustHarvested.Reset();for(auto& Pair:MushroomVisibility)if(auto* Mesh=Pair.Key.Get()){const bool Hidden=Mesh->GetOwner()->IsHidden();if(Hidden && !Pair.Value && Mesh->GetStaticMesh())JustHarvested.Add(HarvestMagic::BaseMushroom(Mesh->GetStaticMesh()->GetName()),Mesh);Pair.Value=Hidden;}
 const auto Bag=ReadBag();for(const auto& Pair:Bag){if(!Pair.Key.StartsWith(TEXT("SM_Env_Mushroom")))continue;const int Delta=Pair.Value-Previous.FindRef(Pair.Key);if(Delta>0)Harvest(Pair.Key,Delta);}Previous=ReadBag();
}
TStatId UForestHarvestMagic::GetStatId() const{RETURN_QUICK_DECLARE_CYCLE_STAT(UForestHarvestMagic,STATGROUP_Tickables);}
void UForestHarvestMagic::Deinitialize(){for(auto& R:Requests)if(R)R->Owner.Reset();Requests.Reset();for(auto& V:Visuals)if(V.Actor.IsValid())V.Actor->Destroy();Visuals.Reset();if(auto* PC=Player.Get())if(Input)PC->PopInputComponent(Input);if(Inventory)Inventory->RemoveFromParent();if(NoticeWidget)NoticeWidget->RemoveFromParent();if(FieldHUD)FieldHUD->RemoveFromParent();Super::Deinitialize();}
