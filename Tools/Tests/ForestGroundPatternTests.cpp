#include "ForestGroundPattern.h"
#include "ForestZonePolicy.h"
#include <cassert>
#include <iostream>
int main()
{
 using namespace ForestGroundPattern;
 auto Center=WorldPoint(.5,.5,4000,10000,20000);assert(Center.X==10000 && Center.Y==20000);
 auto Corner=WorldPoint(1,0,4000,10000,20000);assert(Corner.X==14000 && Corner.Y==16000);
 // A sector with an embedded protected circle, simulating the authoritative membership callback.
 auto Inside=[](double X,double Y){return ForestZonePolicy::Classify(X,Y,.5)==ForestZonePolicy::Part::A && std::hypot(X+1200,Y-1500)>500;};
 int Painted=0,Blocked=0;
 for(int V=0;V<256;++V)for(int U=0;U<256;++U)
 {
  auto P=WorldPoint((U+.5)/256,(V+.5)/256,4000,0,0);
  auto A=Coverage(P.X,P.Y,0,0,4000,8000./256,Inside);
  assert(A>=0 && A<=1);
  if(!Inside(P.X,P.Y) || std::hypot(P.X,P.Y)>=4000){assert(A==0);++Blocked;}
  if(A>0)++Painted;
  auto M=Mottle(P.X,P.Y,true);assert(M>=.32 && M<=1 && M==Mottle(P.X,P.Y,true));
 }
 assert(Painted>10000 && Blocked>30000);
 assert(Coverage(-1200,1500,0,0,4000,31.25,Inside)==0);
 // Near a sector edge stays empty rather than leaking under bilinear sampling.
 assert(Coverage(-2000,1,0,0,4000,31.25,Inside)==0);
 std::cout<<"PASS: projected UV orientation, full-area coverage, protected holes, sector edges, deterministic mottling\n";
}
