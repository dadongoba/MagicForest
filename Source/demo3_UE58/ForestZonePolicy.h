#pragma once
#include <cmath>
#include <cstdint>

namespace ForestZonePolicy
{
 enum class Part { Entry, A, B };
 inline Part Classify(double Along,double Side,double HalfAngleCos)
 {
  const double Length=std::hypot(Along,Side);
  if(Length<1.e-8 || Along>=Length*HalfAngleCos-1.e-8)return Part::Entry;
  return Side>=0?Part::A:Part::B;
 }
 inline bool Available(bool Protected,bool HarvestEnabled,double Scale,std::uint32_t Hash,int Ordinal,int Maximum)
 {
  if(Protected)return true;
  if(!HarvestEnabled || !std::isfinite(Scale) || Scale<=0)return false;
  if(Maximum>=0 && Ordinal>=Maximum)return false;
  return Scale>=1 || double(Hash)/4294967296.0<Scale;
 }
 struct Presentation {bool Hidden,Collision,Tick;};
 inline Presentation ResourcePresentation(bool Collected,bool Available,bool HideUnavailable,bool Water)
 {
  const bool Hidden=Collected || (!Available && HideUnavailable);
  return {Hidden,!Collected && Available,Water && !Collected && !Hidden};
 }

}
