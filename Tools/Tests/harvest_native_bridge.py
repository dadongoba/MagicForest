"""Production daily-loader/value and item-text policies with portable UE adapters.

JSON/file IO, UE containers, CRC and RNG are adapted; this is not an engine host.
The actual policies, loader/getter and item prompt are compiled from the project.
"""
from pathlib import Path
import json,re,subprocess,tempfile

ROOT=Path(__file__).resolve().parents[2]
BASE=ROOT/'Plugins/DialogueMemoryTable/Source/DialogueMemoryTable'

def method(source,name):
 m=re.search(r'(?:bool|FString) '+re.escape(name)+r'\(',source);assert m,name
 a=source.index('{',m.start());end=a+1;depth=1
 while depth:depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[m.start():end]

SHIM=r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <codecvt>
#include <cstdint>
#include <iostream>
#include <locale>
#include <map>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <vector>
#define TEXT(x) L##x
#define MAX_int32 2147483647
#define MoveTemp(x) std::move(x)
using int32=int;using TCHAR=wchar_t;
struct FString:std::wstring {
 using std::wstring::wstring;FString()=default;FString(const std::wstring& s):std::wstring(s){}
 bool IsEmpty()const{return empty();}int32 Len()const{return size();}const TCHAR* operator*()const{return c_str();}
 bool Contains(const TCHAR* s)const{return find(s)!=npos;}
 template<class A>static std::wstring Arg(A a){std::wostringstream o;o<<a;return o.str();}
 template<class... A>static FString Printf(const TCHAR* fmt,A... a){std::vector<std::wstring> args={Arg(a)...};FString out;size_t n=0;for(size_t i=0;fmt[i];++i){if(fmt[i]=='%'&&(fmt[i+1]=='s'||fmt[i+1]=='d')){assert(n<args.size());out+=args[n++];++i;}else out+=fmt[i];}assert(n==args.size());return out;}
};
FString operator/(const FString& a,const TCHAR* b){return a+TEXT("/")+b;}
struct FName:FString{using FString::FString;FName()=default;FString ToString()const{return *this;}};
template<class T>struct TArray:std::vector<T>{using std::vector<T>::vector;void Reset(){this->clear();}int32 Num()const{return this->size();}void Add(const T& v){this->push_back(v);}template<class F>bool ContainsByPredicate(F f)const{for(const auto& v:*this)if(f(v))return true;return false;}template<class F>const T* FindByPredicate(F f)const{for(const auto& v:*this)if(f(v))return &v;return nullptr;}};
template<class T>using TSharedPtr=std::shared_ptr<T>;
struct FMath{static bool IsFinite(double v){return std::isfinite(v);}static double FloorToDouble(double v){return std::floor(v);}};
// Deterministic portable adapters: test ownership/ranges, not Unreal RNG statistics.
struct FCrc{static uint32_t StrCrc32(const TCHAR* s){uint32_t h=2166136261u;while(*s){h^=*s++;h*=16777619u;}return h;}};
struct FRandomStream{std::mt19937 R;FRandomStream(int32 s):R(static_cast<uint32_t>(s)){}int32 RandRange(int32 a,int32 b){return std::uniform_int_distribution<int32>(a,b)(R);}};
enum class EAlchemyElement{Fire,Wind,Water};
struct FAlchemyVector{int32 Fire=0,Wind=0,Water=0;};
struct FDailyMushroomCardRow{FName MushroomId,CardId;FString DisplayName,Tags;int32 Day=1,PipValue=1,Weight=1;bool bAvailable=false;EAlchemyElement Element=EAlchemyElement::Fire;FAlchemyVector Value;};
struct FDailyRegionalValueRule{FName RuleId,SourceId;EAlchemyElement Element=EAlchemyElement::Fire;int32 MinimumPip=1,MaximumPip=9,SeedSalt=92821;FString Tags;};
enum class EJson{Object,Null};
struct FJsonObject;
struct FJsonValue{EJson Type=EJson::Object;TSharedPtr<FJsonObject> O;TSharedPtr<FJsonObject> AsObject()const{return O;}};
struct FJsonObject{
 std::map<FString,double> Numbers;std::map<FString,FString> Strings;TArray<TSharedPtr<FJsonValue>> Rows;
 bool TryGetNumberField(const TCHAR* k,double& v)const{auto p=Numbers.find(k);if(p==Numbers.end())return false;v=p->second;return true;}
 bool TryGetStringField(const TCHAR* k,FString& v)const{auto p=Strings.find(k);if(p==Strings.end())return false;v=p->second;return true;}
 bool TryGetArrayField(const TCHAR* k,const TArray<TSharedPtr<FJsonValue>>*& v)const{if(std::wstring(k)!=TEXT("Rules"))return false;v=&Rows;return true;}
};
TSharedPtr<FJsonObject> Fixture;bool FilePresent=true,ParseOK=true;
struct FPaths{static FString ProjectConfigDir(){return TEXT("Config");}};
struct FFileHelper{static bool LoadFileToString(FString&,const TCHAR*){return FilePresent;}};
template<class T=void>struct TJsonReaderFactory{static int Create(const FString&){return 0;}};
struct FJsonSerializer{static bool Deserialize(int,TSharedPtr<FJsonObject>& R){R=Fixture;return ParseOK;}};
class UDailyMushroomSubsystem{public:int32 CurrentDay=1;TArray<FDailyRegionalValueRule> RegionalValueRules;bool LoadRegionalValueRules();bool GetDailyRegionalResource(const FString&,EAlchemyElement,FDailyMushroomCardRow&)const;};
struct FForestMagicItem{FString Id,InventoryKey,Name,Description,SourceMushroom;int32 PipValue=1,HarvestDay=1;EAlchemyElement Element=EAlchemyElement::Fire;FAlchemyVector Value;bool bGeneratedByAI=false,bNamingPending=true;};
class UForestHarvestMagic{public:static FString Prompt(const FForestMagicItem&);static FString ElementText(EAlchemyElement e){return e==EAlchemyElement::Fire?TEXT("火"):e==EAlchemyElement::Wind?TEXT("风"):TEXT("水");}};
std::wstring_convert<std::codecvt_utf8<wchar_t>> CV;
std::string q(const std::wstring& w){std::string out="\"";for(char c:CV.to_bytes(w)){if(c=='"'||c=='\\')out+='\\';if(c=='\n')out+="\\n";else if(c=='\r')out+="\\r";else out+=c;}return out+"\"";}
'''

class NativeHarvest:
 def __init__(self):
  self.temp=tempfile.TemporaryDirectory(prefix='forest-harvest-native-');p=Path(self.temp.name);self.exe=p/'bridge'
  for f in ['CoreMinimal.h','Misc/Crc.h','Math/RandomStream.h']:
   (p/f).parent.mkdir(parents=True,exist_ok=True);(p/f).write_text('#pragma once\n')
  daily=(BASE/'Private/DailyMushroomSubsystem.cpp').read_text();harvest=(ROOT/'Source/demo3_UE58/ForestHarvestMagic.cpp').read_text()
  source=SHIM+'\n#include "'+str(BASE/'Public/DailyResourceValuePolicy.h')+'"\n#include "'+str(ROOT/'Source/demo3_UE58/ForestItemNarrativePolicy.h')+'"\n'
  source+='\n'.join([method(daily,'UDailyMushroomSubsystem::LoadRegionalValueRules'),method(daily,'UDailyMushroomSubsystem::GetDailyRegionalResource'),method(harvest,'UForestHarvestMagic::Prompt')])
  data=json.loads((ROOT/'Config/ForestDailyResourceValues.json').read_text())
  source+='\nTSharedPtr<FJsonObject> ValidFixture(){auto Root=std::make_shared<FJsonObject>();Root->Numbers[TEXT("SchemaVersion")]=1;\n'
  for r in data['Rules']:
   source+='{auto O=std::make_shared<FJsonObject>();'
   for k,v in r.items():source+='O->'+('Numbers' if isinstance(v,int) else 'Strings')+'[TEXT('+json.dumps(k)+')]='+ (str(v) if isinstance(v,int) else 'TEXT('+json.dumps(v,ensure_ascii=False)+')')+';'
   source+='auto V=std::make_shared<FJsonValue>();V->O=O;Root->Rows.Add(V);}\n'
  source+='return Root;}\n'
  source+=r'''
void DailyTests(){
 UDailyMushroomSubsystem D;Fixture=ValidFixture();assert(D.LoadRegionalValueRules());
 int Different=0;std::map<int,int> Seen;
 for(int e=0;e<3;++e)for(int n=0;n<96;++n){
  FString Id=FString::Printf(TEXT("region_%d:%d"),e,n);FDailyMushroomCardRow A,B,C;
  D.CurrentDay=1;assert(D.GetDailyRegionalResource(Id,static_cast<EAlchemyElement>(e),A));assert(D.GetDailyRegionalResource(Id,static_cast<EAlchemyElement>(e),B));
  assert(A.PipValue==B.PipValue&&A.CardId==B.CardId&&A.bAvailable&&A.PipValue>=1&&A.PipValue<=9);Seen[A.PipValue]++;
  assert(A.Element==static_cast<EAlchemyElement>(e)&&A.Value.Fire==(e==0?A.PipValue:0)&&A.Value.Wind==(e==1?A.PipValue:0)&&A.Value.Water==(e==2?A.PipValue:0));
  UDailyMushroomSubsystem Reload;assert(Reload.LoadRegionalValueRules());assert(Reload.GetDailyRegionalResource(Id,static_cast<EAlchemyElement>(e),C));assert(A.PipValue==C.PipValue);
  D.CurrentDay=2;assert(D.GetDailyRegionalResource(Id,static_cast<EAlchemyElement>(e),C));Different+=A.PipValue!=C.PipValue;assert(C.Day==2&&C.CardId!=A.CardId);
 }
 assert(Seen.size()==9&&Different>144);
 for(int bad=0;bad<15;++bad){Fixture=ValidFixture();FilePresent=ParseOK=true;auto O=Fixture->Rows[0]->O;
  if(bad==0)FilePresent=false;else if(bad==1)ParseOK=false;else if(bad==2)Fixture->Numbers[TEXT("SchemaVersion")]=2;
  else if(bad==3)Fixture->Rows.pop_back();else if(bad==4)Fixture->Rows[1]=Fixture->Rows[0];else if(bad==5)O->Strings[TEXT("SourceId")]=TEXT("unknown");
  else if(bad==6)O->Strings[TEXT("RuleId")]=TEXT("");else if(bad==7)O->Numbers[TEXT("MinimumPip")]=0;else if(bad==8)O->Numbers[TEXT("MaximumPip")]=10;
  else if(bad==9){O->Numbers[TEXT("MinimumPip")]=8;O->Numbers[TEXT("MaximumPip")]=2;}
  else if(bad==10)O->Numbers[TEXT("SeedSalt")]=1.5;else if(bad==11)O->Numbers[TEXT("SeedSalt")]=2147483648.;
  else if(bad==12)Fixture->Rows[0]->Type=EJson::Null;else if(bad==13)O->Strings[TEXT("Element")]=TEXT("earth");else O->Numbers.erase(TEXT("MaximumPip"));
  assert(!D.LoadRegionalValueRules()&&D.RegionalValueRules.empty());FDailyMushroomCardRow R;assert(!D.GetDailyRegionalResource(TEXT("valid"),EAlchemyElement::Fire,R)&&!R.bAvailable);
 }
 FilePresent=ParseOK=true;Fixture=ValidFixture();assert(D.LoadRegionalValueRules());FDailyMushroomCardRow R;assert(!D.GetDailyRegionalResource(TEXT(""),EAlchemyElement::Fire,R));assert(!D.GetDailyRegionalResource(TEXT("r"),static_cast<EAlchemyElement>(8),R));
 std::cout<<"{\"passed\":true,\"daily_identity_samples\":288,\"invalid_configs\":15,\"all_pip_values_seen\":9}";
}
void ApplyTests(){
 FForestMagicItem Original;Original.Id=TEXT("stable-id");Original.InventoryKey=TEXT("stable-key");Original.Name=TEXT("原来的小纽扣");Original.Description=TEXT("原有描述");Original.PipValue=7;Original.HarvestDay=2;Original.Element=EAlchemyElement::Water;Original.Value.Water=7;
 const FString Name=TEXT("藏雨玻璃纽扣"),Description=TEXT("透明的纽扣里悬着一朵小云，贴近掌心时细雨便沿着玻璃内侧缓缓滑落，却从不沾湿衣袖。");
 assert(ForestItemNarrativePolicy::Valid(Name,Description));
 assert(!ForestItemNarrativePolicy::Valid(TEXT("火5纽扣"),Description));assert(!ForestItemNarrativePolicy::Valid(Name,TEXT("太短")));assert(!ForestItemNarrativePolicy::Valid(TEXT("ABCD"),Description));
 assert(!ForestItemNarrativePolicy::Valid(Name,Description+TEXT("\n")));
 for(bool Valid:{false,true})for(bool Owned:{false,true})for(bool Saved:{false,true}){
  auto Row=Original;int Calls=0;assert(ForestItemNarrativePolicy::Apply(Row,Name,Description,Valid,Owned,[&](){++Calls;return Saved;})==Saved);assert(Calls==1&&!Row.bNamingPending);
  const bool Changed=Valid&&Owned&&Saved;assert(Row.bGeneratedByAI==Changed);assert(Row.Name==(Changed?Name:Original.Name));assert(Row.Description==(Changed?Description:Original.Description));
  assert(Row.Id==Original.Id&&Row.InventoryKey==Original.InventoryKey&&Row.PipValue==7&&Row.HarvestDay==2&&Row.Element==Original.Element&&Row.Value.Water==7&&Row.Value.Fire==0&&Row.Value.Wind==0);
 }
 std::cout<<"{\"passed\":true,\"metadata_save_cases\":8,\"immutable_attributes\":true}";
}
int main(int argc,char** argv){std::string Mode=argc>1?argv[1]:"daily";if(Mode=="daily")DailyTests();else if(Mode=="apply")ApplyTests();else if(Mode=="valid")std::cout<<(ForestItemNarrativePolicy::Valid(CV.from_bytes(argv[2]),CV.from_bytes(argv[3]))?"true":"false");else if(Mode=="prompt"){FForestMagicItem I;I.Element=static_cast<EAlchemyElement>(std::stoi(argv[2]));I.PipValue=std::stoi(argv[3]);I.SourceMushroom=I.Element==EAlchemyElement::Water?TEXT("Region_Water_Shell"):TEXT("Region_Fire_Mushroom");std::cout<<q(UForestHarvestMagic::Prompt(I));}}
'''
  (p/'bridge.cpp').write_text(source)
  subprocess.run(['c++','-std=c++17','-Wno-deprecated-declarations','-I',str(p),str(p/'bridge.cpp'),'-o',str(self.exe)],check=True)
 def call(self,*args):return json.loads(subprocess.check_output([str(self.exe),*map(str,args)]))
 def close(self):self.temp.cleanup()

if __name__=='__main__':
 n=NativeHarvest()
 try:
  print(n.call('daily'));print(n.call('apply'))
 finally:n.close()
