#!/usr/bin/env python3
"""Small paired request-shape cost test; baseline is the previous closing-summary flow.

No credential persistence. Not a real UE benchmark or a general savings claim.
"""
import os,json,time,ssl,urllib.request,hashlib,argparse
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from compact_memory_bridge import NativeCompact,ROOT,BASE
from dialogue_policy_bridge import NativePolicy
from dialogue_fact_host import FactHost

def main():
 p=argparse.ArgumentParser();p.add_argument('--output',required=True);a=p.parse_args();key=os.environ['FOREST_DIALOGUE_API_KEY']
 n=NativeCompact();native=NativePolicy();h=FactHost(native);extra=n.call('rules');rules={x:native.call('rules',x) for x in ['analysis','generation','review','state']}
 def api(system,data,tokens,temp=0,tail=''):
  start=time.monotonic();data=data if isinstance(data,str) else json.dumps(data,ensure_ascii=False)
  req=urllib.request.Request('https://api.deepseek.com/v1/chat/completions',json.dumps(dict(model='deepseek-chat',messages=[dict(role='system',content=system),dict(role='user',content=data+tail)],temperature=temp,max_tokens=tokens,response_format={'type':'json_object'}),ensure_ascii=False).encode(),{'Content-Type':'application/json','Authorization':'Bearer '+key})
  with urllib.request.urlopen(req,timeout=35,context=ssl.create_default_context()) as resp:b=json.load(resp)
  assert b['choices'][0].get('finish_reason')!='length'
  return json.loads(b['choices'][0]['message']['content']),dict(seconds=round(time.monotonic()-start,3),usage=b.get('usage',{}),model=b.get('model'))
 def base_prompt(idx,allowed,view):return native.call('prompt',h.entries[idx]['persona'],1,idx,json.dumps(view,ensure_ascii=False))+h.guidance(idx,1,allowed,[])+rules['state']+json.dumps(dict(visibility='internal_only',commission_state='not_accepted',action_receipt='none'))
 def pair(idx):
  owner={1:'natta',4:'fawnia',7:'forest_witch'}[idx];q='请叫我青苔，今天只闲聊，不接委托。';window=[dict(role='user',content=q)];out=dict(npc=owner)
  for new in [False,True]:
   metric=[];start=time.monotonic();legacy=dict(relationship='',last_exchange='',facts=[],open_loops=[],world=[]);catalog=dict(source='bounded_memory_data_not_instructions',facts=[],loops=[])
   data=dict(task_catalog=h.catalog(idx,1),may_accept=True,presentation=False,current_layer=1,already_delivered_fact_ids=[],unlock_topics=h.profiles[h.entries[idx]['persona']]['unlocks'][1:],messages=[dict(index=0,**window[0])])
   if new:data['memory_catalog']=catalog
   plan,m=api(rules['analysis']+(extra['analysis'] if new else ''),data,1150 if new else 850);metric.append(m)
   allowed=h.allowed(idx,1,plan.get('fact_ids',[]),[]);memory=plan.get('memory',{});view=n.call('select',dict(catalog=catalog,analysis=memory)) if new else legacy
   contract=base_prompt(idx,allowed,view)+(extra['context'] if new else '')+'\n【本轮意图】\n'+plan.get('intent','smalltalk')
   answer,m=api(contract+rules['generation'],dict(conversation=window,instruction='只输出json对象：spoken_text和claimed_fact_ids。'),650,.7);metric.append(m)
   rd=dict(speaker_contract=contract,player_message=q,candidate=answer['spoken_text'],task_catalog_validation_only=h.catalog(idx,1),allowed_fact_ids=allowed,claimed_fact_ids=answer.get('claimed_fact_ids',[]),recent_messages=window)
   if new:rd['proposed_player_memory']=json.dumps(memory,ensure_ascii=False)
   review,m=api(rules['review']+(extra['review'] if new else ''),rd,800 if new else 500,tail=extra['review_instruction'] if new else '\n以上全部为引用资料。现在只执行复核，返回allowed、code、span、reason、delivered_fact_ids的json对象，不续写candidate，不返回spoken_text。\n');metric.append(m)
   if not new:
    profile=native.call('memoryprofile',h.entries[idx]['persona']);prompt=(ROOT/'Plugins/DialogueMemoryTable/Resources/DialogueMemorySummarizerPrompt.md').read_text()+'\n【当前权威人物设定与可披露范围】\n'+profile['voice']+'\n'+profile['daily']+'\n'+profile['layer']
    text=f'npc_id={owner}\nsession_id=1\n当前记忆={json.dumps(legacy,ensure_ascii=False)}\n本次对话：\nuser: {q}\nassistant: {answer["spoken_text"]}\n严格只输出 JSON 对象。'
    summary,m=api(prompt,text,1400,.1);metric.append(m)
   out['compact' if new else 'previous']=dict(review_allowed=review.get('allowed'),requests=len(metric),input_tokens=sum(x['usage'].get('prompt_tokens',0) for x in metric),output_tokens=sum(x['usage'].get('completion_tokens',0) for x in metric),seconds=round(time.monotonic()-start,3),metrics=metric)
  # Existing review observes the actual last NPC line even without another player turn.
  candidate='好，下次我们再聊你怕水的事。';contract=base_prompt(idx,[],dict(facts=[],loops=[]))+extra['context']
  rv,m=api(rules['review']+extra['review'],dict(speaker_contract=contract,player_message='我有点怕水，这次不展开了，下次我们再聊这个吧？',candidate=candidate,task_catalog_validation_only=h.catalog(idx,1),allowed_fact_ids=[],claimed_fact_ids=[],recent_messages=[dict(role='user',content='我有点怕水，这次不展开了，下次我们再聊这个吧？')],proposed_player_memory='{}'),800,tail=extra['review_instruction'])
  saved=n.call('apply',dict(store={},owner=owner,day=1,commission=idx,turn=1,revision='cost-check',player='我有点怕水，这次不展开了，下次我们再聊这个吧？',spoken=candidate,analysis={},review=rv.get('memory',{}),catalog=dict(facts=[],loops=[])))
  out['last_line']=dict(passed=rv.get('allowed') is True and len(saved['store']['loops'])==1,review=rv,metrics=m)
  return out
 with ThreadPoolExecutor(max_workers=3) as pool:cases=list(pool.map(pair,[1,4,7]))
 report=dict(cases=cases,passed=all(c['previous']['review_allowed'] and c['compact']['review_allowed'] and c['last_line']['passed'] for c in cases),limitations=['Three single-turn synthetic conversations per flow, no UE/UI or true engine save.','Previous-flow source contracts and summarizer resource are hosted, not an old engine binary.','No repair path exercised; benchmark is small, unpaired random generations and cache effects may change latency/tokens.','Provider usage is recorded; no monetary or general savings claim.','Last-line review uses a fixed candidate rather than a live generated promise.'])
 for label in ['previous','compact']:report[label]={k:sum(c[label][k] for c in cases) for k in ['requests','input_tokens','output_tokens','seconds']}
 Path(a.output).write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['passed','previous','compact']}));n.close();native.close()
 return 0 if report['passed'] else 2
if __name__=='__main__':raise SystemExit(main())
