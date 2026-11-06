#!/usr/bin/env python3
"""Labeled semantic proposals checked through native evidence/progress rules."""
import argparse,json,os,ssl,urllib.request,hashlib
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from dialogue_policy_bridge import NativePolicy,BASE
from dialogue_fact_host import FactHost

def main():
 p=argparse.ArgumentParser();p.add_argument('--output',required=True);a=p.parse_args()
 n=NativePolicy();h=FactHost(n);rule=n.call('rules','analysis')
 source=BASE/'Private/AlchemyDialogueKnowledge.cpp';frozen=hashlib.sha256(source.read_bytes()).hexdigest()
 offer='新炉壁得先烘干，才好开炉。我需要一份烘炉壁的药剂，你愿意帮我做吗？'
 # name, current layer, player text, allowed final layer range, accepted, display, offer
 cases=[
 ('negated_keywords',1,'我不是问安置办，也不问人被带走，只问炉子该怎么烘。',1,2,False,False,offer),
 ('read_label',1,'把安置办三个字读一遍，不要谈你的经历。',1,1,False,False,offer),
 ('quoted_question',1,'“你在安置办做了什么”是书里的台词，不是我问你。',1,1,False,False,offer),
 ('not_asking_past',1,'今天先不聊你以前拆东西，我只问在哪里合成。',1,1,False,False,offer),
 ('natural_why',1,'为什么要烘干炉壁，你以前做过类似的活吗？',2,2,False,False,offer),
 ('deep_question',2,'那些人后来去了哪里，你当时觉得自己做对了吗？',3,3,False,False,offer),
 ('display_not_unlock',1,'这只铜铃标签上写着安置办和被带走，请看看。',1,1,False,True,offer),
 ('quoted_accept',1,'我向别人转述过“这份委托我来做”，但我没有答应你。',1,2,False,False,offer),
 ('refuse',1,'这份委托我不接，我要回家睡觉。',1,2,False,False,offer),
 ('accept',1,'我来做这份烘炉壁的药剂，会回小屋调配封瓶。',1,2,True,False,offer),
 ('ask_not_accept',1,'你是想让我来做烘炉壁的药剂吗？',1,2,False,False,offer),
 ('no_offer',1,'好，我来做。',1,2,False,False,'今天天气很好。')]
 key=os.environ['FOREST_DIALOGUE_API_KEY'];base=os.getenv('FOREST_DIALOGUE_BASE_URL','https://api.deepseek.com/v1').rstrip('/');assert base.startswith('https://');model=os.getenv('FOREST_DIALOGUE_MODEL','deepseek-chat')
 def check(c):
  name,layer,q,lo,hi,accepted,display,opening=c;messages=[dict(role='assistant',content=opening),dict(role='user',content=q)]
  data=dict(task_catalog=h.catalog(1,layer),may_accept=not display,presentation=display,current_layer=layer,already_delivered_fact_ids=[],unlock_topics=[x for x in h.profiles['blacksmith']['unlocks'] if x['layer']>layer],messages=[dict(index=i,**m) for i,m in enumerate(messages)])
  payload=dict(model=model,messages=[dict(role='system',content=rule),dict(role='user',content=json.dumps(data,ensure_ascii=False))],temperature=0,max_tokens=850,response_format={'type':'json_object'})
  out=dict(name=name,question=q,expected_layer_range=[lo,hi],expected_accepted=accepted)
  try:
   req=urllib.request.Request(base+'/chat/completions',data=json.dumps(payload,ensure_ascii=False).encode(),headers={'Content-Type':'application/json','Authorization':'Bearer '+key})
   with urllib.request.urlopen(req,timeout=35,context=ssl.create_default_context()) as response:body=json.load(response)
   d=json.loads(body['choices'][0]['message']['content']);requested=d.get('requested_layer',layer);proof=d.get('layer_evidence','')
   valid_proof=type(requested) is int and isinstance(proof,str) and bool(proof.strip()) and proof in q
   promoted=n.call('promote',layer,requested,int(valid_proof),int(d.get('has_unlock_evidence') is True),int(display)) if type(requested) is int else False
   final=requested if promoted else layer;oi=d.get('offer_index',-1);ui=d.get('accept_index',-1);oq=d.get('offer_quote','');uq=d.get('accept_quote','')
   saved=False
   if type(oi) is int and type(ui) is int and 0<=oi<2 and 0<=ui<2 and isinstance(oq,str) and isinstance(uq,str):
    saved=n.call('accept',int(d.get('accepted') is True and not display),oi,ui,2,int(messages[oi]['role']=='assistant'),int(messages[ui]['role']=='user'),int(bool(oq) and oq in messages[oi]['content']),int(bool(uq) and uq in messages[ui]['content']))
   out.update(plan=d,final_layer=final,accepted=bool(saved),returned_model=body.get('model'),passed=lo<=final<=hi and bool(saved)==accepted)
  except Exception as error:out.update(passed=False,error_type=type(error).__name__)
  return out
 with ThreadPoolExecutor(max_workers=3) as pool:results=list(pool.map(check,cases))
 report=dict(passed=all(c['passed'] for c in results),passed_count=sum(c['passed'] for c in results),total=len(results),cases=results,source_hash=frozen,source_unchanged=hashlib.sha256(source.read_bytes()).hexdigest()==frozen,limitations=['Real semantic API and native policy, hosted JSON/HTTP/evidence; no UE save or UI execution.','Finite labeled cases, not a guarantee for every negation or quoted instruction.'])
 Path(a.output).write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['passed','passed_count','total','source_unchanged']}));n.close();return 0 if report['passed'] else 2

if __name__=='__main__':raise SystemExit(main())
