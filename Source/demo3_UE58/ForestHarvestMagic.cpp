#include "ForestHarvestMagic.h"
#include "ForestNPCInteraction.h"
#include "DailyMushroomSubsystem.h"
#include "DialogueAIServiceSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
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
}
void UForestHarvestRequest::Completed(bool Success,const FString& Text){if(Owner.IsValid())Owner->HandleResponse(this,Success,Text);}
FString UForestHarvestMagic::ElementText(EAlchemyElement E){return E==EAlchemyElement::Wind?TEXT("风"):E==EAlchemyElement::Water?TEXT("水"):TEXT("火");}
void UForestMagicItemButton::Clicked(){if(Owner.IsValid())Owner->Select(Key);}
TSharedRef<SWidget> UForestHarvestNoticeWidget::RebuildWidget()
{
 if(!WidgetTree)WidgetTree=NewObject<UWidgetTree>(this,TEXT("HarvestNoticeTree"));
 auto* Border=WidgetTree->ConstructWidget<UBorder>();WidgetTree->RootWidget=Border;
 Border->SetBrushColor(FLinearColor(.02,.03,.025,.9));Border->SetPadding(FMargin(16));
 Text=WidgetTree->ConstructWidget<UTextBlock>();Text->SetWrapTextAt(600);Border->SetContent(Text);
 return Super::RebuildWidget();
}
TSharedRef<SWidget> UForestMagicInventoryWidget::RebuildWidget()
{
 SetIsFocusable(true);if(!WidgetTree)WidgetTree=NewObject<UWidgetTree>(this);
 auto* Border=WidgetTree->ConstructWidget<UBorder>();WidgetTree->RootWidget=Border;Border->SetPadding(FMargin(28));Border->SetBrushColor(FLinearColor(.025,.035,.04,.97));
 auto* Box=WidgetTree->ConstructWidget<UVerticalBox>();Border->SetContent(Box);
 auto* Title=WidgetTree->ConstructWidget<UTextBlock>();Title->SetText(FText::FromString(TEXT("背包 · 点击物品查看描述（B / Esc 关闭）")));Box->AddChildToVerticalBox(Title);
 auto* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetHeightOverride(350);Box->AddChildToVerticalBox(Size);
 auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>();Size->SetContent(Scroll);List=WidgetTree->ConstructWidget<UVerticalBox>();Scroll->AddChild(List);
 Detail=WidgetTree->ConstructWidget<UTextBlock>();Detail->SetWrapTextAt(650);Box->AddChildToVerticalBox(Detail);
 auto* Button=WidgetTree->ConstructWidget<UButton>();auto* Text=WidgetTree->ConstructWidget<UTextBlock>();Text->SetText(FText::FromString(TEXT("合上背包")));Button->SetContent(Text);Box->AddChildToVerticalBox(Button);Button->OnClicked.AddDynamic(this,&UForestMagicInventoryWidget::Close);
 return Super::RebuildWidget();
}
void UForestMagicInventoryWidget::Refresh()
{
 if(!List || !Owner.IsValid())return;List->ClearChildren();auto Bag=Owner->ReadBag();TArray<FString> Keys;Bag.GetKeys(Keys);Keys.Sort();
 for(const auto& Key:Keys){if(Bag[Key]<=0)continue;auto* Button=WidgetTree->ConstructWidget<UForestMagicItemButton>();Button->Owner=this;Button->Key=Key;auto* Label=WidgetTree->ConstructWidget<UTextBlock>();Label->SetText(FText::FromString(FString::Printf(TEXT("%s  ×%d"),*Key,Bag[Key])));Button->SetContent(Label);List->AddChildToVerticalBox(Button);Button->OnClicked.AddDynamic(Button,&UForestMagicItemButton::Clicked);}
 if(Keys.IsEmpty()){Detail->SetText(FText::FromString(TEXT("背包里还没有物品。采摘蘑菇，让魔法书唤出其中的元素。")));Selected.Reset();}
 else if(Selected.IsEmpty() || !Bag.Contains(Selected))Select(Keys[0]);else Select(Selected);
}
void UForestMagicInventoryWidget::Select(const FString& Key)
{
 Selected=Key;FForestMagicItem Item;if(Owner.IsValid() && Owner->FindItem(Key,Item))Detail->SetText(FText::FromString(FString::Printf(TEXT("%s\n%s%d\n%s"),*Item.Name,*UForestHarvestMagic::ElementText(Item.Element),Item.PipValue,*Item.Description)));else Detail->SetText(FText::FromString(Key+TEXT("\n这件物品尚未留下魔法书的记录。")));
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
void UForestHarvestMagic::Setup()
{
 auto* PC=UGameplayStatics::GetPlayerController(GetWorld(),0);if(!PC || !PC->GetPawn())return;Player=PC;BagOwner=PC->GetPawn();if(!FindFProperty<FMapProperty>(BagOwner->GetClass(),TEXT("Backpack")))return;
 Ledger=Cast<UForestMagicItemSave>(UGameplayStatics::LoadGameFromSlot(HarvestMagic::Slot,0));if(!Ledger)Ledger=NewObject<UForestMagicItemSave>(this);
 Input=NewObject<UInputComponent>(PC);Input->RegisterComponent();Input->Priority=50;Input->BindKey(EKeys::B,IE_Pressed,this,&UForestHarvestMagic::ToggleInventory);PC->PushInputComponent(Input);Previous=ReadBag();bReady=true;
 for(TActorIterator<AActor> It(GetWorld());It;++It)if(auto* M=It->FindComponentByClass<UStaticMeshComponent>(); M && M->GetStaticMesh() && M->GetStaticMesh()->GetName().StartsWith(TEXT("SM_Env_Mushroom")))MushroomVisibility.Add(M,It->IsHidden());
 auto* NewNotice=CreateWidget<UForestHarvestNoticeWidget>(PC);
 if(NewNotice){NoticeWidget=NewNotice;NoticeWidget->AddToViewport(30);Notice=NewNotice->Text;NoticeWidget->SetAlignmentInViewport(FVector2D(.5,0));int32 ScreenX,ScreenY;PC->GetViewportSize(ScreenX,ScreenY);NoticeWidget->SetPositionInViewport(FVector2D(ScreenX*.5f,60),false);NoticeWidget->SetDesiredSizeInViewport(FVector2D(620,160));NoticeWidget->SetVisibility(ESlateVisibility::Collapsed);}
}
void UForestHarvestMagic::ToggleInventory()
{
 auto* PC=Player.Get();if(!PC)return;if(Inventory && Inventory->IsInViewport()){Inventory->RemoveFromParent();PC->SetInputMode(FInputModeGameOnly());PC->bShowMouseCursor=bOldCursor;return;}
 if(auto* Manager=Cast<AForestNPCInteraction>(UGameplayStatics::GetActorOfClass(GetWorld(),AForestNPCInteraction::StaticClass()));Manager && (Manager->IsDialogueOpen() || Manager->IsFinderMenuOpen()))return;
 if(!Inventory){Inventory=CreateWidget<UForestMagicInventoryWidget>(PC);Inventory->Owner=this;}bOldCursor=PC->bShowMouseCursor;Inventory->AddToViewport(100);Inventory->SetDesiredSizeInViewport(FVector2D(720,560));Inventory->SetPositionInViewport(FVector2D(60,60));Inventory->Refresh();FInputModeUIOnly Mode;Mode.SetWidgetToFocus(Inventory->TakeWidget());PC->SetInputMode(Mode);PC->bShowMouseCursor=true;
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
 if(!Notice)return;Notice->SetText(FText::FromString(FString::Printf(TEXT("蘑菇散作星尘，魔法书将其中的%s凝成了一件小物。\n获得：%s · %s%d\n%s\n已收入背包 · 按 B 查看"),*ElementText(I.Element),*I.Name,*ElementText(I.Element),I.PipValue,*I.Description)));NoticeUntil=GetWorld()->GetTimeSeconds()+8;NoticeWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
}
void UForestHarvestMagic::BeginVisual(const FString& Source,EAlchemyElement Element)
{
 auto* PC=Player.Get();if(!PC)return;UStaticMeshComponent* Original=JustHarvested.FindRef(HarvestMagic::BaseMushroom(Source)).Get();JustHarvested.Remove(HarvestMagic::BaseMushroom(Source));
 if(!Original)return;FActorSpawnParameters Params;Params.ObjectFlags|=RF_Transient;auto* A=GetWorld()->SpawnActor<AActor>(AActor::StaticClass(),Original->GetComponentLocation(),FRotator::ZeroRotator,Params);if(!A)return;
 auto* Mushroom=NewObject<UStaticMeshComponent>(A);A->SetRootComponent(Mushroom);Mushroom->SetStaticMesh(Original->GetStaticMesh());for(int J=0;J<Original->GetNumMaterials();++J)Mushroom->SetMaterial(J,Original->GetMaterial(J));Mushroom->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mushroom->RegisterComponent();Mushroom->SetWorldTransform(Original->GetComponentTransform());
 auto* Book=NewObject<USkeletalMeshComponent>(A);Book->SetupAttachment(Mushroom);Book->SetSkeletalMeshAsset(LoadObject<USkeletalMesh>(nullptr,HarvestMagic::BookMesh));Book->SetCollisionEnabled(ECollisionEnabled::NoCollision);Book->RegisterComponent();Book->SetWorldScale3D(FVector(.35));Book->PlayAnimation(HarvestMagic::Anim(TEXT("Mimic-book_Open_Anim")),false);FLinearColor Color=Element==EAlchemyElement::Fire?FLinearColor(3,.3,.04):Element==EAlchemyElement::Wind?FLinearColor(.3,2,1):FLinearColor(.1,.5,3);for(int J=0;J<Book->GetNumMaterials();++J)if(auto* MID=Book->CreateDynamicMaterialInstance(J)){MID->SetVectorParameterValue(TEXT("EmissionColor"),Color);MID->SetScalarParameterValue(TEXT("EmissionPower"),4);}
 FVisual V;V.Actor=A;V.Mushroom=Mushroom;V.Book=Book;V.Origin=Original->GetComponentLocation();V.Scale=Original->GetComponentScale();V.Element=Element;V.Portal=HarvestMagic::FX(GetWorld(),TEXT("FXS_Portal_01"),V.Origin+FVector(0,0,55),.08);Visuals.Add(V);
 if(auto* Dust=HarvestMagic::FX(GetWorld(),TEXT("FXS_Dust_Spots_01"),V.Origin+FVector(0,0,35),.08)){FTimerHandle H;TWeakObjectPtr<UNiagaraComponent> Weak=Dust;GetWorld()->GetTimerManager().SetTimer(H,[Weak](){if(Weak.IsValid())Weak->Deactivate();},1.6f,false);}
}
void UForestHarvestMagic::UpdateVisuals(float DT)
{
 for(int J=Visuals.Num()-1;J>=0;--J){auto& V=Visuals[J];V.Age+=DT;if(!V.Actor.IsValid()){Visuals.RemoveAt(J);continue;}auto* M=V.Mushroom.Get();auto* Book=V.Book.Get();float T=FMath::Clamp((V.Age-.4f)/1.3f,0.f,1.f);if(M){M->SetWorldLocation(V.Origin+FVector(0,0,50*FMath::Clamp(V.Age/.5f,0.f,1.f)));M->SetWorldScale3D(V.Scale*FMath::Max(.001f,1-T));if(T>=1)M->SetVisibility(false,false);}if(Book){Book->SetWorldLocation(V.Origin+FVector(45*FMath::Cos(V.Age*2),45*FMath::Sin(V.Age*2),80));Book->SetWorldScale3D(FVector(.35));Book->SetWorldRotation(FRotator(0,V.Age*50,0));}
 if(V.Age>=1.7f && !V.bBurst){V.bBurst=true;if(V.Portal.IsValid())V.Portal->Deactivate();if(Book)Book->PlayAnimation(HarvestMagic::Anim(TEXT("Mimic-book_Unfold_Anim")),false);auto* F=HarvestMagic::FX(GetWorld(),V.Element==EAlchemyElement::Water?TEXT("FXS_Water_Splash_01"):V.Element==EAlchemyElement::Wind?TEXT("FXS_Leaves_01"):TEXT("FXS_Sparkles"),V.Origin+FVector(0,0,85),.12);if(F){F->SetAutoDestroy(true);FTimerHandle Handle;TWeakObjectPtr<UNiagaraComponent> Weak=F;GetWorld()->GetTimerManager().SetTimer(Handle,[Weak](){if(Weak.IsValid())Weak->Deactivate();},1.2f,false);}if(V.Element==EAlchemyElement::Fire){auto* Sparks=UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),LoadObject<UParticleSystem>(nullptr,TEXT("/Game/Witch_House/Demo/StarterContent/Particles/P_Sparks.P_Sparks")),V.Origin+FVector(0,0,85),FRotator::ZeroRotator,FVector(.12));if(Sparks){FTimerHandle H;TWeakObjectPtr<UParticleSystemComponent> Weak=Sparks;GetWorld()->GetTimerManager().SetTimer(H,[Weak](){if(Weak.IsValid())Weak->DeactivateSystem();},1.f,false);}}}
 if(V.Age>3){if(V.Portal.IsValid())V.Portal->DestroyComponent();V.Actor->Destroy();Visuals.RemoveAt(J);}
 }
}
void UForestHarvestMagic::Tick(float DT)
{
 if(!GetWorld() || !GetWorld()->IsGameWorld())return;if(!bReady){Setup();return;}UpdateVisuals(DT);if(NoticeWidget && GetWorld()->GetTimeSeconds()>NoticeUntil)NoticeWidget->SetVisibility(ESlateVisibility::Collapsed);if(!BagOwner.IsValid())return;
 PollTime+=DT;if(PollTime<.05f)return;PollTime=0;
 JustHarvested.Reset();for(auto& Pair:MushroomVisibility)if(auto* Mesh=Pair.Key.Get()){const bool Hidden=Mesh->GetOwner()->IsHidden();if(Hidden && !Pair.Value && Mesh->GetStaticMesh())JustHarvested.Add(HarvestMagic::BaseMushroom(Mesh->GetStaticMesh()->GetName()),Mesh);Pair.Value=Hidden;}
 const auto Bag=ReadBag();for(const auto& Pair:Bag){if(!Pair.Key.StartsWith(TEXT("SM_Env_Mushroom")))continue;const int Delta=Pair.Value-Previous.FindRef(Pair.Key);if(Delta>0)Harvest(Pair.Key,Delta);}Previous=ReadBag();
}
TStatId UForestHarvestMagic::GetStatId() const{RETURN_QUICK_DECLARE_CYCLE_STAT(UForestHarvestMagic,STATGROUP_Tickables);}
void UForestHarvestMagic::Deinitialize(){for(auto& R:Requests)if(R)R->Owner.Reset();Requests.Reset();for(auto& V:Visuals)if(V.Actor.IsValid())V.Actor->Destroy();Visuals.Reset();if(auto* PC=Player.Get())if(Input)PC->PopInputComponent(Input);if(Inventory)Inventory->RemoveFromParent();if(NoticeWidget)NoticeWidget->RemoveFromParent();Super::Deinitialize();}
