#!/usr/bin/env python3
"""Compile production commission accessors and inject validator outcomes.

Checks that failed validation hides every legacy row, lookup never fabricates
success, and recovery exposes the approved definitions. Unreal JSON/file I/O is
not hosted here: the validator's boolean and atomic-update contract are injected.
No UE editor, network request, config mutation or player save is involved.
"""
import json
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'Plugins/DialogueMemoryTable/Source/DialogueMemoryTable/Private/AlchemyGameplaySubsystem.cpp'
text = SOURCE.read_text()
approved = json.loads((ROOT / 'Config/ForestCommissionKnowledge.json').read_text())['entries']


def method(name):
    match = re.search(r'(?:TArray<FAlchemyCommissionDefinition>|bool) UAlchemyGameplaySubsystem::' + name + r'\(', text)
    assert match, name
    start = text.index('{', match.start())
    depth, end = 1, start + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end]


shim = r'''
#include <string>
#include <vector>
#include <iostream>
#include <cassert>
using int32=int;
using FString=std::string;
using FName=std::string;
#define TEXT(x) x
static int ErrorLogs=0;
#define UE_LOG(Category,Verbosity,Format,...) (++ErrorLogs)
template<class T>struct TArray:std::vector<T> {
 using std::vector<T>::vector;
 void Reserve(int n){this->reserve(n);}
 void Add(const T& x){this->push_back(x);}
 void Reset(){this->clear();}
 int Num()const{return static_cast<int>(this->size());}
 template<class P>const T* FindByPredicate(P p)const{for(const auto& x:*this)if(p(x))return &x;return nullptr;}
};
enum class EAlchemyElement{Fire,Wind,Water};
enum class EAlchemyCardType{Pip,Transform,Bracket,Prefix,Court,Seal};
struct FAlchemyVector{int Fire=0,Wind=0,Water=0;};
struct FAlchemyCardDefinition {
 FName Id;FString Name;EAlchemyCardType Type=EAlchemyCardType::Transform;
 EAlchemyElement Element=EAlchemyElement::Fire;int PipValue=0;
};
struct FAlchemyCommissionDefinition {
 int OrderIndex=0,CustomerOrderIndex=0;FString Customer;FName CharacterId;
 FString Wish,ElementRequest,SensoryPrompt,Teaching,HandCardsSpec,OpeningLine;
 FAlchemyVector Target;int SlotCount=0;bool bMustUseAll=false;int CostLimit=0;
 TArray<FAlchemyCardDefinition> MustCards;
};
class UAlchemyGameplaySubsystem {
public:
 static TArray<FAlchemyCardDefinition> MakeMajorCards(){return {};}
 static TArray<FAlchemyCommissionDefinition> MakeCommissions();
 TArray<FAlchemyCommissionDefinition> GetCommissions()const;
 bool GetCommission(int,FAlchemyCommissionDefinition&)const;
};
namespace AlchemyGameplay {FString DescribeElementTarget(const FAlchemyVector&){return "structural default";}}
namespace AlchemyDialogueKnowledge {
 // Failed, partial failed, and validated outcomes emulate the loader boundary.
 int Outcome=0;
 bool ApplyReviewedDefinitions(TArray<FAlchemyCommissionDefinition>& rows) {
  if(Outcome==0)return false;
  if(Outcome==1){rows[0].Wish="partial unapproved mutation";return false;}
'''
for entry in approved:
    shim += '  rows[%d].Target={%s};rows[%d].Wish=%s;\n' % (
        entry['commission_id'] - 1, ','.join(map(str, entry['target'])),
        entry['commission_id'] - 1, json.dumps(entry['purpose'], ensure_ascii=False))
shim += r'''
  return true;
 }
}
'''
cpp = shim + '\n'.join(method(name) for name in ['MakeCommissions', 'GetCommissions', 'GetCommission'])
cpp += r'''
int main() {
 UAlchemyGameplaySubsystem runtime;
 FAlchemyCommissionDefinition output;output.OrderIndex=777;output.Wish="sentinel";
 // Missing/rejected config must expose no authoring defaults or partial output.
 for(int outcome:{0,1}) {
  AlchemyDialogueKnowledge::Outcome=outcome;
  assert(runtime.GetCommissions().Num()==0);
  for(int id=1;id<=9;++id)assert(!runtime.GetCommission(id,output));
  assert(output.OrderIndex==777 && output.Wish=="sentinel");
 }
 assert(ErrorLogs==1); // Repeated UI reads do not flood the error log.
 AlchemyDialogueKnowledge::Outcome=2;
 const auto recovered=runtime.GetCommissions();assert(recovered.Num()==9);
'''
for entry in approved:
    idx = entry['commission_id']
    fire, wind, water = entry['target']
    cpp += (' assert(runtime.GetCommission(%d,output));'
            'assert(output.OrderIndex==%d && output.Target.Fire==%d && output.Target.Wind==%d && output.Target.Water==%d);'
            'assert(output.Wish==%s);\n') % (idx, idx, fire, wind, water, json.dumps(entry['purpose'], ensure_ascii=False))
cpp += r'''
 for(int id:{-1,0,10,2147483647})assert(!runtime.GetCommission(id,output));
 AlchemyDialogueKnowledge::Outcome=0;
 assert(runtime.GetCommissions().Num()==0 && !runtime.GetCommission(1,output));
 assert(ErrorLogs==2); // A later failure after recovery is diagnosed again.
 std::cout<<"PASS: production Make/Get fail closed, partial failure, lookup rejection, log/recovery, nine approved targets\n";
}
'''
with tempfile.TemporaryDirectory(prefix='forest-commission-config-') as temp:
    source = Path(temp) / 'test.cpp'
    binary = Path(temp) / 'test'
    source.write_text(cpp)
    subprocess.run(['c++', '-std=c++17', '-O0', str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
