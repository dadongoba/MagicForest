"""Portable bridge to production persona/trigger/prompt/text functions, not a UE host.

UE file loading and JSON reflection are supplied by the harness; native factories,
prompt construction, familiarity triggers and reply guards are compiled from source.
"""
import ast,json,re,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
BASE=ROOT/'Plugins/DialogueMemoryTable/Source/DialogueMemoryTable'

def extract(source,name):
 m=re.search(r'(?:FName|bool|FString|int32) '+re.escape(name)+r'\(',source);assert m,name
 a=source.index('{',m.start());b=a+1;level=1
 while level:level+=(source[b]=='{')-(source[b]=='}');b+=1
 return source[m.start():b]

def literal(source,name):
 tree=ast.parse(source)
 return next(n.value.value for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id==name for t in n.targets))

class NativePolicy:
 def __init__(self):
  self.tmp=tempfile.TemporaryDirectory(prefix='forest-dialogue-bridge-');p=Path(self.tmp.name);self.exe=p/'bridge'
  gameplay=(BASE/'Private/AlchemyGameplaySubsystem.cpp').read_text()
  persona=(BASE/'Private/AlchemyPersonaPolicy.cpp').read_text()
  knowledge=(BASE/'Private/AlchemyDialogueKnowledge.cpp').read_text()
  shim=literal((ROOT/'Tools/Tests/run_persona_policy_tests.py').read_text(),'shim')
  shim=shim.replace('using int32=int;','using int32=int;using TCHAR=wchar_t;')
  shim='#include <sstream>\n'+shim
  shim=shim.replace('template<class... A>static FString Printf(const wchar_t*,A...){return TEXT("matched");}',r'''
 template<class A>static std::wstring FormatArg(A a){std::wostringstream out;out<<a;return out.str();}
 template<class... A>static FString Printf(const wchar_t* fmt,A... a){
  std::vector<std::wstring> args={FormatArg(a)...};FString out;size_t next=0;
  for(size_t i=0;fmt[i];++i){
   if(fmt[i]==L'%' && (fmt[i+1]==L's'||fmt[i+1]==L'd')){assert(next<args.size());out+=args[next++];++i;}
   else if(fmt[i]==L'%' && fmt[i+1]==L'%'){out+=L'%';++i;}
   else out+=fmt[i];
  }
  assert(next==args.size());return out;
 }''')
  shim=shim.replace(' bool IsEmpty()const{return empty();}',r'''
 static FString Chr(wchar_t c){return FString(1,c);}
 void ReplaceInline(const wchar_t* a,const wchar_t* b){size_t p=0;while((p=find(a,p))!=npos){replace(p,std::wstring(a).size(),b);p+=std::wstring(b).size();}}
 bool StartsWith(const wchar_t* s)const{return find(s)==0;}
 int Len()const{return size();}
 FString TrimStartAndEnd()const{FString s=*this;s.TrimStartAndEndInline();return s;}
 bool IsEmpty()const{return empty();}''')
  shim=shim.replace('P(p){}','P(std::wstring(p).substr(std::wstring(p).find(L"(?i)")==0?4:0),std::regex::icase){}')
  shim=shim.replace('struct UAlchemyGameplaySubsystem{','struct FAlchemyCommissionDefinition{int OrderIndex=0;FString Wish;};\nstruct UAlchemyGameplaySubsystem{\n bool GetCommission(int,FAlchemyCommissionDefinition&)const;\n FString BuildPersonaSystemPrompt(FName,int,int,const FString&,const TArray<struct FDialogueMessage>&)const;')
  config=json.loads((ROOT/'Config/ForestCommissionKnowledge.json').read_text())
  definitions='bool UAlchemyGameplaySubsystem::GetCommission(int id,FAlchemyCommissionDefinition& c)const{c.OrderIndex=id;switch(id){'+''.join('case %d:c.Wish=TEXT(%s);return true;'%(e['commission_id'],json.dumps(e['purpose'])) for e in config['entries'])+'default:return false;}}\n'
  knowledge_stub='namespace AlchemyDialogueKnowledge {\n'+extract(knowledge,'Rules')+r'''
 FString Intro(const FAlchemyCommissionDefinition& c){return c.Wish;}
 FString Guidance(const FAlchemyCommissionDefinition&,int,int){return TEXT("__GUIDANCE__");}
 FString SafeMemoryJson(const FString& s){return s;}
'''+ '\n'.join(extract(knowledge,n) for n in ['HasNumericDisclosure','IsSafeReply','IsSafeReplyForPlayer','TopicBit','LimitTopics'])+'\n}\n'
  helper='namespace AlchemyGameplay {\n'+'\n'.join(extract(gameplay,n) for n in ['FindKeyword','MatchesLayerTrigger','LayerPrompt'])+'\n}\n'
  methods='\n'.join(extract(gameplay,'UAlchemyGameplaySubsystem::'+n) for n in ['GetPersonaProfile','EvaluatePersonaTrigger','BuildPersonaSystemPrompt'])
  extra=r'''
#include <codecvt>
#include <locale>
std::wstring_convert<std::codecvt_utf8<wchar_t>> cv;
std::string q(const std::wstring& s){std::string v=cv.to_bytes(s),out="\"";for(char ch:v){if(ch=='"'||ch=='\\')out+='\\';if(ch=='\n')out+="\\n";else if(ch=='\r')out+="\\r";else if(ch=='\t')out+="\\t";else out+=ch;}return out+"\"";}
int main(int argc,char** argv){
 UAlchemyGameplaySubsystem g;std::string mode=argc>1?argv[1]:"profiles";
 auto arg=[&](int n)->FString{return n<argc?cv.from_bytes(argv[n]):std::wstring();};
 if(mode=="guard"){std::cout<<(AlchemyDialogueKnowledge::IsSafeReplyForPlayer(arg(2),arg(3))?"true":"false");return 0;}
 if(mode=="safe"){std::cout<<q(AlchemyDialogueKnowledge::IsSafeReply(arg(2))?arg(2):FString());return 0;}
 if(mode=="clean"){std::cout<<q(AlchemyPersonaPolicy::CleanReply(arg(2),arg(3)));return 0;}
 if(mode=="trigger"){int layer=0;FString why;int result=g.EvaluatePersonaTrigger(arg(2),arg(4),std::stoi(argv[3]),layer,why);std::cout<<result;return 0;}
 if(mode=="topics"){std::cout<<AlchemyDialogueKnowledge::LimitTopics(std::stoi(argv[2]));return 0;}
 if(mode=="prompt"){std::cout<<q(g.BuildPersonaSystemPrompt(arg(2),std::stoi(argv[3]),std::stoi(argv[4]),arg(5),{}));return 0;}
 std::cout<<"{";bool first=true;for(const auto& p:{g.MakeBlacksmithProfile(),g.MakeOperatorProfile(),g.MakeNovelistProfile()}){if(!first)std::cout<<",";first=false;std::cout<<q(p.PersonaId)<<":{\"name\":"<<q(p.Name)<<",\"layers\":[";for(int i=0;i<3;++i){if(i)std::cout<<",";std::cout<<q(p.Layers[i].Prompt);}std::cout<<"]}";}std::cout<<"}";
}
'''
  source=shim+'\nstruct FDialogueMessage{};\n'+(BASE/'Private/AlchemyPersonaProfiles.inl').read_text()+helper+definitions+knowledge_stub+methods+'\nnamespace AlchemyPersonaPolicy{'+extract(persona,'CanonicalId')+extract(persona,'CleanReply')+'}\n'+extra
  (p/'bridge.cpp').write_text(source)
  subprocess.run(['c++','-std=c++17',str(p/'bridge.cpp'),'-o',str(self.exe)],check=True,capture_output=True)
 def call(self,*args):
  return json.loads(subprocess.check_output([str(self.exe),*map(str,args)]))
 def close(self):self.tmp.cleanup()

def guidance(entry,mask,shown,codex):
 names=['purpose','fire','wind','water','bounds','process'];out='\n【本轮按玩家追问开放的需求线索】\n'
 for i,key in enumerate(names):
  if not ((mask|shown)&(1<<i)):continue
  text=entry['purpose'] if i==0 else entry['topics'][key]
  if 1<=i<=3:
   strength=entry['target'][i-1]
   description='不额外给予或取走这种性质。' if strength==0 else next(e['description'] for e in codex['entries'] if e['element']==key and e['strength']==strength)
   text+='\n图鉴中的同一性质依据：'+description
  out+=('可承接已解释的主题：' if shown&(1<<i) else '可新解释的主题：')+text+'\n'
 return out+'只回答上述与本轮提问有关的内容，不抄成目录，不主动谈其他未开放主题。\n'

if __name__=='__main__':
 n=NativePolicy()
 try:
  assert n.call('guard','我要火6','') is False
  assert n.call('trigger','blacksmith',1,'你好')==1
  assert n.call('trigger','blacksmith',1,'安置办')==3
  profiles=n.call('profiles')
  for who,idx in [('blacksmith',1),('operator',4),('novelist',7)]:
   for level in [1,2,3]:
    p=n.call('prompt',who,level,idx,'{}')
    assert profiles[who]['name'] in p and profiles[who]['layers'][level-1] in p
    assert '【基础人设】' in p and '【日常状态】' in p and '火性6' not in p and '__GUIDANCE__' in p
  print('Compiled production prompt/persona/trigger/guard bridge passed')
 finally:n.close()
