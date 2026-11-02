#include "ForestAlchemyGuide.h"
#include "AlchemyGameplaySubsystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

namespace ForestAlchemyGuide
{
 namespace
 {
  TSharedPtr<FJsonObject> Read()
  {
   FString Text;TSharedPtr<FJsonObject> Root;
   if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectConfigDir()/TEXT("ForestMajorGuide.json"))) ||
    !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root))return nullptr;
   return Root;
  }
  FString Field(const TCHAR* Name)
  {const auto Root=Read();FString Text;return Root.IsValid() && Root->TryGetStringField(Name,Text)?Text:TEXT("大牌手札暂时无法读取，请检查书页配置。");}
 }
 FString Introduction(){return Field(TEXT("introduction"));}
 FString Notes(){return Field(TEXT("notes"));}
 FString VectorText(int32 Fire,int32 Wind,int32 Water)
 {return FString::Printf(TEXT("火 %d · 风 %d · 水 %d"),Fire,Wind,Water);}
 bool Major(const UAlchemyGameplaySubsystem* Gameplay,FName Id,FForestMajorGuide& Out)
 {
  Out={};const auto Root=Read();const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
  if(!Gameplay || !Root.IsValid() || !Root->TryGetArrayField(TEXT("entries"),Rows))return false;
  const auto Definitions=Gameplay->GetMajorCards();
  for(const auto& Value:*Rows)
  {
   const TSharedPtr<FJsonObject>* Row=nullptr;FString CardId;
   if(!Value.IsValid() || !Value->TryGetObject(Row) || !Row || !Row->IsValid() || !(*Row)->TryGetStringField(TEXT("id"),CardId) || FName(*CardId)!=Id)continue;
   const TArray<TSharedPtr<FJsonValue>>* Example=nullptr;
   if(!(*Row)->TryGetStringField(TEXT("howToUse"),Out.HowToUse) || !(*Row)->TryGetStringField(TEXT("reverse"),Out.Reverse) ||
    !(*Row)->TryGetStringField(TEXT("caution"),Out.Caution) || !(*Row)->TryGetArrayField(TEXT("example"),Example) || Example->IsEmpty())return false;
   TArray<FAlchemyCardSequenceItem> Sequence;TArray<FString> Labels;
   for(const auto& TokenValue:*Example)
   {
    FString Token;if(!TokenValue.IsValid() || !TokenValue->TryGetString(Token))return false;
    FAlchemyCardSequenceItem Item;FString Element,Number;
    if(Token.Split(TEXT(":"),&Element,&Number))
    {
     if((Element!=TEXT("fire") && Element!=TEXT("wind") && Element!=TEXT("water")) || !Number.IsNumeric())return false;
     Item.Type=EAlchemyCardType::Pip;Item.PipValue=FCString::Atoi(*Number);if(Item.PipValue<1 || Item.PipValue>9)return false;
     Item.Element=Element==TEXT("fire")?EAlchemyElement::Fire:Element==TEXT("wind")?EAlchemyElement::Wind:EAlchemyElement::Water;
     const FString Name=Element==TEXT("fire")?TEXT("火"):Element==TEXT("wind")?TEXT("风"):TEXT("水");
     Item.CardId=FName(*FString::Printf(TEXT("Guide_%d"),Sequence.Num()));Labels.Add(Name+Number);
    }
    else
    {
     const auto* D=Definitions.FindByPredicate([&Token](const auto& C){return C.Id==FName(*Token);});if(!D)return false;
     Item.CardId=D->Id;Item.Type=D->Type;Item.Cost=D->Cost;Item.BracketLength=D->BracketLength;Item.Element=D->Element;Item.PipValue=D->PipValue;Labels.Add(D->Name);
    }
    Sequence.Add(Item);
   }
   FAlchemyVector Result;int32 Cost=0;
   if(!Gameplay->PreviewAlchemySequence(Sequence,Result,Cost))return false;
   Out.Example=FString::Join(Labels,TEXT(" → "))+TEXT("\n得到：")+VectorText(Result.Fire,Result.Wind,Result.Water)+FString::Printf(TEXT("\n示例使用 %d 个槽位、%d 点额度。"),Sequence.Num(),Cost);
   return true;
  }
  return false;
 }
}
