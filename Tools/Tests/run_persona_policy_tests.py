#!/usr/bin/env python3
"""Run production persona factories and text policies with a small STL shim.

Does not launch UE or call an LLM. JSON patch merging and real saves need UE QA.
"""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
PRIVATE = ROOT / 'Plugins/DialogueMemoryTable/Source/DialogueMemoryTable/Private'
policy = (PRIVATE / 'AlchemyPersonaPolicy.cpp').read_text()
profiles = (PRIVATE / 'AlchemyPersonaProfiles.inl').read_text()
gameplay = (PRIVATE / 'AlchemyGameplaySubsystem.cpp').read_text()


def function(name, source=policy):
    match = re.search(r'(?:FName|bool|FString|int32) ' + re.escape(name) + r'\(', source)
    assert match, name
    start = source.index('{', match.start())
    depth, end = 1, start + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end]


shim = r'''
#include <algorithm>
#include <cassert>
#include <cwctype>
#include <iostream>
#include <regex>
#include <string>
#include <vector>
#define TEXT(x) L##x
using int32=int;
enum class ESearchCase {CaseSensitive,IgnoreCase};
struct FString:std::wstring {
 using std::wstring::wstring;using std::wstring::operator=;
 FString(const std::wstring& x):std::wstring(x){}
 const wchar_t* operator*()const{return c_str();}
 void Reset(){clear();}
 template<class... A>static FString Printf(const wchar_t*,A...){return TEXT("matched");}
 bool IsEmpty()const{return empty();}
 bool Contains(FString x,ESearchCase c=ESearchCase::IgnoreCase)const{
  std::wstring a=*this,b=x;
  if(c==ESearchCase::IgnoreCase){for(auto& v:a)v=std::towlower(v);for(auto& v:b)v=std::towlower(v);}
  return a.find(b)!=std::wstring::npos;
 }
 void RemoveAt(int a,int n){erase(a,n);}
 FString Left(int n)const{return substr(0,n);}
 void TrimStartAndEndInline(){while(!empty()&&std::iswspace(front()))erase(0,1);while(!empty()&&std::iswspace(back()))pop_back();}
};
using FName=std::wstring;
template<class T> struct TArray:std::vector<T>{
 using std::vector<T>::vector;using std::vector<T>::operator=;
 int Num()const{return this->size();}
 bool IsValidIndex(int i)const{return i>=0&&i<Num();}
 template<class... A>void Emplace(A&&... a){this->emplace_back(std::forward<A>(a)...);}
};
template<class A,class B>struct TPair{A Key;B Value;TPair(A a,B b):Key(a),Value(b){}};
struct FRegexPattern{std::wregex P;FRegexPattern(const wchar_t* p):P(p){}};
struct FRegexMatcher{
 std::wstring Value;std::wsregex_iterator Next,End;std::wsmatch Match;
 FRegexMatcher(const FRegexPattern& p,const FString& text):Value(text),Next(Value.begin(),Value.end(),p.P){}
 bool FindNext(){if(Next==End)return false;Match=*Next++;return true;}
 int GetMatchBeginning()const{return Match.position();}
 int GetMatchEnding()const{return Match.position()+Match.length();}
};
struct FDialoguePersonaLayer{
 int FamiliarityLevel=1;FString Title,Summary,Prompt;bool bUnlockedByDefault=false;
 TArray<FString>TriggerKeywords,TriggerTopicKeywords,TriggerIntentKeywords;
};
struct FDialoguePersonaProfile{
 FName PersonaId;FString Name,Title,Axis,VoiceTone,DailyState,SecretPast;
 TArray<FString>QuickTags;TArray<FDialoguePersonaLayer>Layers;
};
struct UAlchemyGameplaySubsystem{
 struct EmptyOverrides{const FDialoguePersonaProfile* Find(const FName&)const{return nullptr;}}CustomPersonas;
 static FDialoguePersonaProfile MakeBlacksmithProfile();
 static FDialoguePersonaProfile MakeOperatorProfile();
 static FDialoguePersonaProfile MakeNovelistProfile();
 bool GetPersonaProfile(FName,FDialoguePersonaProfile&)const;
 int32 EvaluatePersonaTrigger(FName,const FString&,int32,int32&,FString&)const;
};
struct FMath{static int Clamp(int x,int a,int b){return std::clamp(x,a,b);}};
'''

cases = r'''
int main(){
 using namespace AlchemyPersonaPolicy;
 assert(CanonicalId(TEXT("natta"))==TEXT("blacksmith"));
 assert(CanonicalId(TEXT("fawnia"))==TEXT("operator"));
 assert(CanonicalId(TEXT("forest_witch"))==TEXT("novelist"));
 const auto B=UAlchemyGameplaySubsystem::MakeBlacksmithProfile();
 const auto O=UAlchemyGameplaySubsystem::MakeOperatorProfile();
 const auto N=UAlchemyGameplaySubsystem::MakeNovelistProfile();
 for(const auto& p:{B,O,N}){
  assert(p.Layers.Num()==3 && p.Layers[0].bUnlockedByDefault);
  assert(!ContainsRetiredBinding(p.PersonaId,p.SecretPast));
  for(int i=0;i<3;++i){
   assert(p.Layers[i].FamiliarityLevel==i+1);
   assert(!p.Layers[i].Prompt.Contains(TEXT("我的老师")));
   assert(!p.Layers[i].Prompt.Contains(TEXT("标记了一个同事")));
   assert(!p.Layers[i].Prompt.Contains(TEXT("说话极少、极静")));
  }
 }
 assert(B.VoiceTone.Contains(TEXT("脾气不好")));
 assert(B.Layers[0].Prompt.Contains(TEXT("评价"))||B.Layers[0].Prompt.Contains(TEXT("泛泛")));
 assert(B.Layers[2].Prompt.Contains(TEXT("当时还认为事情做对了")));
 assert(O.VoiceTone.Contains(TEXT("活泼")) && O.VoiceTone.Contains(TEXT("健忘")));
 assert(O.Layers[2].Prompt.Contains(TEXT("当晚仍睡得很好")));
 assert(O.Layers[2].Prompt.Contains(TEXT("不永久沉默")));
 assert(N.VoiceTone.Contains(TEXT("轻浮多情")) && N.Layers[0].Prompt.Contains(TEXT("暧昧")));
 assert(N.SecretPast.Contains(TEXT("一本样书都没带出来")));
 assert(N.Layers[2].Prompt.Contains(TEXT("作者身份不当")));
 assert(ContainsRetiredBinding(TEXT("natta"),TEXT("她拆过老师家的炉子")));
 assert(ContainsRetiredBinding(TEXT("blacksmith"),TEXT("blacksmith.TEACHER_IDENTITY")));
 assert(ContainsRetiredBinding(TEXT("fawnia"),TEXT("标记同事后被带走")));
 assert(ContainsRetiredBinding(TEXT("operator"),TEXT("colleague.call")));
 assert(!ContainsRetiredBinding(TEXT("operator"),TEXT("标记人们谈话，有人被带走")));
 assert(!ContainsRetiredBinding(TEXT("novelist"),TEXT("审查员说你以后不用写了")));
 assert(ContextText(TEXT("natta"),TEXT("老师的炉子")).IsEmpty());
 assert(ContextText(TEXT("natta"),TEXT("玩家已经接下委托"))==TEXT("玩家已经接下委托"));
 assert(ContainsRetiredWorldBinding(TEXT("blacksmith.teacher_identity")));
 assert(ContainsRetiredWorldBinding(TEXT("接线员的同事")));
 assert(!ContainsRetiredWorldBinding(TEXT("北边区域关闭，玩家完成委托")));
 assert(CleanReply(TEXT("natta"),TEXT("（皱眉）先讲用途。[看了一眼]"))==TEXT("先讲用途。"));
 assert(CleanReply(TEXT("fawnia"),TEXT("*挥手*诶，你说。"))==TEXT("诶，你说。"));
 assert(CleanReply(TEXT("forest_witch"),TEXT("【微笑】")).Contains(TEXT("选词")));
 assert(CleanReply(TEXT("natta"),TEXT("   ")).Contains(TEXT("具体")));
 assert(CleanReply(TEXT("fawnia"),TEXT("（停顿）")).Contains(TEXT("先听")));
 assert(CleanReply(TEXT("operator"),TEXT("你说的三本书，我记得。"))==TEXT("你说的三本书，我记得。"));
 UAlchemyGameplaySubsystem Game;int Layer=0;FString Reason;
 assert(Game.EvaluatePersonaTrigger(TEXT("blacksmith"),TEXT("这份材料用来做什么"),1,Layer,Reason)==2 && Layer==2);
 assert(Game.EvaluatePersonaTrigger(TEXT("blacksmith"),TEXT("安置办"),1,Layer,Reason)==3 && Layer==3);
 assert(Game.EvaluatePersonaTrigger(TEXT("operator"),TEXT("这么大的风做什么"),1,Layer,Reason)==2);
 assert(Game.EvaluatePersonaTrigger(TEXT("operator"),TEXT("当晚睡得很好"),2,Layer,Reason)==3);
 assert(Game.EvaluatePersonaTrigger(TEXT("novelist"),TEXT("你以前也写这种东西吗"),1,Layer,Reason)==2);
 assert(Game.EvaluatePersonaTrigger(TEXT("novelist"),TEXT("三本书"),2,Layer,Reason)==2 && Layer==0);
 assert(Game.EvaluatePersonaTrigger(TEXT("novelist"),TEXT("作者身份不当"),2,Layer,Reason)==3);
 assert(Game.EvaluatePersonaTrigger(TEXT("blacksmith"),TEXT("老师"),1,Layer,Reason)==1);
 assert(Game.EvaluatePersonaTrigger(TEXT("operator"),TEXT("同事"),1,Layer,Reason)==1);
 assert(Game.EvaluatePersonaTrigger(TEXT("operator"),TEXT("你刚才忘了"),3,Layer,Reason)==3);
 std::cout<<"Production persona factories, aliases, retired context and reply cleanup passed\n";
}
'''

trigger_helpers = '\nnamespace AlchemyGameplay {\n' + function('FindKeyword', gameplay) + '\n' + function('MatchesLayerTrigger', gameplay) + '\n}\n'
trigger_methods = function('UAlchemyGameplaySubsystem::GetPersonaProfile', gameplay) + '\n' + function('UAlchemyGameplaySubsystem::EvaluatePersonaTrigger', gameplay)
source = shim + profiles + trigger_helpers + trigger_methods + '\nnamespace AlchemyPersonaPolicy {\n' + '\n'.join(
    function(name) for name in ['CanonicalId', 'ContainsRetiredBinding', 'ContextText', 'ContainsRetiredWorldBinding', 'CleanReply']
) + '\n}\n' + cases
with tempfile.TemporaryDirectory(prefix='forest-persona-') as directory:
    cpp = Path(directory) / 'test.cpp'
    exe = Path(directory) / 'test'
    cpp.write_text(source)
    subprocess.run(['c++', '-std=c++17', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
