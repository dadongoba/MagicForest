#include "../../Plugins/DialogueMemoryTable/Source/DialogueMemoryTable/Public/AlchemyCampaignPolicy.h"
#include <cassert>
#include <iostream>
int main()
{
 using namespace AlchemyCampaign;
 State S;
 for(int Day=1;Day<=3;++Day)for(int Mask=0;Mask<8;++Mask)
 {assert(AllDailyCommissionsResolved(Day,Mask&1,Mask&2,Mask&4)==(Mask==7));}
 assert(!AllDailyCommissionsResolved(0,true,true,true));
 assert(!AllDailyCommissionsResolved(4,true,true,true));
 assert(!Confirm(S,2));assert(!Confirm(S,0));assert(!Confirm(S,4));
 assert(Confirm(S,1) && S.Day==2 && !S.Completed);
 assert(Confirm(S,1) && S.Day==2);
 assert(Confirm(S,2) && S.Day==3 && !S.Completed);
 assert(Confirm(S,2) && S.Day==3);
 assert(Confirm(S,3) && S.Day==3 && S.Completed);
 assert(Confirm(S,3) && S.Day==3 && S.Completed);
 assert(Confirm(S,1) && S.Day==3 && S.Completed);
 assert(!Confirm(S,4));
 for(int Day=4;Day<=20;++Day){const auto M=Normalize({Day,Day-1,false});assert(M.Day==3 && M.Confirmed==3 && M.Completed);}
 const auto Final=Normalize({3,3,false});assert(Final.Completed && Final.Day==3);
 const auto Invalid=Normalize({-2,-1,false});assert(Invalid.Day==1 && Invalid.Confirmed==0);
 const auto Stale=Normalize({1,2,false});assert(Stale.Day==3 && !Stale.Completed);
 for(int Day=1;Day<=3;++Day){assert(CommissionId(0,Day)==Day);assert(CommissionId(3,Day)==3+Day);assert(CommissionId(6,Day)==6+Day);}
 assert(CommissionId(0,4)==0 && CommissionId(6,0)==0 && CommissionId(-1,1)==0);
 State Journey;
 for(int Day=1;Day<=3;++Day)
 {
  for(int Count=1;Count<=3;++Count)
  {
   if(AllDailyCommissionsResolved(Day,Count>=1,Count>=2,Count>=3))assert(Confirm(Journey,Day));
   assert(Journey.Day==(Count<3?Day:(Day<3?Day+1:3)));
   assert(Journey.Completed==(Day==3 && Count==3));
  }
 }
 assert(Journey.Completed);
 std::cout << "Campaign policy checks passed\n";
}
