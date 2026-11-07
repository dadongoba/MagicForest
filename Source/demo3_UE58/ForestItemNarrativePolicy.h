#pragma once
#include "CoreMinimal.h"

namespace ForestItemNarrativePolicy
{
 inline bool Valid(const FString& Name,const FString& Description)
 {
  if(Name.Len()<4 || Name.Len()>12 || Description.Len()<25 || Description.Len()>65)return false;
  const FString Text=Name+Description;
  for(const TCHAR* Marker:{TEXT("\n"),TEXT("\r"),TEXT("\t"),TEXT("```"),TEXT("{"),TEXT("}"),TEXT("<"),TEXT(">")})
   if(Text.Contains(Marker))return false;
  if(Name.Contains(TEXT("·")))return false;
  int32 Han=0;
  for(const TCHAR C:Name)if(C>=0x4e00 && C<=0x9fff)++Han;
  if(Han<4)return false;
  for(const TCHAR C:Text)if((C>=TEXT('0') && C<=TEXT('9')) || (C>=0xff10 && C<=0xff19))return false;
  for(const TCHAR* Stat:{TEXT("生命值"),TEXT("攻击力"),TEXT("冷却时间"),TEXT("伤害值"),TEXT("任务奖励"),TEXT("恢复体力"),TEXT("增加好感"),TEXT("提升好感"),TEXT("解锁任务")})
   if(Text.Contains(Stat))return false;
  return true;
 }

 // Metadata only. Inventory identity, quantity and authoritative attributes are
 // never rewritten by model output, including late or failed-save completions.
 template<class Item,class Save>
 bool Apply(Item& Row,const FString& Name,const FString& Description,bool ValidOutput,bool StillOwned,Save Persist)
 {
  const Item Before=Row;Row.bNamingPending=false;
  if(ValidOutput && StillOwned){Row.Name=Name;Row.Description=Description;Row.bGeneratedByAI=true;}
  if(Persist())return true;
  Row=Before;Row.bNamingPending=false;
  return false;
 }
}
