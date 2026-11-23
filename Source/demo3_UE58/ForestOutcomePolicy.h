#pragma once
#include <cmath>
// Portable production policy. Bands are supplied by the validated JSON table.
namespace ForestOutcomePolicy
{
struct Bands {double PerfectCos=.995,PerfectDose=.05,GoodMin=.8,GoodMax=1.2,GoodSide=.15,FairMin=.65,FairMax=1.4,FairSide=.45;};
inline int Grade(const Bands& B,double Dose,double Side,double Cos,bool SpecialFailure,bool MissingRequired)
{
 if(SpecialFailure)return 4;
 if(MissingRequired)return 3;
 constexpr double E=1.e-6;
 if(Cos>B.PerfectCos && std::abs(Dose-1.)<=B.PerfectDose+E)return 0;
 if(Dose>=B.GoodMin-E && Dose<=B.GoodMax+E && Side<=B.GoodSide+E)return 1;
 if(Dose>=B.FairMin-E && Dose<=B.FairMax+E && Side<=B.FairSide+E)return 2;
 return 3;
}
inline bool CountsForDay(int ReceiptDay,int QueryDay){return ReceiptDay>=1 && ReceiptDay<QueryDay;}
inline bool LineUnavailable(bool Disconnected,int ReceiptDay,int QueryDay){return Disconnected && CountsForDay(ReceiptDay,QueryDay);}
}
