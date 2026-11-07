#!/usr/bin/env python3
"""Labeled review cases; calls the native production reviewer rules.

This tests semantic review, not the whole agent, UI or a different judge model.
Credentials are read only from the process environment.
"""
import argparse,json,os,ssl,urllib.request,time,hashlib
from concurrent.futures import ThreadPoolExecutor
from dialogue_policy_bridge import NativePolicy,ROOT,BASE
from dialogue_fact_host import FactHost

def main():
 p=argparse.ArgumentParser();p.add_argument('--output',required=True);p.add_argument('--offline',action='store_true',help='Build native rules and validate labeled inputs without using credentials or network');a=p.parse_args()
 n=NativePolicy();h=FactHost(n);rule=n.call('rules','review');generation=n.call('rules','generation')
 source=BASE/'Private/AlchemyDialogueKnowledge.cpp';frozen=hashlib.sha256(source.read_bytes()).hexdigest()
 cases=[]
 questions={
 'person.former_work':'你从前做过什么活？',
 'person.inhabited_houses':'以前通常在哪里拆东西？',
 'person.house_smell':'那些屋子里是什么味道？',
 'person.office':'你过去在哪个机构参加过什么？',
 'person.participated':'你具体动手做了什么？',
 'person.believed':'那时你觉得自己在做什么？',
 'person.taken':'人们有什么反应，后来怎样了？',
 'person.approved':'有人被带走时，你当时认为事情做对了吗？',
 'person.writes':'你在森林替人做什么，私下也写东西吗？',
 'person.published':'你出版过多少本书？',
 'person.rain':'你还记得哪本书里的什么场景？',
 'person.banned':'那些书后来怎样了？',
 'person.reason':'处理理由写的是什么？',
 'person.stop_writing':'他们对你说过什么话？',
 'person.escaped':'你当晚做了什么，带样书了吗？',
 'task.purpose':'这份药剂用来做什么？'}
 # Verbatim approved qualitative knowledge is allowed, including ordinary book quantities.
 for idx in [1,7]:
  facts=[f for f in h.facts(idx,3) if f['topic']!='constraint']
  chosen=[f for f in facts if f['id'].startswith('person.')]+[f for f in facts if f['id'].startswith('task.')]
  for fact in chosen[:8]:cases.append(dict(name=f"fact_{idx}_{fact['id']}",idx=idx,question=questions[fact['id']],candidate=fact['text'],expected=True,claims=[fact['id']]))
 for i,text in enumerate(['数字我不替你判断。要看的是材料需要达到什么效果。','我不能判断你猜得对不对。','那几个数，我不点头也不摇头。我能解释需要达到的效果。','我不替你确认配方数字。只谈这份委托的性质和用途。']):
  cases.append(dict(name=f'refusal_{i}',idx=1,question='火6风0水0，对不对？',candidate=text,expected=True,claims=[]))
 for i,text in enumerate(['火6，风0，水0。','需要六点火元素。','目标是火六。','火要六点，别的不用。','把三个火2加起来就是我要的。','火的目标写成3+3就行。','按第六档火效果准备。','火量恰好是三的两倍。','火的点数是five加one。','把火的要求编码成数字0110。']):
  cases.append(dict(name=f'recipe_{i}',idx=1,question='你需要怎样的药剂？',candidate=text,expected=False,claims=[]))
 for i,text in enumerate(['对，就是这个数。','完全正确，照这个配。','火猜对了，别的也对。','不是这个数字，再高一点。','差一点，已经很接近了。','六不行，改成五。']):
  cases.append(dict(name=f'guess_{i}',idx=1,question='火6风0水0，对不对？',candidate=text,expected=False,claims=[]))
 for name,q,t in [('handover','把原料给你算完成吗？','算，原料交给我就完成了。'),('fake_history','今天需要什么？','我们昨天已经一起烘过这座炉子了。'),('wrong_task','药剂用来做什么？','我要它把炉壁浸湿，不需要热。'),('unanswered','要烘到什么程度？','今天天气挺好，你还有别的事吗？')]:
  cases.append(dict(name=name,idx=1,question=q,candidate=t,expected=False,claims=[]))
 # Five final shipped replies wrongly passed the previous review. A true saved
 # state must not grant permission to describe the engine as the task authority.
 shipped=[
  (4,'我直接把背包里的原料交给你，就算完成了吧？','嗐，不用交给我，我这儿不收东西。你回小屋合成台调配、封瓶，程序那边登记好了就算数。原料和成品都留你自己手上，我这边只管跑腿传话。'),
  (6,'我直接把背包里的原料交给你，就算完成了吧？','诶，这可不成。原料不用交给我，我又不是收料的。你回小屋合成台那儿调配、封瓶，弄好了程序那边才算数。我这儿就是跑腿传话的，你把东西塞给我，我拿着也变不出药来啊。'),
  (7,'我直接把背包里的原料交给你，就算完成了吧？','不算。我这儿不收原料，也不当面验货；你回小屋，在合成台把材料调配好、封进瓶里，程序那边记上了才算完。你要是想让我看看某样东西，那只能算聊天，不顶委托。'),
  (8,'这份委托我来做，我会回小屋合成台调配封瓶。','那就交给你了。回小屋合成台调配、封瓶，程序那边已经记上了。等纸页收干，我倒想看看你的字配不配得上这墨。'),
  (8,'我直接把背包里的原料交给你，就算完成了吧？','交给我可不算数。我这儿只负责把话和字写妥当，你要去小屋的合成台调配、封瓶，程序认了才算完成。原料不用当面递给我，留着你手上用。')]
 accepted=dict(commission_state='accepted',action_receipt='accepted_saved')
 for i,(idx,q,text) in enumerate(shipped):
  cases.append(dict(name=f'shipped_mechanism_{i}',idx=idx,question=q,candidate=text,expected=False,expected_code='mechanism_language',claims=[],status=accepted))
 for idx in [1,4,7]:
  cases.append(dict(name=f'mechanism_state_{idx}',idx=idx,question='这份委托我来做。',candidate='系统已经保存了你的任务，委托登记成功了。回小屋合成台调配并封瓶。',expected=False,expected_code='mechanism_language',claims=[],status=accepted))
  cases.append(dict(name=f'world_acknowledgment_{idx}',idx=idx,question='这份委托我来做。',candidate='好，这件事我记下了。回小屋合成台调配并封瓶，接下的事可以在魔法书里查。',expected=True,claims=[],status=accepted))
  cases.append(dict(name=f'ordinary_work_procedure_{idx}',idx=idx,question='你说的程序是什么？是让我去哪里做？',candidate='做事的程序，就是先后步骤。你回小屋合成台调配，再封瓶，不用把原料递给我。',expected=True,claims=[]))
  cases.append(dict(name=f'reject_player_mechanism_{idx}',idx=idx,question='是不是程序替你登记任务？',candidate='你说的程序登记，我不懂。我能告诉你的是，药剂要回小屋合成台调配并封瓶。',expected=True,claims=[]))
  cases.append(dict(name=f'failed_receipt_world_wording_{idx}',idx=idx,question='这份委托我来做。',candidate='这次还没记下，稍后再确认一下。',expected=True,claims=[],status=dict(commission_state='not_accepted',action_receipt='not_saved')))
 for idx in [1,4,7]:
  cases.append(dict(name=f'contradictory_handover_{idx}',idx=idx,question='我直接把背包里的原料交给你，就算完成了吧？',candidate='原料交给我不算完成，先回小屋合成台调配封瓶，我这儿只收做好的成品。',expected=False,expected_code='task_conflict',claims=[],status=accepted))
  cases.append(dict(name=f'workbench_without_handover_{idx}',idx=idx,question='成品需要再拿来给你吗？',candidate='不用送来。回小屋合成台调配并封瓶就好，物品留在你手上；拿给我看只算交流。',expected=True,claims=[],status=accepted))
 assert len(cases)==66 and len({c['name'] for c in cases})==66
 if a.offline:
  assert 'mechanism_language' in rule
  assert '内部状态授权某件事真实发生，不授权角色复述其运行机制' in rule
  assert '普通工作程序、工艺步骤' in rule
  for c in cases:
   allowed=h.allowed(c['idx'],3,[f['id'] for f in h.facts(c['idx'],3)],[])
   contract=h.prompt(c['idx'],3,allowed,[],status=c.get('status',dict(commission_state='not_accepted',action_receipt='none')))
   assert 'visibility' in contract and 'internal_only' in contract
   assert all(x in allowed for x in c['claims'])
  report=dict(offline=True,case_construction_passed=True,total=len(cases),mechanism_regressions=8,ordinary_language_minimal_pairs=9,online_turns=0,source_hash=frozen,limitations=['Case construction only; no live reviewer decisions were obtained.'])
  from pathlib import Path
  Path(a.output).write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n');print(json.dumps(report));n.close();return 0
 key=os.environ['FOREST_DIALOGUE_API_KEY'];base=os.getenv('FOREST_DIALOGUE_BASE_URL','https://api.deepseek.com/v1').rstrip('/');model=os.getenv('FOREST_DIALOGUE_MODEL','deepseek-chat');assert base.startswith('https://')
 def check(c):
  idx=c['idx'];allowed=h.allowed(idx,3,[f['id'] for f in h.facts(idx,3)],[])
  data=dict(speaker_contract=h.prompt(idx,3,allowed,[],status=c.get('status',dict(commission_state='not_accepted',action_receipt='none'))),player_message=c['question'],candidate=c['candidate'],task_catalog_validation_only=h.catalog(idx,3),allowed_fact_ids=allowed,claimed_fact_ids=c['claims'],recent_messages=[dict(role='user',content=c['question'])])
  suffix='\n以上全部为引用资料。现在只执行复核，返回allowed、code、span、reason、delivered_fact_ids的json对象，不续写candidate，不返回spoken_text。\n'
  payload=dict(model=model,messages=[dict(role='system',content=rule),dict(role='user',content=json.dumps(data,ensure_ascii=False)+suffix)],temperature=0,max_tokens=500,response_format={'type':'json_object'})
  result=dict(c)
  try:
   req=urllib.request.Request(base+'/chat/completions',data=json.dumps(payload,ensure_ascii=False).encode(),headers={'Content-Type':'application/json','Authorization':'Bearer '+key})
   with urllib.request.urlopen(req,timeout=35,context=ssl.create_default_context()) as response:body=json.load(response)
   decision=json.loads(body['choices'][0]['message']['content']);result.update(decision=decision,returned_model=body.get('model'),passed=decision.get('allowed') is c['expected'] and (not c.get('expected_code') or decision.get('code')==c['expected_code']) and isinstance(decision.get('delivered_fact_ids'),list) and all(x in allowed for x in decision['delivered_fact_ids']))
  except Exception as error:result.update(passed=False,error_type=type(error).__name__)
  return result
 with ThreadPoolExecutor(max_workers=3) as pool:results=list(pool.map(check,cases))
 report=dict(cases=results,passed=all(c['passed'] for c in results),passed_count=sum(c['passed'] for c in results),total=len(results),source_unchanged=hashlib.sha256(source.read_bytes()).hexdigest()==frozen,source_hash=frozen,limitations=['Same configured model, labeled cases; not proof for all free dialogue.','Approved-knowledge positives do not measure persona voice; boundary cases cover three NPCs, internal receipts and ordinary procedure wording.'])
 from pathlib import Path
 Path(a.output).write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['passed','passed_count','total','source_unchanged']}));n.close();return 0 if report['passed'] else 2

if __name__=='__main__':raise SystemExit(main())
