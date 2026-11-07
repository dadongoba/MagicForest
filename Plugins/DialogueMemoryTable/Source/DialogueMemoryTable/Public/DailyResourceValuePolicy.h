#pragma once
#include "CoreMinimal.h"
#include "Misc/Crc.h"
#include "Math/RandomStream.h"

// Stable per-resource/day stream, independent of placement and AI randomness.
namespace DailyResourceValuePolicy
{
 inline bool ValidRange(int32 Minimum,int32 Maximum)
 {return Minimum>=1 && Minimum<=Maximum && Maximum<=9;}

 inline int32 Roll(int32 Day,const FString& ResourceId,int32 Element,int32 Salt,int32 Minimum,int32 Maximum)
 {
  if(Day<1 || ResourceId.IsEmpty() || Element<0 || Element>2 || !ValidRange(Minimum,Maximum))return 0;
  const FString Key=FString::Printf(TEXT("%d|%d|%d|%s"),Salt,Day,Element,*ResourceId);
  FRandomStream Random(static_cast<int32>(FCrc::StrCrc32(*Key)));
  return Random.RandRange(Minimum,Maximum);
 }
}
