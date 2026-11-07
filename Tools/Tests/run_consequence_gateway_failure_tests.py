#!/usr/bin/env python3
"""Compile the production consequence gateway with a prompt-builder double.

An invalid configuration/task must never dispatch an empty task to the model.
This tests the public gateway, not consequence writing or scoring.
"""
from pathlib import Path
import subprocess
import tempfile
from run_dialogue_cancellation_tests import method

ROOT = Path(__file__).resolve().parents[2]
service = (ROOT / 'Plugins/DialogueMemoryTable/Source/DialogueMemoryTable/Private/DialogueAIServiceSubsystem.cpp').read_text()
source = r'''
#include <cassert>
#include <functional>
#include <iostream>
#include <string>
#include <vector>
#define TEXT(s) s
using int32=int;
struct FString:std::string{using std::string::string;bool IsEmpty()const{return empty();}};
template<class T>struct TArray:std::vector<T>{void Add(const T& V){this->push_back(V);}};
struct FAlchemyEvaluationResult{};
struct FDialogueMessage{FString Role,Content;};
struct FDialogueAICompletion{std::function<void(bool,const FString&)> Fn;void ExecuteIfBound(bool S,const FString& T)const{if(Fn)Fn(S,T);}};
class UAlchemyGameplaySubsystem{
 public:bool ConfigValid=true;
 FString BuildWorldConsequencePrompt(int Id,const FAlchemyEvaluationResult&,const FString&)const{return ConfigValid&&Id==1?TEXT("reviewed task and result"):TEXT("");}
};
struct GameInstance{UAlchemyGameplaySubsystem* Gameplay=nullptr;template<class T>T* GetSubsystem(){return Gameplay;}};
class UDialogueAIServiceSubsystem{
 public:GameInstance* Instance=nullptr;int Requests=0;FString LastPrompt;
 GameInstance* GetGameInstance(){return Instance;}
 void RequestChatCompletion(const TArray<FDialogueMessage>& M,float,int,FDialogueAICompletion C){++Requests;LastPrompt=M[0].Content;C.ExecuteIfBound(true,TEXT("generated"));}
 void RequestWorldConsequence(int32,const FAlchemyEvaluationResult&,const FString&,FDialogueAICompletion);
};
'''
source += method(service, 'UDialogueAIServiceSubsystem::RequestWorldConsequence')
source += r'''
int main(){
 UDialogueAIServiceSubsystem S;GameInstance I;UAlchemyGameplaySubsystem G;int Callbacks=0;bool Success=false;FString Error;
 FDialogueAICompletion C;C.Fn=[&](bool Ok,const FString& Text){++Callbacks;Success=Ok;Error=Text;};
 S.RequestWorldConsequence(1,{},TEXT("{}"),C);assert(Callbacks==1&&!Success&&S.Requests==0);
 S.Instance=&I;S.RequestWorldConsequence(1,{},TEXT("{}"),C);assert(Callbacks==2&&!Success&&S.Requests==0);
 I.Gameplay=&G;G.ConfigValid=false;S.RequestWorldConsequence(1,{},TEXT("{}"),C);assert(Callbacks==3&&!Success&&Error==TEXT("commission_missing")&&S.Requests==0);
 G.ConfigValid=true;S.RequestWorldConsequence(99,{},TEXT("{}"),C);assert(Callbacks==4&&!Success&&S.Requests==0);
 S.RequestWorldConsequence(1,{},TEXT("{}"),C);assert(Callbacks==5&&Success&&S.Requests==1&&S.LastPrompt==TEXT("reviewed task and result"));
 std::cout<<"PASS: production consequence gateway rejects unavailable gameplay, invalid config and missing task before dispatch; valid task still dispatches\n";
}
'''
with tempfile.TemporaryDirectory(prefix='forest-consequence-gateway-') as temp:
    path=Path(temp)
    (path/'test.cpp').write_text(source)
    subprocess.run(['c++','-std=c++17',str(path/'test.cpp'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
