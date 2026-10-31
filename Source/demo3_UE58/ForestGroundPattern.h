#pragma once
#include <cmath>
#include <algorithm>

namespace ForestGroundPattern
{
 // DeferredDecal.usf: UV=(local Z,local Y); pitch -90 maps these to world X,Y.
 struct Point {double X,Y;};
 inline Point WorldPoint(double U,double V,double Radius,double CenterX,double CenterY)
 {return {CenterX+(2*U-1)*Radius,CenterY+(2*V-1)*Radius};}
 template<class Membership>
 double Coverage(double X,double Y,double CenterX,double CenterY,double Radius,double PixelSize,Membership Inside)
 {
  const double Distance=std::hypot(X-CenterX,Y-CenterY);
  if(Distance>=Radius || !Inside(X,Y))return 0;
  // Erode by two texels: bilinear filtering cannot paint over a protected boundary.
  const double Margin=PixelSize*2;
  for(int I=0;I<8;++I)
  {const double A=I*3.141592653589793/4;if(!Inside(X+std::cos(A)*Margin,Y+std::sin(A)*Margin))return 0;}
  return std::clamp((Radius-Distance-Margin)/(PixelSize*3),0.0,1.0);
 }
 inline double Mottle(double X,double Y,bool Disturbed)
 {
  const double Large=std::sin(X*.004+std::sin(Y*.003))*std::cos(Y*.0035-X*.001);
  const double Fine=std::sin(X*.021+Y*.015)*std::cos(Y*.017);
  return std::clamp(.7+(Disturbed?.28:.15)*Large+.06*Fine,.32,1.0);
 }
}
