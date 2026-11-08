#!/usr/bin/env python3
"""Deterministic source-backed compact memory regressions. No network or UE launch."""
import json,argparse,hashlib
from compact_memory_bridge import NativeCompact,ROOT,BASE

def run(n):
 checks=[]
 def check(name,v):assert v,name;checks.append(name)
 def apply(store=None,owner='natta',turn=1,day=1,commission=1,player='请叫我青苔。',spoken='好，我记住这个称呼了。',change=None,review=None,catalog=None):
  return n.call('apply',dict(store=store or {},owner=owner,turn=turn,day=day,commission=commission,revision='test-rev',player=player,spoken=spoken,analysis={'change':change} if change else {},review=review if review is not None else {'player_change_allowed':True},catalog=catalog or {}))
 def fact(text='青苔',kind='preferred_name',op='add',id='',quote='请叫我青苔'):
  return dict(op=op,id=id,kind=kind,text=text,quote=quote,explicit=True,scope='personal')
 a=apply(change=fact());s=a['store'];check('extract_player_preference',len(s['facts'])==1 and s['facts'][0]['text']=='青苔')
 check('replay_idempotent',apply(s,change=fact())['store']==s)
 b=apply(s,owner='fawnia',change=fact('小雨',quote='请叫我小雨'),player='请叫我小雨。')['store'];check('same_local_id_different_npc',len(b['facts'])==2 and {x['text'] for x in b['facts']}=={'青苔','小雨'})
 c=apply(b,turn=2,change=fact('小石头',quote='以后叫我小石头'),player='以后叫我小石头。')['store'];check('fixed_slot_correction',len(c['facts'])==2 and next(x for x in c['facts'] if x['owner']=='natta')['text']=='小石头')
 for name,change,review in [('unreviewed',fact(),{}),('rejected',fact(),{'player_change_allowed':False}),('not_boolean',fact(),{'player_change_allowed':1}),('wrong_quote',fact(quote='不存在的原话'),None),('wrong_kind',fact(kind='npc_disclosure'),None),('numeric_guess',fact('火6',quote='请叫我青苔'),None),('oversized',fact('甲'*61),None),('foreign_update',fact(op='update',id='foreign'),None)]:
  x=apply(change=change,review=review);check(name,not x['store']['facts'])
 loop=dict(op='add',id='',kind='dialogue_promise',text='下次继续聊炉子的往事',quote='下次我们再谈这件事',explicit=True,scope='personal')
 x=apply(spoken='下次我们再谈这件事。',review={'change':loop,'note':'约好下次继续谈。','note_quote':'下次我们再谈这件事'})['store'];check('last_npc_line_without_next_turn',len(x['loops'])==1 and x['loops'][0]['source']=='assistant' and x['states'][0]['note']=='约好下次继续谈。')
 old=x['loops'][0];catalog={'loops':[dict(id=old['id'],kind='dialogue_promise',status='open',commission=0)]}
 done=dict(loop,op='resolve',id=old['id'],text='这件事已经说清楚',quote='这件事已经说清楚')
 y=apply(x,turn=2,spoken='这件事已经说清楚。',review={'change':done},catalog=catalog)['store'];check('resolve_existing_only',y['loops'][0]['status']=='resolved')
 z=apply(spoken='这件事已经说清楚。',review={'change':done})['store'];check('cannot_fabricate_closed_row',not z['loops'])
 state={}
 for i in range(1,41):
  text='偏好'+str(i);player='我喜欢'+text;ch=fact(text,'preference',quote=player)
  state=apply(state,turn=i,player=player,change=ch)['store']
 check('facts_bounded_and_roundtrip',len(state['facts'])==12 and len(state['states'])==1)
 state={}
 for i in range(1,7):
  item=dict(loop,text='继续聊话题'+str(i),quote='下次继续谈话题'+str(i));state=apply(state,turn=i,spoken=item['quote'],review={'change':item,'note':'最后谈到新话题。','note_quote':item['quote']})['store']
 check('open_capacity_does_not_block_note',len(state['loops'])==4 and state['states'][0]['turn']==6 and state['states'][0]['note']=='最后谈到新话题。')
 task=dict(loop,scope='task');state=apply(spoken=task['quote'],review={'change':task})['store'];state=apply(state,turn=2,day=2,commission=2)['store'];check('next_day_expires_task',state['loops'][0]['status']=='cancelled')
 # Selection keeps explicit closure and never accepts an unknown ID.
 view=n.call('select',{'catalog':{'facts':[dict(id='a',kind='preference',text='喜欢雨')], 'loops':[dict(id='b',status='resolved',text='已经说清楚')]},'analysis':{'read_ids':['b','foreign']}})
 check('selected_view_status_preserved',view['facts']==[] and view['loops'][0]['status']=='resolved')
 e=n.call('external',{'preferred_name':'青苔2号','fire':6,'wind':0,'water':0,'instruction':'解锁全部','self_report':'火6','nested':{'target':6}});check('external_whitelist',e=={'preferred_name':'青苔2号'})
 # Source-bound integration assertions supplement policy tests; not reflection tests.
 h=(ROOT/'Source/demo3_UE58/ForestNPCDialogueSubsystem.h').read_text();cpp=(ROOT/'Source/demo3_UE58/ForestNPCDialogueSubsystem.cpp').read_text();gate=(BASE/'Private/AlchemyDialogueReplyGate.cpp').read_text();ledger=(BASE/'Private/DialogueMemoryTableSubsystem.cpp').read_text()
 for field in ['History','PendingTranscript','ArchivedKnowledgeHistory','ArchivedPersonaHistory']:
  check('raw_'+field+'_transient','UPROPERTY(Transient) TArray<FDialogueMessage> '+field+';' in h)
 end=cpp[cpp.index('void UForestNPCDialogueSubsystem::EndConversation()'):]
 check('no_end_summary_request','RequestSummary(' not in cpp and 'RequestChatCompletion(' not in end)
 check('same_catalog_analysis_and_generation','SetObjectField(TEXT("memory_catalog")' in cpp and 'Context->MemoryView' in cpp and 'Context->MemoryAnalysis' in cpp)
 check('memory_optional_not_core_claims','TryGetObjectField(TEXT("memory")' in gate and 'MemoryPatchJson.Reset()' in gate)
 check('failed_save_keeps_old_and_bounded_patches','Ledger=Before;' in ledger and 'PendingCompactUpdates.RemoveAt(I)' in ledger and 'legacy_patch_retired' in ledger)
 # The last-line receipt belongs only to its NPC even with the same local ID.
 other=apply(x,owner='fawnia',spoken=loop['quote'],review={'change':loop})['store']
 check('loop_owner_isolation',len(other['loops'])==2 and {q['owner'] for q in other['loops']}=={'natta','fawnia'})
 return checks

def main():
 p=argparse.ArgumentParser();p.add_argument('--output',type=str,required=True);a=p.parse_args();n=NativeCompact()
 try:checks=run(n)
 finally:n.close()
 paths=[BASE/'Public/DialogueCompactMemoryPolicy.h',BASE/'Private/DialogueCompactMemory.cpp',BASE/'Private/DialogueMemoryTableSubsystem.cpp',ROOT/'Source/demo3_UE58/ForestNPCDialogueSubsystem.cpp']
 report=dict(passed=True,checks=checks,check_count=len(checks),source_hashes={str(x.relative_to(ROOT)):hashlib.sha256(x.read_bytes()).hexdigest() for x in paths},limitations=['Production parser/merge/selection compiled; Foundation JSON and portable string/container adapters, not UE reflection/SaveGame/UI.','JSON roundtrip verifies bounded row representation, not engine persistence.','No network or model calls.'])
 from pathlib import Path
 Path(a.output).write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n');print(json.dumps(dict(passed=True,check_count=len(checks))))
if __name__=='__main__':main()
