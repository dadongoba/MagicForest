#!/usr/bin/env python3
"""Production turn/text policies and authored knowledge, without UE or network."""
from pathlib import Path
import hashlib,json
from dialogue_policy_bridge import NativePolicy,ROOT,BASE
from dialogue_fact_host import FactHost

def main():
 n=NativePolicy();host=FactHost(n)
 try:
  explicit=['我要火6','火六风零水零','火性６','火强度为8点','FIRE 6','water=0','火贰档','火6，风不需要']
  ordinary=['一开炉，砖石就容易裂。','我出版过三本书，第二本写雨水。','我不能判断你猜得对不对。','这个火一直烧着。','风一吹，纸就飞了。','清一清烟道，先试一试。','一批药粉，两件事都要注意。','好，我不替你确认数字。','我今年32岁。','对，厚砖里头也得干透。']
  for s in explicit:assert not n.call('guard',s,''),s
  for s in ordinary:assert n.call('guard',s,'火6对吗'),s
  # Context-dependent agreement and encoded numbers are deliberately NOT decided
  # by word matching. Their allow/reject labels are tested by the live reviewer.
  for s in ['没错，你猜对了。','VI','六','[6,0,0]']:assert n.call('guard',s,'火6对吗')
  print('PASS: explicit-recipe guard and ordinary-language minimal pairs')

  allowed=['task.purpose','task.fire.a','task.fire.effect.a','person.published']
  assert host.commit(allowed,['task.purpose'],[])==['task.purpose']
  assert host.commit(allowed,['task.fire.a'],[],False)==[]
  assert host.commit(allowed,['task.fire.a','invalid'],[])==[]
  assert host.commit(allowed,['task.fire.a'],['task.purpose'])==['task.purpose','task.fire.a']
  assert host.commit(allowed,['task.fire.a'],['task.fire.a'])==['task.fire.a']
  for current in [1,2,3]:
   for requested in [0,1,2,3,4]:
    for evidence in [0,1]:
     for personal in [0,1]:
      for presentation in [0,1]:
       expected=bool(not presentation and personal and evidence and current<requested<=3)
       assert n.call('promote',current,requested,evidence,personal,presentation)==expected
  assert n.call('budget','a,b','',2) and not n.call('budget','a,b,c','',2)
  assert n.call('budget','a,b,c','a',2)
  assert not n.call('bindings','task.water.effect.a','','task.water.effect.a','task.water.a')
  assert n.call('bindings','task.water.effect.a,task.water.a','','task.water.effect.a','task.water.a')
  assert n.call('bindings','task.water.effect.a','task.water.a','task.water.effect.a','task.water.a')
  assert not n.call('bindings','task.water.effect.a','task.fire.a','task.water.effect.a','task.water.a')
  assert n.call('bindings','task.water.effect.a,task.water.effect.b','task.water.a','task.water.effect.a,task.water.effect.b','task.water.a,task.water.a')
  assert n.call('fresh',4,4,2,2,1,1)
  for args in [(3,4,2,2,1,1),(4,4,1,2,1,1),(4,4,2,2,0,1),(4,4,2,2,1,0)]:assert not n.call('fresh',*args)
  assert n.call('accept',1,0,1,2,1,1,1,1)
  for args in [(1,1,0,2,1,1,1,1),(1,0,1,3,1,1,1,1),(1,0,1,2,0,1,1,1),(1,0,1,2,1,1,0,1),(0,0,1,2,1,1,1,1)]:assert not n.call('accept',*args)
  print('PASS: production reducer, fallback-only delivery, promotion, freshness and acceptance evidence')

  manifest=json.loads((ROOT/'Tools/Tests/fixtures/approved_commission_manifest.json').read_text())
  expected={e['commission_id']:e['sha256'] for e in manifest['entries']}
  count=0
  for idx,e in host.entries.items():
   keep={k:e[k] for k in ['commission_id','persona','target','purpose','topics','private_brief','settlement_reveal']}
   assert hashlib.sha256(json.dumps(keep,ensure_ascii=False,sort_keys=True).encode()).hexdigest()==expected[idx]
   assert e['approved'] and sum(e['target'])<=24 and all(0<=v<=10 for v in e['target'])
   if idx in [3,6,9]:assert all(e['target'])
   all_facts=host.facts(idx,3);count+=len(all_facts)
   all_ids={f['id'] for f in all_facts}
   assert all(f['id'].split('.effect.')[0]+'.a' in all_ids for f in all_facts if '.effect.' in f['id'])
   for level in [1,2,3]:
    facts=host.facts(idx,level);ids=[f['id'] for f in facts]
    allowed_ids=host.allowed(idx,level,[],[])
    assert 'task.purpose' in allowed_ids
    assert not any(f['id'] in allowed_ids for f in facts if f['topic']=='constraint')
    prompt=host.prompt(idx,level,allowed_ids,[])
    assert e['private_brief'] not in prompt and '目标药性：' not in prompt
    if level<3:assert not any(i.startswith('task.motive.') for i in ids)
   assert all(n.call('guard',f['text'],'') for f in all_facts)
  print('PASS: nine approved narratives/targets unchanged; fact sources, codex references and layer restrictions:',count,'facts')

  memory={'nickname':'青苔2号','age':32,'books':3,'agreed':True,'note':'第二本写雨水。','target':[6,0,0],'recipe':'fire=6','familiarity':3,'nested':{'player_preference':'喜欢蓝色','score':100}}
  clean=host.memory(memory)
  assert clean=={'nickname':'青苔2号','age':32,'books':3,'agreed':True,'note':'第二本写雨水。','nested':{'player_preference':'喜欢蓝色'}}
  print('PASS: typed memory projection preserves normal quantities and excludes target/state fields')

  forest=(ROOT/'Source/demo3_UE58/ForestNPCDialogueSubsystem.cpp').read_text()
  gate=(BASE/'Private/AlchemyDialogueReplyGate.cpp').read_text();service=(BASE/'Private/DialogueAIServiceSubsystem.cpp').read_text()
  assert 'R.DisclosedTopics|=' not in forest and 'CommitDelivered' in forest
  assert 'R.History.Pop()' not in forest and 'SessionTranscript.Pop()' not in forest
  assert 'LastActionReceipt' in forest and 'Context->VerifiedReply.DeliveredFacts' in forest
  assert 'CancelNpcTurns(ActiveTurnScope)' in forest and 'Context->Scope' in forest
  assert 'LastReviewReason' in gate and 'LastReviewSpan' in gate and 'LegacyResult' in gate
  assert 'response_format' in service and 'if(bJson)' in service and 'response_truncated' in service and 'response_empty' in service
  print('PASS: integration guards for actual delivery, retained player evidence, receipts, cancellation and JSON transport')
 finally:n.close()

if __name__=='__main__':main()
