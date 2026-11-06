#pragma once

// Engine-independent rules shared by the runtime and regression host.
namespace AlchemyDialogueTurnPolicy
{
 inline bool IsFresh(int Generation,int ActiveGeneration,int Day,int ActiveDay,bool SameNpc,bool Active)
 {return Active && SameNpc && Generation==ActiveGeneration && Day==ActiveDay;}
 inline bool MayPromote(int Current,int Requested,bool DirectEvidence,bool PersonalQuestion,bool Presentation)
 {return !Presentation && PersonalQuestion && DirectEvidence && Requested>Current && Requested<=3 && Current>=1;}
 template<class Facts> bool ValidateDelivered(const Facts& Allowed,const Facts& Delivered)
 {for(const auto& Id:Delivered)if(!Allowed.Contains(Id))return false;return true;}
 template<class Facts> void CommitDelivered(const Facts& Allowed,const Facts& Delivered,Facts& Known,bool Displayed)
 {if(Displayed && ValidateDelivered(Allowed,Delivered))for(const auto& Id:Delivered)Known.AddUnique(Id);}
 template<class Facts> bool WithinNewFactBudget(const Facts& Selected,const Facts& Known,int Budget)
 {int New=0;for(const auto& Id:Selected)if(!Known.Contains(Id))++New;return New<=Budget;}
 template<class Facts> bool ValidateBindings(const Facts& Selected,const Facts& Known,const Facts& Effects,const Facts& Bindings)
 {
  if(Effects.Num()!=Bindings.Num())return false;
  for(int I=0;I<Effects.Num();++I)if(Selected.Contains(Effects[I]) && !Selected.Contains(Bindings[I]) && !Known.Contains(Bindings[I]))return false;
  return true;
 }
}
