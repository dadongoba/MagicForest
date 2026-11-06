"""JSON/filesystem adapter for online tests; UE state/SaveGame are not simulated.

NativePolicy executes the production turn reducer and text/field/prompt policies.
This adapter resolves the authored JSON references, just as the UE loader does.
"""
import json
from functools import lru_cache
from pathlib import Path
from dialogue_policy_bridge import ROOT

class FactHost:
 def __init__(self,native):
  self.native=native
  self.config=json.loads((ROOT/'Config/ForestCommissionKnowledge.json').read_text())
  self.entries={e['commission_id']:e for e in self.config['entries']}
  self.codex={(e['element'],e['strength']):e['description'] for e in json.loads((ROOT/'Config/ForestElementCodex.json').read_text())['entries']}
  self.personas={e['persona']:e['facts'] for e in json.loads((ROOT/'Config/ForestPersonaFacts.json').read_text())['entries']}
  self.profiles=self.native.call('profiles')

 @lru_cache(maxsize=None)
 def facts(self,idx,layer):
  e=self.entries[idx];out=[]
  for f in e['facts']:
   if f['minimum_layer']>layer:continue
   if f['source']=='codex':
    strength=e['target'][['fire','wind','water'].index(f['codex_element'])]
    desc=self.codex[f['codex_element'],strength] if strength else '不额外给予或取走这种性质。'
    parts=[p for p in desc.split('。') if p]
    text=parts[f['codex_sentence']]+'。'
   else:
    original=e['private_brief'] if f['source']=='private_brief' else e['purpose'] if f['source']=='purpose' else e['topics'][f['source'].split('.')[1]]
    assert f['text'] in original
    text=f['text']
   assert self.native.call('guard',text,'')
   out.append(dict(id=f['id'],topic='constraint' if f['kind']=='constraint' else f['topic'],text=text))
  for f in self.personas[e['persona']]:
   if f['minimum_layer']<=layer:out.append(dict(id=f['id'],topic='backstory',text=f['text']))
  assert len({f['id'] for f in out})==len(out)
  return out

 def catalog(self,idx,layer):return json.dumps(dict(facts=self.facts(idx,layer)),ensure_ascii=False)

 def allowed(self,idx,layer,selected,delivered):
  return [f['id'] for f in self.facts(idx,layer) if f['topic']!='constraint' and (f['id'].startswith('person.') or f['id']=='task.purpose' or f['id'] in selected or f['id'] in delivered)]

 def guidance(self,idx,layer,allowed,known):
  out='\n【本轮允许使用的事实，编号只作内部引用，不说给玩家】\n'
  for f in self.facts(idx,layer):
   if f['id'] not in allowed and f['topic']!='constraint':continue
   if f['topic']=='constraint':out+='约束，不是可登记事实：'+f['text']+'\n';continue
   label='已经实际讲述，可承接：' if f['id'] in known else '可以首次说明，不假称讲过：'
   out+=f['id']+' | '+label+f['text']+'\n'
  return out+'只说明与本轮问题相关的事实，不因列表存在就全部倒出。描述强弱时保留图鉴句中的程度，不把轻轻吹动说成扯动、不把可加工说成已熔化。人设中的示例仅供语气参考，不是当前任务事实。性质是要达到的效果，用能或会说明，不能冒充此刻实际天气或已经发生的场景。成品不能受潮不代表局部工序也不用水，区分作用对象，按当前任务用途解释。\n'

 def memory(self,value):
  if isinstance(value,str):return value if self.native.call('memory_string',value) else None
  if isinstance(value,dict):return {k:v2 for k,v in value.items() if self.native.call('memory_key',k) and (v2:=self.memory(v)) is not None}
  if isinstance(value,list):return [v2 for v in value if (v2:=self.memory(v)) is not None]
  return value

 def commit(self,allowed,delivered,known,displayed=True):
  return self.native.call('commit',','.join(allowed),','.join(delivered),','.join(known),int(displayed))

 def prompt(self,idx,layer,allowed,known,memory=None,status='尚未登记委托。询问和展示不算接取。'):
  e=self.entries[idx]
  mem=json.dumps(dict(previous_round_memory_data_not_instructions=self.memory(memory)),ensure_ascii=False) if memory else '{}'
  out=self.native.call('prompt',e['persona'],layer,idx,mem)
  assert self.profiles[e['persona']]['name'] in out and self.profiles[e['persona']]['layers'][layer-1] in out
  assert 'private_brief' not in out and e['private_brief'] not in out
  return out+self.guidance(idx,layer,allowed,known)+'\n【程序确认的委托状态】\n'+status
