#!/usr/bin/env python3
"""Execute production cancellation/Finish bodies with synchronous callback doubles.

No UE launch or HTTP: this tests ownership and reentrancy, not engine scheduling.
"""
import json
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
BASE = ROOT / 'Plugins/DialogueMemoryTable/Source/DialogueMemoryTable'


def method(source, name):
    match = re.search(r'(?:void|bool) ' + re.escape(name) + r'\(', source)
    assert match, name
    start = source.index('{', match.start())
    end, depth = start + 1, 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end]


def main():
    service = (BASE / 'Private/DialogueAIServiceSubsystem.cpp').read_text()
    gate = (BASE / 'Private/AlchemyDialogueReplyGate.cpp').read_text()
    owner = (ROOT / 'Source/demo3_UE58/ForestNPCDialogueSubsystem.cpp').read_text()
    close = method(owner, 'UForestNPCDialogueSubsystem::EndConversation')
    assert close.index('ClosingScope=ActiveTurnScope') < close.index('ActiveTurnScope.Reset()') < close.index('CancelNpcTurns(ClosingScope)')
    source = r'''
#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#define TEXT(s) s
using FString=std::string;
using FName=std::string;
template<class T>using TSharedPtr=std::shared_ptr<T>;
template<class T>struct TArray:std::vector<T>{using std::vector<T>::vector;void Reset(){this->clear();}};
struct FPlatformTime{static double Seconds(){return 10.;}};
struct Request{int CancelCount=0;std::function<void()> Callback;void CancelRequest(){++CancelCount;if(Callback)Callback();}};
struct FDialogueRequestScope{bool bCancelled=false;double Deadline=100.;TArray<TSharedPtr<Request>> Requests;bool IsActive()const;void Cancel();};
struct FAlchemyDialogueTurnResult{bool bSuccess=false,bFallback=false;FString Text,Error,ReviewCode;TArray<FName> DeliveredFacts;};
struct ReplyCallback{std::function<void(const FAlchemyDialogueTurnResult&)> Fn;void ExecuteIfBound(const FAlchemyDialogueTurnResult& R)const{if(Fn)Fn(R);}};
struct LegacyCallback{std::function<void(bool,const FString&)> Fn;void ExecuteIfBound(bool S,const FString& T)const{if(Fn)Fn(S,T);}};
class UAlchemyDialogueReplyGate;
class UDialogueAIServiceSubsystem{
 public:TArray<TSharedPtr<UAlchemyDialogueReplyGate>> NpcReplyGates;
 void CancelNpcTurns(TSharedPtr<FDialogueRequestScope> Scope);
 void ReleaseNpcReplyGate(UAlchemyDialogueReplyGate* Gate){auto& G=NpcReplyGates;G.erase(std::remove_if(G.begin(),G.end(),[Gate](const auto& P){return P.get()==Gate;}),G.end());}
};
struct ServiceRef{UDialogueAIServiceSubsystem* Ptr=nullptr;bool IsValid()const{return Ptr!=nullptr;}UDialogueAIServiceSubsystem* operator->()const{return Ptr;}};
class UAlchemyDialogueReplyGate{
 public:bool bFinished=false;FString LastReviewCode;ServiceRef Service;ReplyCallback Result;LegacyCallback LegacyResult;TSharedPtr<FDialogueRequestScope> Scope;
 void Finish(bool Success,const FString& Text,bool Fallback=false,const TArray<FName>& Delivered={});
};
'''
    source += '\n'.join([
        method(service, 'FDialogueRequestScope::IsActive'),
        method(service, 'FDialogueRequestScope::Cancel'),
        method(service, 'UDialogueAIServiceSubsystem::CancelNpcTurns'),
        method(gate, 'UAlchemyDialogueReplyGate::Finish'),
    ])
    source += r'''
TSharedPtr<UAlchemyDialogueReplyGate> Add(UDialogueAIServiceSubsystem& S,const TSharedPtr<FDialogueRequestScope>& Scope){auto G=std::make_shared<UAlchemyDialogueReplyGate>();G->Service.Ptr=&S;G->Scope=Scope;S.NpcReplyGates.push_back(G);return G;}
int main(){
 // Direct alias reset in a synchronous completion: the original crash trigger.
 {UDialogueAIServiceSubsystem S;S.CancelNpcTurns(nullptr);auto Active=std::make_shared<FDialogueRequestScope>();auto G=Add(S,Active);auto Keep=Active;int Calls=0;
  G->Result.Fn=[&](const auto& R){++Calls;assert(!R.bSuccess&&Keep->bCancelled);Active.reset();};
  S.CancelNpcTurns(Active);assert(!Active&&Keep->bCancelled&&Calls==1&&S.NpcReplyGates.empty());
  S.CancelNpcTurns(Keep);G->Finish(true,"late success");assert(Calls==1);}
 // Two gates in one scope, recursive cancellation, and a different active scope.
 {UDialogueAIServiceSubsystem S;auto Active=std::make_shared<FDialogueRequestScope>();auto Keep=Active;auto Other=std::make_shared<FDialogueRequestScope>();auto A=Add(S,Active),B=Add(S,Active),C=Add(S,Other);int AC=0,BC=0,CC=0;
  A->Result.Fn=[&](const auto&){++AC;auto Old=Active;Active.reset();S.CancelNpcTurns(Old);};B->Result.Fn=[&](const auto&){++BC;};C->Result.Fn=[&](const auto&){++CC;};
  S.CancelNpcTurns(Active);assert(AC==1&&BC==1&&CC==0&&Keep->bCancelled&&!Other->bCancelled&&S.NpcReplyGates.size()==1);
  S.CancelNpcTurns(Other);assert(CC==1&&S.NpcReplyGates.empty());}
 // HTTP cancellation itself completes synchronously and re-enters cancellation.
 {UDialogueAIServiceSubsystem S;auto Active=std::make_shared<FDialogueRequestScope>();auto Keep=Active;auto G=Add(S,Active);auto R=std::make_shared<Request>();Active->Requests.push_back(R);int Calls=0;
  G->Result.Fn=[&](const auto& Out){++Calls;assert(!Out.bSuccess);Active.reset();};
  R->Callback=[&](){assert(Keep->bCancelled);S.CancelNpcTurns(Active);G->Finish(false,"cancelled HTTP");};
  S.CancelNpcTurns(Active);S.CancelNpcTurns(Keep);G->Finish(true,"stale response");assert(R->CancelCount==1&&Calls==1&&Keep->Requests.empty());}
 std::cout<<"PASS: alias reset, recursive/multiple gates, synchronous HTTP, repeat cancel and late completion\n";
}
'''
    with tempfile.TemporaryDirectory(prefix='forest-cancel-regression-') as temp:
        path = Path(temp)
        (path / 'test.cpp').write_text(source)
        subprocess.run(['c++', '-std=c++17', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer', str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
        subprocess.run([str(path / 'test')], check=True)
    print(json.dumps({'production_bodies': True, 'sanitizers': ['address', 'undefined'], 'ue_runtime_tested': False}))


if __name__ == '__main__':
    main()
