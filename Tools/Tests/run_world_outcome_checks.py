"""Run production score policy and calendar code; validate configured state coverage.
Portable checks do not replace UE SaveGame or map/Slate runtime acceptance.
"""
from pathlib import Path
import json, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[2]
def main():
 rules=json.loads((ROOT/'Config/ForestWorldRules.json').read_text())
 assert rules['Revision']==1 and len(rules['Commissions'])==9
 assert {r['Id'] for r in rules['Commissions']}==set(range(1,10))
 transitions=rules['Transitions'];assert len(transitions)==4
 for row in transitions:
  assert len(row)==5 and all(isinstance(s,int) and 0<=s<=3 for s in row)
 for prev,row in enumerate(transitions):
  assert row[0]<=prev and row[4]==3
  assert row==sorted(row),row
 for s in rules['States']:
  assert .4<=s['AvailabilityA']<=1 and .85<=s['AvailabilityB']<=1
 zones=json.loads((ROOT/'Config/ForestZones.json').read_text()); ids={p['Id'] for p in zones['VisualPresets']}
 assert all(f'{p}_s{s}' in ids for p in ['weakened','overgrown','disturbed','inactive'] for s in [1,2,3])
 assert zones['Controls']==[] # no unconditional penalties in shipped layout
 for c in rules['Commissions']:
  assert c['Persona']==('blacksmith' if c['Id']<=3 else 'operator' if c['Id']<=6 else 'novelist')
  assert all(c[k] for k in ['Success','Partial','Terminal','Continuity','Refusal'])
  assert not any(ch.isdigit() for k in ['Success','Partial','Terminal','Continuity','Refusal'] for ch in c[k])
 bands=rules['Bands']; assignments=''.join(f'b.{k}={v};' for k,v in bands.items())
 cpp=r'''
#include "ForestOutcomePolicy.h"
#include "AlchemyCampaignPolicy.h"
#include <cassert>
#include <iostream>
int main(){
 ForestOutcomePolicy::Bands b; ASSIGN
 auto g=[&](double d,double p,double c=1,bool fail=false,bool missing=false){return ForestOutcomePolicy::Grade(b,d,p,c,fail,missing);};
 assert(g(1,0)==0);assert(g(.8,.15,.98)==1);assert(g(1.2,.15,.98)==1);
 assert(g(.65,.45,.9)==2);assert(g(1.4,.45,.9)==2);
 assert(g(.649,.1,.9)==3);assert(g(1.401,.1,.9)==3);assert(g(1,.451,.9)==3);
 assert(g(1,0,1,true)==4);assert(g(1,0,1,false,true)==3);
 for(int d=0;d<=300;++d)for(int p=0;p<=200;++p){int grade=g(d/100.,p/100.,.98);assert(grade>=0&&grade<=3);}
 for(int day=1;day<=3;++day)for(int mask=0;mask<8;++mask){
  int required=0;for(int role=0;role<3;++role)if(!ForestOutcomePolicy::LineUnavailable(mask&(1<<role),day-1,day))++required;
  assert(required==(day==1?3:3-__builtin_popcount((unsigned)mask)));
  assert(!ForestOutcomePolicy::LineUnavailable(true,day,day));
 }
 auto early=AlchemyCampaign::Normalize({1,1,true});assert(early.Completed && early.Day==1);
 assert(!AlchemyCampaign::Confirm(early,2));
 auto last=AlchemyCampaign::Normalize({3,3,true});assert(last.Day==3 && last.Completed);
 std::cout<<"PASS production bands, special failures, disconnect day boundaries and early-ending reload\n";
}'''.replace('ASSIGN',assignments)
 with tempfile.TemporaryDirectory() as d:
  src=Path(d)/'check.cpp';out=Path(d)/'check';src.write_text(cpp)
  subprocess.run(['clang++','-std=c++17','-fsanitize=address,undefined','-I'+str(ROOT/'Source/demo3_UE58'),'-I'+str(ROOT/'Plugins/DialogueMemoryTable/Source/DialogueMemoryTable/Public'),str(src),'-o',str(out)],check=True)
  subprocess.run([str(out)],check=True)
 print('PASS all 20 state transitions, 9 task branches, resource floors and 12 severity presets')
if __name__=='__main__':main()
