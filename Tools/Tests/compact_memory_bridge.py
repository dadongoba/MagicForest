"""Native production compact parser/merge/selection, with Foundation JSON and portable UE value adapters.

Does not launch UE or emulate UObject/SaveGame/reflection/GUI. Text and containers are
adapted; compiled methods are read from the production source on every invocation.
"""
from pathlib import Path
import json,re,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
BASE=ROOT/'Plugins/DialogueMemoryTable/Source/DialogueMemoryTable'

def method(s,name):
 m=re.search(r'(?:bool|FString|FName|DialogueCompactMemory::Delta)\s+'+re.escape(name)+r'\(',s);assert m,name
 a=s.index('{',m.start());b=a+1;depth=1
 while depth:depth+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[m.start():b]

SHIM=r'''
#import <Foundation/Foundation.h>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <regex>
#include <codecvt>
#include <locale>
#include <sstream>
#include <iostream>
#include <cassert>
#define TEXT(s) L##s
using TCHAR=wchar_t;using int32=int;using int64=long long;
std::wstring_convert<std::codecvt_utf8<wchar_t>> cv;
struct FString:std::wstring {
 using std::wstring::wstring;using std::wstring::operator=;
 FString()=default;FString(const std::wstring& x):std::wstring(x){}
 int Len()const{return size();}bool IsEmpty()const{return empty();}const wchar_t* operator*()const{return c_str();}
 bool Contains(const FString& x)const{return find(x)!=npos;}
 FString TrimStartAndEnd()const{auto a=find_first_not_of(L" \r\n\t"),b=find_last_not_of(L" \r\n\t");return a==npos?FString():substr(a,b-a+1);}
 template<class A>static std::wstring Arg(A a){std::wostringstream o;o<<a;return o.str();}
 template<class... A>static FString Printf(const TCHAR* f,A... a){std::vector<std::wstring> args={Arg(a)...};FString out;size_t n=0;for(size_t i=0;f[i];++i){if(f[i]==L'%'){while(f[i]&&f[i]!=L's'&&f[i]!=L'd')++i;out+=args.at(n++);}else out+=f[i];}return out;}
};
struct FName:FString{using FString::FString;FName()=default;FName(const FString& s):FString(s){}FString ToString()const{return *this;}};FName NAME_None;
namespace ESearchCase{const bool IgnoreCase=true;}
// Retired binding matching uses this overload.
bool insensitiveContains(const FString& s,const FString& q){auto a=s,b=q;for(auto& c:a)c=towlower(c);for(auto& c:b)c=towlower(c);return a.Contains(b);}
template<class T>struct TArray:std::vector<T>{using std::vector<T>::vector;int Num()const{return this->size();}void Add(const T& x){this->push_back(x);}};
template<class T>using TSharedPtr=std::shared_ptr<T>;
template<class T,class... A>auto MakeShared(A&&... a){return std::make_shared<T>(std::forward<A>(a)...);}
struct FRegexPattern{std::wregex rx;FRegexPattern(const wchar_t* s){std::wstring p=s;if(p.find(L"(?i)")==0)p=p.substr(4);rx=std::wregex(p,std::regex::icase);}};
struct FRegexMatcher{FRegexPattern p;FString s;FRegexMatcher(const FRegexPattern& x,const FString& y):p(x),s(y){}bool FindNext(){return std::regex_search(s,p.rx);}};
struct FJsonObject;
struct FJsonValue{TSharedPtr<FJsonObject> o;FString s;bool isString=false;virtual ~FJsonValue()=default;auto AsObject()const{return o;}bool TryGetString(FString& v)const{if(!isString)return false;v=s;return true;}};
struct FJsonObject{
 std::map<FString,FString> strings;std::map<FString,bool> booleans;std::map<FString,double> numbers;std::map<FString,TSharedPtr<FJsonObject>> objects;std::map<FString,TArray<TSharedPtr<FJsonValue>>> arrays;
 bool TryGetStringField(const TCHAR* k,FString& v)const{auto i=strings.find(k);if(i==strings.end())return false;v=i->second;return true;}
 bool TryGetNumberField(const TCHAR* k,double& v)const{auto i=numbers.find(k);if(i==numbers.end())return false;v=i->second;return true;}
 bool TryGetBoolField(const TCHAR* k,bool& v)const{auto i=booleans.find(k);if(i==booleans.end())return false;v=i->second;return true;}
 bool TryGetObjectField(const TCHAR* k,const TSharedPtr<FJsonObject>*& v)const{auto i=objects.find(k);if(i==objects.end())return false;v=&i->second;return true;}
 bool TryGetArrayField(const TCHAR* k,const TArray<TSharedPtr<FJsonValue>>*& v)const{auto i=arrays.find(k);if(i==arrays.end())return false;v=&i->second;return true;}
 void SetStringField(const TCHAR* k,const FString& v){strings[k]=v;}
 void SetArrayField(const TCHAR* k,const TArray<TSharedPtr<FJsonValue>>& a){arrays[k]=a;}
};
struct FJsonValueObject:FJsonValue{FJsonValueObject(TSharedPtr<FJsonObject> x){o=x;}};
std::string U(const FString& s){return cv.to_bytes(s);}FString T(const std::string& s){return cv.from_bytes(s);}
NSString* NS(const std::string& x){return [NSString stringWithUTF8String:x.c_str()];}
std::string bytes(id x){return x?std::string([[(NSString*)x description] UTF8String]):"";}
NSDictionary* decode(const std::string& x){NSData* d=[NSData dataWithBytes:x.data() length:x.size()];id v=[NSJSONSerialization JSONObjectWithData:d options:0 error:nil];return [v isKindOfClass:[NSDictionary class]]?v:nil;}
std::string encode(id x){NSData* d=[NSJSONSerialization dataWithJSONObject:x?:@{} options:NSJSONWritingFragmentsAllowed error:nil];return std::string((const char*)d.bytes,d.length);}
TSharedPtr<FJsonObject> from(id obj){if(![obj isKindOfClass:[NSDictionary class]])return nullptr;auto o=MakeShared<FJsonObject>();for(NSString* key in obj){id v=obj[key];auto k=T(bytes(key));if([v isKindOfClass:[NSString class]])o->strings[k]=T(bytes(v));else if([v isKindOfClass:[NSDictionary class]])o->objects[k]=from(v);else if([v isKindOfClass:[NSArray class]]){auto& a=o->arrays[k];for(id row in v){auto e=MakeShared<FJsonValue>();if([row isKindOfClass:[NSString class]]){e->s=T(bytes(row));e->isString=true;}else e->o=from(row);a.Add(e);}}else if([v isKindOfClass:[NSNumber class]]){o->numbers[k]=[v doubleValue];if(CFGetTypeID((__bridge CFTypeRef)v)==CFBooleanGetTypeID())o->booleans[k]=[v boolValue];}}return o;}
id to(TSharedPtr<FJsonObject> o){auto d=[NSMutableDictionary dictionary];if(!o)return d;for(auto& [k,v]:o->strings)d[NS(U(k))]=NS(U(v));for(auto& [k,v]:o->numbers)d[NS(U(k))]=@(v);for(auto& [k,v]:o->booleans)d[NS(U(k))]=@(v);for(auto& [k,v]:o->objects)d[NS(U(k))]=to(v);for(auto& [k,v]:o->arrays){auto a=[NSMutableArray array];for(auto& e:v)[a addObject:e->isString?(id)NS(U(e->s)):to(e->o)];d[NS(U(k))]=a;}return d;}
TSharedPtr<FJsonObject> Object(const FString& s){return from(decode(U(s)));}
FString Json(const TSharedPtr<FJsonObject>& o){return T(encode(to(o)));}
FString Str(const TSharedPtr<FJsonObject>& o,const TCHAR* k){FString s;if(o)o->TryGetStringField(k,s);return s;}
const TArray<TSharedPtr<FJsonValue>>* Rows(const TSharedPtr<FJsonObject>& o,const TCHAR* k){const TArray<TSharedPtr<FJsonValue>>* a=nullptr;if(o)o->TryGetArrayField(k,a);return a;}
TSharedPtr<FJsonObject> Child(const TSharedPtr<FJsonObject>& o,const TCHAR* k){const TSharedPtr<FJsonObject>* c=nullptr;return o&&o->TryGetObjectField(k,c)?*c:nullptr;}
'''
MAIN=r'''
using namespace DialogueCompactMemory;
Store load(NSDictionary* d){Store s;for(NSDictionary* x in d[@"facts"]){Fact f;f.owner=bytes(x[@"owner"]);f.id=bytes(x[@"id"]);f.kind=bytes(x[@"kind"]);f.text=bytes(x[@"text"]);f.evidence=bytes(x[@"evidence"]);f.day=[x[@"day"] intValue];f.turn=[x[@"turn"] longLongValue];s.facts.push_back(f);}for(NSDictionary* x in d[@"loops"]){Loop l;l.owner=bytes(x[@"owner"]);l.id=bytes(x[@"id"]);l.kind=bytes(x[@"kind"]);l.text=bytes(x[@"text"]);l.evidence=bytes(x[@"evidence"]);l.status=bytes(x[@"status"]);l.source=bytes(x[@"source"]);l.day=[x[@"day"] intValue];l.turn=[x[@"turn"] longLongValue];l.commission=[x[@"commission"] intValue];s.loops.push_back(l);}for(NSDictionary* x in d[@"states"]){State n;n.owner=bytes(x[@"owner"]);n.note=bytes(x[@"note"]);n.evidence=bytes(x[@"evidence"]);n.revision=bytes(x[@"revision"]);n.day=[x[@"day"] intValue];n.turn=[x[@"turn"] longLongValue];s.states.push_back(n);}return s;}
id dump(const Store& s){auto f=[NSMutableArray array],l=[NSMutableArray array],n=[NSMutableArray array];for(auto& x:s.facts)[f addObject:@{@"owner":NS(x.owner),@"id":NS(x.id),@"kind":NS(x.kind),@"text":NS(x.text),@"evidence":NS(x.evidence),@"day":@(x.day),@"turn":@(x.turn)}];for(auto& x:s.loops)[l addObject:@{@"owner":NS(x.owner),@"id":NS(x.id),@"kind":NS(x.kind),@"text":NS(x.text),@"evidence":NS(x.evidence),@"day":@(x.day),@"turn":@(x.turn),@"commission":@(x.commission),@"status":NS(x.status),@"source":NS(x.source)}];for(auto& x:s.states)[n addObject:@{@"owner":NS(x.owner),@"note":NS(x.note),@"evidence":NS(x.evidence),@"revision":NS(x.revision),@"day":@(x.day),@"turn":@(x.turn)}];return @{@"facts":f,@"loops":l,@"states":n};}
int main(int argc,char** argv){@autoreleasepool{std::ostringstream buf;buf<<std::cin.rdbuf();NSDictionary* d=decode(buf.str());std::string mode=argc>1?argv[1]:"apply";
if(mode=="rules"){std::cout<<encode(@{@"analysis":NS(U(AnalysisRules())),@"review":NS(U(ReviewRules())),@"context":NS(U(ContextRules())),@"review_instruction":NS(U(ReviewInstruction()))});return 0;}
if(mode=="select"){std::cout<<U(Select(T(encode(d[@"catalog"])),T(encode(d[@"analysis"]))));return 0;}
if(mode=="external"){std::cout<<U(External(T(encode(d))));return 0;}
Store s=load(d[@"store"]);auto delta=Parse(FName(T(bytes(d[@"owner"]))),[d[@"day"] intValue],[d[@"commission"] intValue],[d[@"turn"] longLongValue],T(bytes(d[@"revision"])),T(bytes(d[@"player"])),T(bytes(d[@"spoken"])),T(encode(d[@"analysis"])),T(encode(d[@"review"])),T(encode(d[@"catalog"])));bool changed=Apply(s,delta);std::cout<<encode(@{@"changed":@(changed),@"store":dump(s),@"fact_candidates":@(delta.facts.size()),@"loop_candidates":@(delta.loops.size())});}}
'''
class NativeCompact:
 def __init__(self):
  self.temp=tempfile.TemporaryDirectory(prefix='compact-memory-native-');q=Path(self.temp.name);self.exe=q/'bridge'
  src=(BASE/'Private/DialogueCompactMemory.cpp').read_text();knowledge=(BASE/'Private/AlchemyDialogueKnowledge.cpp').read_text();persona=(BASE/'Private/AlchemyPersonaPolicy.cpp').read_text()
  retired=method(persona,'ContainsRetiredBinding').replace('Text.Contains(Word,ESearchCase::IgnoreCase)','insensitiveContains(Text,Word)')
  code=SHIM+'\nnamespace AlchemyDialogueKnowledge{'+method(knowledge,'HasNumericDisclosure')+method(knowledge,'IsSafeReply')+'}\nnamespace AlchemyPersonaPolicy{'+method(persona,'CanonicalId')+retired+'}\n'
  code+='\n#include "'+str(BASE/'Public/DialogueCompactMemoryPolicy.h')+'"\n'
  code+='\n'.join(method(src,n) for n in ['Safe','Parse','Select','External','AnalysisRules','ReviewRules','ContextRules','ReviewInstruction'])
  code+=MAIN
  (q/'bridge.mm').write_text(code)
  subprocess.run(['clang++','-std=c++17','-fobjc-arc','-Wno-nullability-completeness','-framework','Foundation',str(q/'bridge.mm'),'-o',str(self.exe)],check=True,capture_output=True)
 def call(self,mode,data=None):
  return json.loads(subprocess.check_output([str(self.exe),mode],input=json.dumps(data or {},ensure_ascii=False).encode()))
 def close(self):self.temp.cleanup()
