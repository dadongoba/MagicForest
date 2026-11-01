#include "../../Plugins/DialogueMemoryTable/Source/DialogueMemoryTable/Public/AlchemyCampaignPolicy.h"
#include <cassert>
#include <iostream>
int main()
{
 using namespace AlchemyCampaign;
 State S;
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
 std::cout << "Campaign policy checks passed\n";
}
