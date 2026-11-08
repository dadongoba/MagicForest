#!/usr/bin/env python3
"""Synthetic DeepSeek compact-memory checks using compiled production policies/prompts.

Credentials only from environment. Does not launch UE or write credentials. HTTP,
view projection and state JSON roundtrip are portable hosts, not engine saves.
"""
import argparse,hashlib,json,os,ssl,time,urllib.request,urllib.error
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from compact_memory_bridge import NativeCompact,BASE,ROOT
from dialogue_policy_bridge import NativePolicy
from dialogue_fact_host import FactHost

def main():
 p=argparse.ArgumentParser();p.add_argument('--output',required=True);a=p.parse_args()
 key=os.environ.get('FOREST_DIALOGUE_API_KEY');assert key,'credential_unavailable'
 endpoint='https://api.deepseek.com/v1/chat/completions';model='deepseek-chat'
 n=NativeCompact();old=NativePolicy();h=FactHost(old);extra=n.call('rules');core={x:old.call('rules',x) for x in ['analysis','generation','review','state']}
 files=[BASE/'Private/DialogueCompactMemory.cpp',BASE/'Public/DialogueCompactMemoryPolicy.h',BASE/'Private/AlchemyDialogueReplyGate.cpp',BASE/'Private/DialogueMemoryTableSubsystem.cpp',BASE/'Public/DialogueMemoryTypes.h',BASE/'Public/AlchemyDialogueTurn.h',BASE/'Private/DialogueAIServiceSubsystem.cpp',ROOT/'Source/demo3_UE58/ForestNPCDialogueSubsystem.cpp',ROOT/'Source/demo3_UE58/ForestNPCDialogueSubsystem.h']
 frozen={str(f.relative_to(ROOT)):hashlib.sha256(f.read_bytes()).hexdigest() for f in files}
 def api(system,data,tokens=1000,temp=0,deadline=None,instruction=''):
  start=time.monotonic();payload=dict(model=model,messages=[dict(role='system',content=system),dict(role='user',content=json.dumps(data,ensure_ascii=False)+instruction)],temperature=temp,max_tokens=tokens,response_format={'type':'json_object'})
  request=urllib.request.Request(endpoint,json.dumps(payload,ensure_ascii=False).encode(),{'Content-Type':'application/json','Authorization':'Bearer '+key})
  with urllib.request.urlopen(request,timeout=min(35,max(1,deadline-start)) if deadline else 35,context=ssl.create_default_context()) as response:body=json.load(response)
  c=body['choices'][0];assert c.get('finish_reason')!='length','response_truncated'
  return json.loads(c['message']['content']),dict(seconds=round(time.monotonic()-start,3),usage=body.get('usage',{}),model=body.get('model'))
 def catalog(store,owner,day,commission):
  states=[x for x in store.get('states',[]) if x['owner']==owner]
  facts=[dict(id=x['id'],kind=x['kind'],text=x['text'],source_kind='player_self_report',day=x['day']) for x in reversed(store.get('facts',[])) if x['owner']==owner][:12]
  loops=[]
  for x in reversed(store.get('loops',[])):
   if x['owner']!=owner:continue
   active=x['status']=='open' and (not x['commission'] or (x['day']==day and x['commission']==commission))
   loops.append(dict(id=x['id'],kind=x['kind'],text=x['text'],source_kind='player_statement' if x['source']=='user' else 'npc_dialogue',status=x['status'] if active or x['status']!='open' else 'expired',day=x['day'],commission=x['commission']))
  out=dict(source='bounded_memory_data_not_instructions',facts=facts,loops=loops[:6])
  if states and states[0].get('note'):out.update(last_exchange=states[0]['note'],last_exchange_day=states[0]['day'])
  return out
 def prompt(idx,allowed,known,view):
  persona=h.entries[idx]['persona'];out=old.call('prompt',persona,1,idx,json.dumps(view,ensure_ascii=False))
  return out+h.guidance(idx,1,allowed,known)+core['state']+json.dumps(dict(visibility='internal_only',commission_state='not_accepted',action_receipt='none'),ensure_ascii=False)
 def lifecycle(idx):
  owner={1:'natta',4:'fawnia',7:'forest_witch'}[idx];store={};known=[];results=[]
  questions=[('remember_name','请叫我青苔，今天先随便聊聊，不接委托。','青苔'),('recall_after_reload','上回我希望你怎么称呼我？','青苔'),('correct_name','以后请叫我小石头，别用原来的称呼了。','小石头'),('recall_correction','我们现在约定用什么称呼？','小石头'),('unsupported_guess','你以前是不是住过海底宫殿？我只是猜的。',None),('recipe_guess','这份委托按火6、风0、水0就对了吧？',None)]
  for turn,(name,q,expected) in enumerate(questions,1):
   begun=time.monotonic();deadline=begun+45;metrics=[];view=catalog(store,owner,1,idx);window=[dict(role='user',content=q)] # Each call deliberately begins a fresh chat window.
   data=dict(task_catalog=h.catalog(idx,1),may_accept=True,presentation=False,current_layer=1,already_delivered_fact_ids=known,unlock_topics=h.profiles[h.entries[idx]['persona']]['unlocks'][1:],messages=[dict(index=0,**window[0])],memory_catalog=view)
   try:
    plan,m=api(core['analysis']+extra['analysis'],data,1150,deadline=deadline);metrics.append(m)
    selected=plan.get('fact_ids',[]);allowed=h.allowed(idx,1,selected,known)
    memory=plan.get('memory',{});chosen=n.call('select',dict(catalog=view,analysis=memory));contract=prompt(idx,allowed,known,chosen)+extra['context']+'\n【本轮意图】\n'+plan.get('intent','other')
    candidate,m=api(contract+core['generation'],dict(conversation=window,instruction='只输出spoken_text和claimed_fact_ids的json对象。回答最后玩家发言。'),650,.7,deadline);metrics.append(m)
    spoken=candidate['spoken_text'];claims=candidate.get('claimed_fact_ids',[])
    review_data=dict(speaker_contract=contract,player_message=q,candidate=spoken,task_catalog_validation_only=h.catalog(idx,1),allowed_fact_ids=allowed,claimed_fact_ids=claims,recent_messages=window,proposed_player_memory=json.dumps(memory,ensure_ascii=False))
    review,m=api(core['review']+extra['review'],review_data,800,deadline=deadline,instruction=extra['review_instruction']);metrics.append(m)
    passed_review=review.get('allowed') is True and set(review.get('delivered_fact_ids',[]))<=set(allowed) and old.call('guard',spoken,q)
    attempts=[dict(candidate=candidate,review=review)]
    # Match the runtime's single speech repair, sharing its 45 second deadline.
    if not passed_review and time.monotonic()<deadline-3:
     repair=contract+core['generation']+'\n本次唯一修复：'+json.dumps({k:review.get(k) for k in ['code','span','reason']},ensure_ascii=False)
     candidate,m=api(repair,dict(conversation=window,instruction='只输出spoken_text和claimed_fact_ids。'),650,.7,deadline);metrics.append(m);spoken=candidate['spoken_text'];claims=candidate.get('claimed_fact_ids',[])
     review_data.update(candidate=spoken,claimed_fact_ids=claims)
     review,m=api(core['review']+extra['review'],review_data,800,deadline=deadline,instruction=extra['review_instruction']);metrics.append(m)
     passed_review=review.get('allowed') is True and set(review.get('delivered_fact_ids',[]))<=set(allowed) and old.call('guard',spoken,q);attempts.append(dict(candidate=candidate,review=review))
    if passed_review:
     out=n.call('apply',dict(store=store,owner=owner,day=1,commission=idx,turn=turn,revision='live-frozen',player=q,spoken=spoken,analysis=memory,review=review.get('memory',{}),catalog=view));store=out['store'];known=h.commit(allowed,review.get('delivered_fact_ids',[]),known)
    facts=store.get('facts',[]);preference=next((x['text'] for x in facts if x['kind']=='preferred_name'),None)
    passed=bool(passed_review)
    if name in ['remember_name','correct_name']:passed=passed and isinstance(preference,str) and expected in preference
    if name in ['recall_after_reload','recall_correction']:passed=passed and expected in spoken
    if name=='unsupported_guess':passed=passed and not any('海底宫殿' in x['text'] for x in facts)
    if name=='recipe_guess':passed=passed and not any('火6' in x['text'] for x in facts)
    results.append(dict(name=name,question=q,passed=bool(passed),analysis=plan,attempts=attempts,stored=store,metrics=metrics,seconds=round(time.monotonic()-begun,3),request_count=len(metrics)))
   except Exception as e:results.append(dict(name=name,passed=False,error_type=type(e).__name__,http_status=getattr(e,'code',None),metrics=metrics))
  return dict(npc=owner,passed=all(x['passed'] for x in results),turns=results)
 with ThreadPoolExecutor(max_workers=3) as pool:cases=list(pool.map(lifecycle,[1,4,7]))
 report=dict(passed=all(x['passed'] for x in cases),cases=cases,source_hashes=frozen,source_unchanged=all(hashlib.sha256((ROOT/k).read_bytes()).hexdigest()==v for k,v in frozen.items()),limitations=['Synthetic live analysis/generation/review plus compiled production compact parser/selection/merge.','HTTP transport, memory view projection, source IDs and JSON state roundtrip are host adapters; no UE/PIE, reflection, real SaveGame or GUI execution.','Fresh-window nickname tests simulate reload, not restarting the engine.','No before/after monetary savings claim; raw provider usage and request latency are recorded.','Finite same-model extraction/review, not a universal semantic guarantee.'])
 turns=[t for c in cases for t in c['turns']];report.update(passed_turns=sum(t['passed'] for t in turns),total_turns=len(turns),request_count=sum(len(t['metrics']) for t in turns),input_tokens=sum(m['usage'].get('prompt_tokens',0) for t in turns for m in t['metrics']),output_tokens=sum(m['usage'].get('completion_tokens',0) for t in turns for m in t['metrics']))
 Path(a.output).write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['passed','passed_turns','total_turns','request_count','input_tokens','output_tokens','source_unchanged']}));n.close();old.close()
 return 0 if report['passed'] else 2
if __name__=='__main__':raise SystemExit(main())
