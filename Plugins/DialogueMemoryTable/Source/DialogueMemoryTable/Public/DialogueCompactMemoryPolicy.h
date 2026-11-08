#pragma once
// Engine-independent bounded merge policy, shared by runtime and native regression tests.
#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>
namespace DialogueCompactMemory
{
struct Fact {std::string owner,id,kind,text,evidence;int day=0;int64_t turn=0;};
struct Loop {std::string owner,id,kind,text,evidence,source,status="open";int day=0,commission=0;int64_t turn=0;};
struct State {std::string owner,note,evidence,revision;int day=0;int64_t turn=0;};
struct Store {std::vector<Fact> facts;std::vector<Loop> loops;std::vector<State> states;};
struct Delta {std::string owner,note,evidence,revision;int day=0;int64_t turn=0;bool hasNote=false;std::vector<Fact> facts;std::vector<Loop> loops;};
inline size_t Chars(const std::string& s){size_t n=0;for(unsigned char c:s)if((c&0xc0)!=0x80)++n;return n;}
inline bool FactKind(const std::string& k){return k=="preferred_name"||k=="preference"||k=="self_report"||k=="request";}
inline bool LoopKind(const std::string& k){return k=="question"||k=="dialogue_promise";}
inline bool ValidText(const std::string& s,size_t cap){return !s.empty()&&Chars(s)<=cap&&s.find('\n')==std::string::npos&&s.find('\r')==std::string::npos;}
inline bool Valid(const Fact& f){return !f.owner.empty()&&!f.id.empty()&&FactKind(f.kind)&&ValidText(f.text,60)&&ValidText(f.evidence,40)&&f.day>0&&f.turn>0;}
inline bool Valid(const Loop& l){return !l.owner.empty()&&!l.id.empty()&&(l.source=="user"||l.source=="assistant")&&LoopKind(l.kind)&&ValidText(l.text,60)&&ValidText(l.evidence,40)&&l.day>0&&l.turn>0&&(l.status=="open"||l.status=="resolved"||l.status=="cancelled");}
inline void Bound(Store& s,const std::string& owner)
{
 auto old=[](const auto& a,const auto& b){return a.day!=b.day?a.day<b.day:a.turn<b.turn;};
 // Exactly one current row for a fixed preference slot; no model-provided importance.
 std::stable_sort(s.facts.begin(),s.facts.end(),old);
 while(std::count_if(s.facts.begin(),s.facts.end(),[&](const Fact& f){return f.owner==owner;})>12)
 {auto i=std::find_if(s.facts.begin(),s.facts.end(),[&](const Fact& f){return f.owner==owner&&f.kind!="preferred_name";});if(i==s.facts.end())i=std::find_if(s.facts.begin(),s.facts.end(),[&](const Fact& f){return f.owner==owner;});s.facts.erase(i);}
 std::stable_sort(s.loops.begin(),s.loops.end(),old);
 while(std::count_if(s.loops.begin(),s.loops.end(),[&](const Loop& l){return l.owner==owner&&l.status!="open";})>4)
 {auto i=std::find_if(s.loops.begin(),s.loops.end(),[&](const Loop& l){return l.owner==owner&&l.status!="open";});s.loops.erase(i);}
}
inline bool Apply(Store& s,const Delta& d)
{
 if(d.owner.empty()||d.day<1||d.turn<1)return false;
 auto state=std::find_if(s.states.begin(),s.states.end(),[&](const State& x){return x.owner==d.owner;});
 if(state!=s.states.end()&&state->turn>=d.turn)return false;
 if(state==s.states.end()){s.states.push_back(State{});state=s.states.end()-1;state->owner=d.owner;}
 state->turn=d.turn;
 if(!d.facts.empty()&&!d.hasNote){state->note.clear();state->evidence.clear();}
 if(d.hasNote&&ValidText(d.note,80)&&ValidText(d.evidence,40)){state->note=d.note;state->evidence=d.evidence;state->day=d.day;state->revision=d.revision;}
 for(auto& l:s.loops)if(l.owner==d.owner&&l.commission>0&&l.day<d.day&&l.status=="open")l.status="cancelled";
 for(const auto& f:d.facts)
 {
  if(f.owner!=d.owner||!Valid(f))continue;
  auto i=std::find_if(s.facts.begin(),s.facts.end(),[&](const Fact& x){return x.owner==d.owner&&(x.id==f.id||(x.kind=="preferred_name"&&f.kind=="preferred_name"));});
  if(i==s.facts.end())i=std::find_if(s.facts.begin(),s.facts.end(),[&](const Fact& x){return x.owner==d.owner&&x.kind==f.kind&&x.text==f.text;});
  if(i==s.facts.end())s.facts.push_back(f);else {auto id=i->id;*i=f;i->id=id;}
 }
 for(const auto& l:d.loops)
 {
  if(l.owner!=d.owner||!Valid(l))continue;
  auto i=std::find_if(s.loops.begin(),s.loops.end(),[&](const Loop& x){return x.owner==d.owner&&x.id==l.id;});
  if(i==s.loops.end())i=std::find_if(s.loops.begin(),s.loops.end(),[&](const Loop& x){return x.owner==d.owner&&x.kind==l.kind&&x.text==l.text&&x.commission==l.commission;});
  if(i!=s.loops.end()){auto id=i->id;*i=l;i->id=id;continue;}
  // Refuse only the new row, never the summary or the rest of the transaction.
  auto active=std::count_if(s.loops.begin(),s.loops.end(),[&](const Loop& x){return x.owner==d.owner&&x.status=="open";});
  if(l.status=="open"&&active>=4)continue;
  if(l.status=="open")s.loops.push_back(l); // Cannot invent a closed record without an existing source.
 }
 Bound(s,d.owner);return true;
}
}
