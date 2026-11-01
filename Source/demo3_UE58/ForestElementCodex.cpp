#include "ForestElementCodex.h"
#include "ForestUITheme.h"
#include "ForestHarvestMagic.h"
#include "ForestAlchemy.h"
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
 Input->BindKey(EKeys::J,IE_Pressed,this,&UForestElementCodex::Toggle);PC->PushInputComponent(Input);bReady=true;
}
bool UForestElementCodex::IsOpen() const{return Widget && Widget->IsInViewport();}
bool UForestElementCodex::OtherMenuOpen() const
{
 if(auto* H=GetWorld()->GetSubsystem<UForestHarvestMagic>();H && H->IsInventoryOpen())return true;
 if(auto* A=GetWorld()->GetSubsystem<UForestAlchemy>();A && (A->IsWorkbenchOpen() || A->IsCampaignCompleted()))return true;
 if(auto* M=Cast<AForestNPCInteraction>(UGameplayStatics::GetActorOfClass(this,AForestNPCInteraction::StaticClass()));M && (M->IsDialogueOpen() || M->IsFinderMenuOpen()))return true;
 return false;
}
void UForestElementCodex::Open()
{
 if(!bReady)Setup();auto* PC=Player.Get();if(!bReady || !PC || IsOpen() || OtherMenuOpen())return;
 if(APawn* Pawn=PC->GetPawn())if(auto* View=Pawn->FindComponentByClass<UForestIsometricPlayer>();View && (View->bGathering || View->bClimbing))return;
 if(!Widget){Widget=CreateWidget<UForestElementCodexWidget>(PC);if(!Widget)return;Widget->Owner=this;}
 bOldCursor=PC->bShowMouseCursor;PC->StopMovement();Widget->AddToViewport(125);Widget->Refresh();
 OpeningTime=OpenAnimation && BookMesh ? .55f : 0.f;Widget->SetRenderOpacity(OpeningTime>0?0.f:1.f);
 FInputModeUIOnly Mode;Mode.SetWidgetToFocus(Widget->TakeWidget());PC->SetInputMode(Mode);PC->bShowMouseCursor=true;
 if(auto* H=GetWorld()->GetSubsystem<UForestHarvestMagic>())H->DismissNotice();PlayBook(true);
}
void UForestElementCodex::Toggle(){if(IsOpen())Close();else Open();}
void UForestElementCodex::Close()
{
 if(!IsOpen())return;Widget->RemoveFromParent();OpeningTime=0;PlayBook(false);
 if(auto* PC=Player.Get())
 {
  PC->bShowMouseCursor=bOldCursor;
  if(APawn* Pawn=PC->GetPawn())if(auto* View=Pawn->FindComponentByClass<UForestIsometricPlayer>()){View->RefreshGameplayInput();return;}
  if(bOldCursor){FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);PC->SetInputMode(Mode);}else PC->SetInputMode(FInputModeGameOnly());
 }
}
void UForestElementCodex::Select(int32 E,int32 Strength)
{SelectedElement=FMath::Clamp(E,0,2);SelectedStrength=FMath::Clamp(Strength,1,10);if(IsOpen())Widget->Refresh();}
void UForestElementCodex::TurnPage(int32 Direction)
{const int32 Index=FMath::Clamp(SelectedElement*10+SelectedStrength-1+Direction,0,29);Select(Index/10,Index%10+1);}
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
{if(Widget)Widget->RemoveFromParent();if(auto* A=BookVisual.Get())A->Destroy();if(auto* PC=Player.Get())if(Input)PC->PopInputComponent(Input);Super::Deinitialize();}

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
 auto Tabs=SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6,4));
 for(int32 E=0;E<3;++E)Tabs->AddSlot()[ElementBook::Button(FString(ElementBook::Names[E])+TEXT("之卷"),FOnClicked::CreateLambda([this,E](){if(Owner.IsValid())Owner->Select(E,1);return FReply::Handled();}),A->SelectedElement==E)];
 auto Detail=SNew(SVerticalBox);
 Detail->AddSlot().AutoHeight()[ElementBook::Text(TEXT("元素手札"),24,true)];
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
 if(bSinglePage)
 {
  Detail->AddSlot().AutoHeight().Padding(0,16)[ElementBook::Text(A->Introduction,16)];
  Detail->AddSlot().AutoHeight().Padding(0,14)[ElementBook::Text(A->Notes,16)];
 }
 auto Right=SNew(SVerticalBox);
 Right->AddSlot().FillHeight(1)[SNew(SScrollBox).ScrollBarAlwaysVisible(true)+SScrollBox::Slot()[Detail]];
 auto Navigation=SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(5,5));
 Navigation->AddSlot()[ElementBook::Button(TEXT("← 前页"),FOnClicked::CreateLambda([this](){if(Owner.IsValid())Owner->TurnPage(-1);return FReply::Handled();}))];
 Navigation->AddSlot()[ElementBook::Button(TEXT("后页 →"),FOnClicked::CreateLambda([this](){if(Owner.IsValid())Owner->TurnPage(1);return FReply::Handled();}))];
 Navigation->AddSlot()[ElementBook::Button(TEXT("合上书 · J / Esc"),FOnClicked::CreateLambda([this](){if(Owner.IsValid())Owner->Close();return FReply::Handled();}))];
 Right->AddSlot().AutoHeight().Padding(0,12)[Navigation];
 Right->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0,10,0,0)[ElementBook::Text(FString::Printf(TEXT("— %d —"),A->SelectedElement*10+A->SelectedStrength),16)];
 auto Spread=SNew(SHorizontalBox);
 if(!bSinglePage)
 {
  auto Index=SNew(SVerticalBox);Index->AddSlot().AutoHeight().Padding(0,0,0,18)[ElementBook::Text(TEXT("查阅目录"),24,true)];
  Index->AddSlot().AutoHeight().Padding(0,0,0,16)[ElementBook::Text(A->Introduction,16)];
  for(const auto& E:A->Entries)if(E.Element==ElementBook::Keys[A->SelectedElement])
  {const int32 Strength=E.Strength;Index->AddSlot().AutoHeight().Padding(0,3)[ElementBook::Button(FString::Printf(TEXT("%d   %s"),Strength,*E.Title),FOnClicked::CreateLambda([this,Strength](){if(Owner.IsValid())Owner->Select(Owner->SelectedElement,Strength);return FReply::Handled();}),Strength==A->SelectedStrength)];}
  Index->AddSlot().AutoHeight().Padding(0,22,0,0)[ElementBook::Text(A->Notes,16)];
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
