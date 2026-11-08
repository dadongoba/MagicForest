"""Compile production compact save/retry bodies with a deterministic save adapter."""
import subprocess,tempfile
from pathlib import Path
from compact_memory_bridge import BASE,method

def run():
 src=(BASE/'Private/DialogueMemoryTableSubsystem.cpp').read_text()
 code=r'''
#include <cassert>
#include <memory>
#include <string>
#include <vector>
#include <iostream>
#define TEXT(x) x
using FString=std::string;using int32=int;
template<class T>struct TArray:std::vector<T>{using std::vector<T>::vector;int Num()const{return this->size();}bool IsEmpty()const{return this->empty();}void Reset(){this->clear();}void Add(const T& t){this->push_back(t);}T& Last(){return this->back();}template<class F>bool ContainsByPredicate(F f){for(auto& x:*this)if(f(x))return true;return false;}void RemoveAt(int n){this->erase(this->begin()+n);}};
'''
 code+='#include "'+str(BASE/'Public/DialogueCompactMemoryPolicy.h')+'"\n'
 code+=r'''
struct UDialogueMemorySaveGame{int SchemaVersion=2,AuthoritativeLevel=1,WorldDay=1;DialogueCompactMemory::Store Store;};
std::vector<std::unique_ptr<UDialogueMemorySaveGame>> Objects;
template<class T,class U>T* DuplicateObject(T* from,U*){Objects.push_back(std::make_unique<T>(*from));return Objects.back().get();}
struct Ref{UDialogueMemorySaveGame* p=nullptr;auto Get(){return p;}auto operator->(){return p;}explicit operator bool()const{return p;}Ref& operator=(UDialogueMemorySaveGame* x){p=x;return *this;}};
namespace DialogueCompactMemoryRuntime{auto Read(const DialogueCompactMemory::Store& x){return x;}void Write(const DialogueCompactMemory::Store& x,UDialogueMemorySaveGame& y){y.Store=x;}}
struct UDialogueMemoryTableSubsystem{
 Ref Ledger;TArray<DialogueCompactMemory::Delta> PendingCompactUpdates;bool SaveOK=true;UDialogueMemorySaveGame Disk;
 void EnsureLedger(){}auto GetLedgerSnapshot(){return Ledger->Store;}
 bool SaveLedger(FString& e){if(!SaveOK){e="save_failed";return false;}Disk=*Ledger.Get();return true;}
 bool ApplyCompactTurn(const DialogueCompactMemory::Delta&,FString&);bool RetryCompactSaves(FString&);
};
'''
 code+=method(src,'UDialogueMemoryTableSubsystem::ApplyCompactTurn')+method(src,'UDialogueMemoryTableSubsystem::RetryCompactSaves')
 code+=r'''
DialogueCompactMemory::Delta delta(int turn,std::string kind="preference"){
 DialogueCompactMemory::Delta d;d.owner="natta";d.day=1;d.turn=turn;
 DialogueCompactMemory::Fact f;f.owner=d.owner;f.id="m."+std::to_string(turn);f.kind=kind;f.text="preference"+std::to_string(turn);f.evidence="I said this";f.day=1;f.turn=turn;d.facts.push_back(f);return d;
}
int main(){
 UDialogueMemorySaveGame initial;UDialogueMemoryTableSubsystem m;m.Ledger=&initial;m.SaveOK=false;FString error;
 assert(!m.ApplyCompactTurn(delta(1),error));assert(m.Ledger.Get()==&initial&&initial.Store.facts.empty()&&m.PendingCompactUpdates.Num()==1);
 // An authoritative gameplay change made after failed memory save must survive replay.
 initial.AuthoritativeLevel=3;initial.WorldDay=2;m.SaveOK=true;assert(m.ApplyCompactTurn(delta(2),error));
 assert(m.Disk.AuthoritativeLevel==3&&m.Disk.WorldDay==2&&m.Disk.Store.facts.size()==2&&m.PendingCompactUpdates.IsEmpty());
 auto count=m.Disk.Store.facts.size();assert(m.ApplyCompactTurn(delta(2),error));assert(m.Disk.Store.facts.size()==count);
 m.SaveOK=false;for(int i=3;i<20;++i)m.ApplyCompactTurn(delta(i),error);assert(m.PendingCompactUpdates.Num()==3);
 m.SaveOK=true;assert(m.RetryCompactSaves(error));assert(m.PendingCompactUpdates.IsEmpty()&&m.Disk.Store.states[0].turn==19);
 std::cout<<"save rollback, structured bounded retry, fresh authority, idempotence passed";
}
'''
 with tempfile.TemporaryDirectory(prefix='compact-memory-tx-') as td:
  p=Path(td);(p/'tx.cpp').write_text(code)
  subprocess.run(['clang++','-std=c++17','-fsanitize=address,undefined',str(p/'tx.cpp'),'-o',str(p/'tx')],check=True)
  result=subprocess.check_output([str(p/'tx')]).decode()
 return dict(passed=True,result=result,production_methods=['ApplyCompactTurn','RetryCompactSaves'],limitations=['SaveGame disk writes and UObject copies use deterministic adapters; not engine serialization.'])
if __name__=='__main__':print(run())
