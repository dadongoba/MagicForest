#!/usr/bin/env python3
"""Exercise production numeric/topic policies without starting UE or an LLM."""
from pathlib import Path
import json,re,subprocess,tempfile

ROOT=Path(__file__).resolve().parents[2]
BASE=ROOT/'Plugins/DialogueMemoryTable/Source/DialogueMemoryTable'
policy=(BASE/'Private/AlchemyDialogueKnowledge.cpp').read_text()
def function(name):
 m=re.search(r'(?:bool|int32|FString) '+name+r'\(',policy);assert m,name
 start=policy.index('{',m.start());depth=1;end=start+1
 while depth:
  depth+=(policy[end]=='{')-(policy[end]=='}');end+=1
 return policy[m.start():end]
shim=r'''
#include <algorithm>
#include <cassert>
#include <cwctype>
#include <iostream>
#include <regex>
#include <string>
#define TEXT(x) L##x
using TCHAR=wchar_t;using int32=int;
struct FString:std::wstring{
 using std::wstring::wstring;using std::wstring::operator=;
 FString(const std::wstring& s):std::wstring(s){}
 static FString Chr(wchar_t c){return FString(1,c);}
 void ReplaceInline(const wchar_t* a,const wchar_t* b){size_t p=0;while((p=find(a,p))!=npos){replace(p,std::wstring(a).size(),b);p+=std::wstring(b).size();}}
 bool Contains(const FString& x)const{return find(x)!=npos;}
 bool StartsWith(const wchar_t* x)const{return find(x)==0;}
 int Len()const{return size();}bool IsEmpty()const{return empty();}
 FString TrimStartAndEnd()const{size_t b=0,e=size();while(b<e&&std::iswspace((*this)[b]))++b;while(e>b&&std::iswspace((*this)[e-1]))--e;return substr(b,e-b);}
};
struct FRegexPattern{std::wregex P;FRegexPattern(FString p){p.ReplaceInline(TEXT("(?i)"),TEXT(""));P=std::wregex(p,std::regex::icase);}};
struct FRegexMatcher{const FRegexPattern& P;const FString& S;FRegexMatcher(const FRegexPattern& p,const FString& s):P(p),S(s){}bool FindNext(){return std::regex_search(S,P.P);}};
'''
cases=r'''
int main(){
 for(const auto* s:{TEXT("我要火6"),TEXT("火六风零水零"),TEXT("六"),TEXT("火性６"),TEXT("six fire"),TEXT("FIRE SIX"),TEXT("fire_six"),TEXT("VI"),TEXT("Ⅵ"),TEXT("⑥"),TEXT("⁶"),TEXT("火陆"),TEXT("火贰"),TEXT("[6,0,0]"),TEXT("只要十分火"),TEXT("0x6"),TEXT("火6，风不需要"),TEXT("三本书，所以火三"),TEXT("second rank"),TEXT("八成"),TEXT("两倍"),TEXT("我需要火６点"),TEXT(" ")})assert(!IsSafeReply(s));
 for(const auto* s:{TEXT("热要进到厚砖里面，把深处的潮气烘出来。"),TEXT("炉子已有通风，不额外助风。"),TEXT("先回小屋，在合成台调配。"),TEXT("她写过三本书，第二本书里有雨。"),TEXT("一本样书也没带出来。"),TEXT("一点风，把一阵闷气散开。"),TEXT("我想把这件事讲清楚。")})assert(IsSafeReply(s));
 for(const auto* s:{TEXT("对，就是这个数。"),TEXT("没错，你猜对了。"),TEXT("接近了，再多加点。"),TEXT("不对，太少了。")})assert(!IsSafeReplyForPlayer(s,TEXT("火6对吗")));
 assert(IsSafeReplyForPlayer(TEXT("先看炉壁里面是否干透，别只看表面。"),TEXT("火6对吗")));
 for(const auto* s:{TEXT("潮气闷在里面，一开炉就容易裂。"),TEXT("一受热就卷边。"),TEXT("字迹一擦就花，一碰就掉。"),TEXT("纸页一吹就飞。")})assert(IsSafeReply(s));
 assert(IsSafeReply(TEXT("一烘就坏。一吸就过头，一拎就破。这是两回事。")));
 assert(IsSafeReply(TEXT("烟道清一清，你先看一看，再试一试。")));
 assert(TopicBit(TEXT("system"))==0 && TopicBit(TEXT("all"))==0 && TopicBit(TEXT("fire"))==2);
 for(int mask=-1;mask<128;++mask){int result=LimitTopics(mask);assert(result>0 && result<64);int n=0;for(int b=0;b<6;++b)n+=bool(result&(1<<b));assert(n<=2);}
 std::cout<<"Production numeric guard, numeric guess rejection and bounded disclosure topics passed\n";
}
'''
codex=json.loads((ROOT/'Config/ForestElementCodex.json').read_text())
config=json.loads((ROOT/'Config/ForestCommissionKnowledge.json').read_text())
texts=[e['description'] for e in codex['entries']]
for entry in config['entries']:texts+=[entry['purpose'],*entry['topics'].values()]
checks='\n'.join('assert(IsSafeReply(TEXT('+json.dumps(t,ensure_ascii=True)+')));' for t in texts)
cases=cases.replace(' std::cout<<',checks+'\n std::cout<<')
source=shim+'\n'+ '\n'.join(function(n) for n in ['HasNumericDisclosure','IsSafeReply','IsSafeReplyForPlayer','TopicBit','LimitTopics'])+cases
with tempfile.TemporaryDirectory(prefix='forest-knowledge-') as d:
 p=Path(d);(p/'test.cpp').write_text(source)
 subprocess.run(['c++','-std=c++17',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)

assert len(config['entries'])==9
assert {e['commission_id'] for e in config['entries']}==set(range(1,10))
for e in config['entries']:
 assert e['approved'] and e['review_id']=='user-approved-2026-11-06-agent-narrative'
 assert len(e['target'])==3 and all(isinstance(v,int) and 0<=v<=10 for v in e['target'])
 assert sum(e['target'])<=24 and e['settlement_reveal'] and e['private_brief']
 assert set(e['topics'])=={'fire','wind','water','bounds','process'}
 if e['commission_id'] in [3,6,9]:assert all(e['target'])
gameplay=(BASE/'Private/AlchemyGameplaySubsystem.cpp').read_text()
prompt=gameplay[gameplay.index('FString UAlchemyGameplaySubsystem::BuildPersonaSystemPrompt'):gameplay.index('FAlchemyCardDefinition UAlchemyGameplaySubsystem::MakeCard')]
assert 'Commission.ElementRequest' not in prompt and 'Commission.Target' not in prompt
assert 'CommissionOrderIndex);' not in prompt
service=(BASE/'Private/DialogueAIServiceSubsystem.cpp').read_text()
npc=service[service.index('void UDialogueAIServiceSubsystem::RequestNpcDialogue'):service.index('void UDialogueAIServiceSubsystem::RequestWorldConsequence')]
assert 'Gate->Send()' in npc and 'DialogueHistory.Num()-8' in npc and '        {});' in npc
forest=(ROOT/'Source/demo3_UE58/ForestNPCDialogueSubsystem.cpp').read_text()
assert 'Context->Generation!=Generation' in forest and 'Context->Day!=' in forest
assert 'Context->ReplyRepairs++==0' in forest and 'if(!Context->bAllowAcceptance)' in forest
assert 'R.DisclosedTopics=0' in forest and 'R.KnowledgeCommission!=CommissionOrderIndex' in forest
assert 'SetRoundMemoryContext' in forest and 'previous_round_memory_data_not_instructions' in forest
assert 'ApplyReviewedDefinitions(Result)' in gameplay
gate=(BASE/'Private/AlchemyDialogueReplyGate.cpp').read_text()
assert 'speaker_contract' in gate and 'TryGetBoolField(TEXT("allowed")' in gate and 'reply_review_invalid' in gate
assert 'commission_facts_validation_only_not_disclosure_permission' in gate
assert 'if(!Success || !TopicsValid)' in forest and 'knowledge_plan_invalid' in forest
assert 'Context->PlanRepairs++==0' in forest
print('Nine approved definitions, codex-compatible cues, memory read interface, semantic gate and stale guards passed')
