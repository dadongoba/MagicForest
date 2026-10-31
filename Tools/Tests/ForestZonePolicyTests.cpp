#include "ForestZonePolicy.h"
#include <cassert>
#include <iostream>
#include <limits>
#include <vector>
int main()
{
 using namespace ForestZonePolicy;
 constexpr double Cos60=.5;
 assert(Classify(10,0,Cos60)==Part::Entry);
 assert(Classify(0,10,Cos60)==Part::A);
 assert(Classify(0,-10,Cos60)==Part::B);
 assert(Classify(-10,0,Cos60)==Part::A); // Stable back boundary tie.
 assert(Classify(0,0,Cos60)==Part::Entry);
 assert(Classify(1,std::sqrt(3.),Cos60)==Part::Entry);
 // Neutral state leaves every existing candidate available at every hash.
 const std::vector<std::uint32_t> Hashes={0,1,0x7fffffff,0x80000000,0xffffffff};
 for(auto Hash:Hashes)for(int I=0;I<150;++I)assert(Available(false,true,1,Hash,I,-1));
 assert(!Available(false,false,1,0,0,-1));
 assert(!Available(false,true,0,0,0,-1));
 assert(Available(false,true,.5,0x7fffffff,0,-1));
 assert(!Available(false,true,.5,0x80000000,0,-1));
 assert(Available(false,true,1,0,1,2));
 assert(!Available(false,true,1,0,2,2));
 assert(Available(true,false,0,0xffffffff,150,0)); // Entry/reserve protection.
 assert(!Available(false,true,std::numeric_limits<double>::quiet_NaN(),0,0,-1));
 // A neighbouring zone's state is independent: closing A does not close B.
 assert(!Available(false,false,1,0,0,-1));
 assert(Available(false,true,1,0,0,-1));
 const auto NormalShell=ResourcePresentation(false,true,true,true);
 assert(!NormalShell.Hidden && NormalShell.Collision && NormalShell.Tick);
 const auto Inactive=ResourcePresentation(false,false,true,true);
 assert(Inactive.Hidden && !Inactive.Collision && !Inactive.Tick);
 const auto VisibleInactive=ResourcePresentation(false,false,false,true);
 assert(!VisibleInactive.Hidden && !VisibleInactive.Collision && VisibleInactive.Tick);
 const auto SpentAfterReset=ResourcePresentation(true,true,true,true);
 assert(SpentAfterReset.Hidden && !SpentAfterReset.Collision && !SpentAfterReset.Tick);
 std::cout<<"PASS: neutral candidates, sector boundaries, independent controls, reserves, caps\n";
}
