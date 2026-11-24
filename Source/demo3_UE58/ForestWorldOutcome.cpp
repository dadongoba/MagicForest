#include "ForestWorldOutcome.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
namespace
{
struct FWorldRules
{
 bool Valid=false;FString Error;ForestOutcomePolicy::Bands Bands;TSharedPtr<FJsonObject> Root;
 FWorldRules()
 {
  FString Text;
  if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectConfigDir()/TEXT("ForestWorldRules.json"))) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root) || !Root){Error=TEXT("次日世界规则表无法读取。");return;}
  const TSharedPtr<FJsonObject>* B=nullptr;const TArray<TSharedPtr<FJsonValue>> *Transitions=nullptr,*States=nullptr,*Tasks=nullptr;
  double Rev=0;
  if(!Root->TryGetNumberField(TEXT("Revision"),Rev)||Rev!=1 || !Root->TryGetObjectField(TEXT("Bands"),B) || !Root->TryGetArrayField(TEXT("Transitions"),Transitions) || Transitions->Num()!=4 || !Root->TryGetArrayField(TEXT("States"),States)||States->Num()!=4 || !Root->TryGetArrayField(TEXT("Commissions"),Tasks)||Tasks->Num()!=9){Error=TEXT("次日世界规则表结构不完整。");return;}
  const TCHAR* Keys[]={TEXT("PerfectCos"),TEXT("PerfectDose"),TEXT("GoodMin"),TEXT("GoodMax"),TEXT("GoodSide"),TEXT("FairMin"),TEXT("FairMax"),TEXT("FairSide")};
  double* Values[]={&Bands.PerfectCos,&Bands.PerfectDose,&Bands.GoodMin,&Bands.GoodMax,&Bands.GoodSide,&Bands.FairMin,&Bands.FairMax,&Bands.FairSide};
  for(int I=0;I<8;++I)if(!(*B)->TryGetNumberField(Keys[I],*Values[I])||!FMath::IsFinite(*Values[I])){Error=TEXT("评分阈值无效。");return;}
  if(!(Bands.PerfectCos>=0 && Bands.PerfectCos<1 && Bands.PerfectDose>=0 && Bands.PerfectDose<1 && Bands.FairMin>0 && Bands.FairMin<=Bands.GoodMin && Bands.GoodMin<=1-Bands.PerfectDose && Bands.GoodMax>=1+Bands.PerfectDose && Bands.GoodMax<=Bands.FairMax && Bands.GoodSide>=0 && Bands.GoodSide<=Bands.FairSide)){Error=TEXT("评分区间相互冲突。");return;}
  for(const auto& V:*Transitions){const TArray<TSharedPtr<FJsonValue>>* Row=nullptr;if(!V->TryGetArray(Row)||Row->Num()!=5){Error=TEXT("状态转移表不完整。");return;}for(const auto& C:*Row){double N=0;if(!C->TryGetNumber(N)||N<0||N>3||N!=FMath::FloorToDouble(N)){Error=TEXT("状态转移值无效。");return;}}}
  for(int I=0;I<4;++I){auto S=(*States)[I]->AsObject();double Id=-1,A=-1,BScale=-1;FString Name;if(!S||!S->TryGetNumberField(TEXT("Id"),Id)||Id!=I||!S->TryGetNumberField(TEXT("AvailabilityA"),A)||!S->TryGetNumberField(TEXT("AvailabilityB"),BScale)||A<.4||A>1||BScale<.85||BScale>1||!S->TryGetStringField(TEXT("Name"),Name)||Name.IsEmpty()){Error=TEXT("资源状态表无效。");return;}}
  TSet<int> Seen;
  for(const auto& V:*Tasks){auto T=V->AsObject();double Id=0;FString Persona,Region;if(!T||!T->TryGetNumberField(TEXT("Id"),Id)||Id<1||Id>9||Id!=FMath::FloorToDouble(Id)||Seen.Contains(Id)||!T->TryGetStringField(TEXT("Persona"),Persona)||!T->TryGetStringField(TEXT("Region"),Region)||FName(*Persona)!=ForestOutcome::PersonaForCommission(Id)||Region!=(Id<=3?TEXT("fire"):Id<=6?TEXT("wind"):TEXT("water"))){Error=TEXT("委托归属无效。");return;}Seen.Add(Id);
   for(const TCHAR* K:{TEXT("Success"),TEXT("Partial"),TEXT("Terminal"),TEXT("Continuity"),TEXT("Refusal")}){FString S;if(!T->TryGetStringField(K,S)||S.IsEmpty()||S.Len()>300){Error=TEXT("委托后果文案不完整。");return;}}
   const TSharedPtr<FJsonObject>* D=nullptr;if(!T->TryGetObjectField(TEXT("Deviations"),D)){Error=TEXT("缺少委托偏差事实。");return;}
  }
  Valid=true;
 }
};
const FWorldRules& Rules(){static const FWorldRules R;return R;}
}
namespace ForestOutcome
{
bool Ready(FString& Error){Error=Rules().Error;return Rules().Valid;}
FName Canonical(FName Id){return Id==TEXT("natta")?FName(TEXT("blacksmith")):Id==TEXT("fawnia")?FName(TEXT("operator")):Id==TEXT("forest_witch")?FName(TEXT("novelist")):Id;}
FName PersonaForCommission(int32 Id){return Id<1||Id>9?NAME_None:Id<=3?FName(TEXT("blacksmith")):Id<=6?FName(TEXT("operator")):FName(TEXT("novelist"));}
FString GradeName(int32 G){const TCHAR* N[]={TEXT("精确完成"),TEXT("完成"),TEXT("有偏差"),TEXT("明显偏差"),TEXT("委托线终止")};return N[FMath::Clamp(G,0,4)];}
FString StateName(int32 S){return Rules().Valid?Rules().Root->GetArrayField(TEXT("States"))[FMath::Clamp(S,0,3)]->AsObject()->GetStringField(TEXT("Name")):TEXT("规则未就绪");}
bool Rate(FAlchemyEvaluationResult& R,int32& Grade)
{
 if(!Rules().Valid || !R.bValid || !FMath::IsFinite(R.DoseRatio)||!FMath::IsFinite(R.Cosine)||!FMath::IsFinite(R.Projection)||!FMath::IsFinite(R.PerpendicularCost))return false;
 const int32 T[]={R.Target.Fire,R.Target.Wind,R.Target.Water},A[]={R.Actual.Fire,R.Actual.Wind,R.Actual.Water};
 bool Missing=false;for(int I=0;I<3;++I)Missing|=T[I]>0 && A[I]<=0;
 const bool Unused=R.VerdictCode==TEXT("contract_breach")||R.VerdictCode==TEXT("caput_mortuum")||R.VerdictCode==TEXT("empty_bottle");
 const bool Failed=Unused||R.Projection<=0||R.VerdictCode==TEXT("reversed_effect");
 Grade=ForestOutcomePolicy::Grade(Rules().Bands,R.DoseRatio,R.PerpendicularCost/FMath::Max(.0001f,FMath::Abs(R.Projection)),R.Cosine,Failed,Missing);
 if(!Failed)R.VerdictCode=Grade==0?TEXT("magnum_opus"):Grade==1?TEXT("deliverable"):R.DoseRatio<Rules().Bands.GoodMin?TEXT("under_dosed"):R.DoseRatio>Rules().Bands.GoodMax?TEXT("over_dosed"):TEXT("side_effect");
 R.Verdict=GradeName(Grade);R.Reward=Grade==0?3:Grade==1?2:Grade==2?1:0;
 R.Reason=Grade==0?TEXT("方向和剂量都贴合委托。") : Grade==1?TEXT("药效足以顺利完成工作，偏差较小。") : Grade==2?TEXT("已有所需药效，但剂量或配比有偏差。") : Grade==3?TEXT("关键效果不足、过强或比例失衡，工作仍需补救；后续委托继续。") : TEXT("本次托付彻底失败，人物不再提出后续委托。");
 return true;
}
bool Build(int32 Id,const FAlchemyEvaluationResult& Result,int32 Previous,FForestWorldOutcome& O)
{
 FString Error;if(!Ready(Error)||Id<1||Id>9)return false;
 auto R=Result;int Grade=0;if(!Rate(R,Grade))return false;
 TSharedPtr<FJsonObject> Task;for(const auto& V:Rules().Root->GetArrayField(TEXT("Commissions")))if(V->AsObject()->GetIntegerField(TEXT("Id"))==Id)Task=V->AsObject();if(!Task)return false;
 O.Revision=1;O.Grade=Grade;O.PreviousState=FMath::Clamp(Previous,0,3);O.NextState=Rules().Root->GetArrayField(TEXT("Transitions"))[O.PreviousState]->AsArray()[Grade]->AsNumber();O.bDisconnected=Grade==4;
 O.Persona=FName(*Task->GetStringField(TEXT("Persona")));O.Region=FName(*Task->GetStringField(TEXT("Region")));
 const auto State=Rules().Root->GetArrayField(TEXT("States"))[O.NextState]->AsObject();O.AvailabilityA=State->GetNumberField(TEXT("AvailabilityA"));O.AvailabilityB=State->GetNumberField(TEXT("AvailabilityB"));
 const int32 T[]={R.Target.Fire,R.Target.Wind,R.Target.Water},A[]={R.Actual.Fire,R.Actual.Wind,R.Actual.Water};const TCHAR* Elements[]={TEXT("fire"),TEXT("wind"),TEXT("water")};
 TArray<FString> Details;auto Deviations=Task->GetObjectField(TEXT("Deviations"));
 for(int I=0;I<3;++I)if(A[I]!=T[I]){const FName Tag(*(FString(Elements[I])+(A[I]<T[I]?TEXT("_short"):TEXT("_excess"))));O.Tags.Add(Tag);FString Fact;if(Deviations->TryGetStringField(Tag.ToString(),Fact))Details.Add(Fact);else Details.Add(FString(I==0?TEXT("热度"):I==1?TEXT("气流"):TEXT("湿润程度"))+(A[I]<T[I]?TEXT("低于所需，干扰了本次工序。"):TEXT("高于所需，干扰了本次工序。")));}
 const bool Unused=Result.VerdictCode==TEXT("caput_mortuum")||Result.VerdictCode==TEXT("empty_bottle")||Result.VerdictCode==TEXT("contract_breach");
 O.TaskFact=Grade<=1?Task->GetStringField(TEXT("Success")):Unused?TEXT("药剂不可用，未能投入委托工作；没有获得需要的帮助，这件事未能完成。"):Task->GetStringField(TEXT("Partial"))+TEXT(" ")+FString::Join(Details,TEXT(" "));
 O.NpcFact=O.bDisconnected?Task->GetStringField(TEXT("Terminal")):Grade<=1?TEXT("记得玩家具体帮成了什么，表达符合性格的认可；仍会提出当天的新委托。"):TEXT("记得玩家造成的具体偏差，可以指责和不满；仍然提出当天的新委托，不把失误说成委托线中断。");
 O.Continuity=O.bDisconnected?TEXT("此后不再提出、暗示、接受任何新委托；仍可交谈、寻找、展示物品，回应须承认这次损失并拒绝再托付。不得自行原谅后恢复委托。"):Task->GetStringField(TEXT("Continuity"));
 O.Fallback=O.bDisconnected?Task->GetStringField(TEXT("Refusal")):Grade<=1?TEXT("上回托你的事确实帮上了。今天还有一件需要你帮忙，先听我把用途说清楚。"):TEXT("上回的事没做好，留下的问题我记着。今天这件仍想请你帮忙，你先听清楚要达到什么效果。");
 const double Side=R.PerpendicularCost/FMath::Max(.0001f,FMath::Abs(R.Projection));
 O.Visual=O.NextState==0?TEXT("normal"):Unused?TEXT("weakened"):Grade==4?TEXT("inactive"):Side>Rules().Bands.GoodSide?TEXT("disturbed"):R.DoseRatio>Rules().Bands.GoodMax?TEXT("overgrown"):R.DoseRatio<Rules().Bands.GoodMin?TEXT("weakened"):TEXT("weakened");
 return true;
}
FString Summary(const FForestWorldOutcome& O,bool FinalNight,const FString& PlayerNotice)
{
 if(!O.Revision)return TEXT("");const FString Region=O.Region==TEXT("fire")?TEXT("火区"):O.Region==TEXT("wind")?TEXT("风区"):TEXT("水区");
 const FString Name=O.Persona==TEXT("blacksmith")?TEXT("铁匠"):O.Persona==TEXT("operator")?TEXT("接线员"):TEXT("小说家");
 const FString Consequence=O.bDisconnected?TEXT("你搞砸了这次托付。")+Name+TEXT("不会再向你提出新的委托。你仍然可以找到对方、交谈或展示物品。\n\n"):TEXT("");
 return (O.bDisconnected && !PlayerNotice.IsEmpty()?PlayerNotice+TEXT("\n\n"):Consequence)+FString::Printf(TEXT("%s：%s → %s。%s"),*Region,*StateName(O.PreviousState),*StateName(O.NextState),FinalNight?TEXT("此状态计入旅程结局，不生成第四天资源。"):*FString::Printf(TEXT("次日 A 区约 %.0f%%、B 区约 %.0f%% 的普通采集点可用，入口保底不受影响。"),O.AvailabilityA*100,O.AvailabilityB*100));
}
FString Memory(const FForestWorldOutcome& O)
{return O.TaskFact+TEXT("\n人物态度：")+O.NpcFact+TEXT("\n承接约束：")+O.Continuity;}
}
