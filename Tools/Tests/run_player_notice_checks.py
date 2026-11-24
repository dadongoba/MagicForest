"""Execute the actual player-notice formatter with a minimal string adapter, no UE launch."""
from pathlib import Path
import tempfile,subprocess
ROOT=Path(__file__).resolve().parents[2]
def main():
 src=(ROOT/'Source/demo3_UE58/ForestWorldOutcome.cpp').read_text()
 fn=src[src.index('FString Summary('):src.index('\nFString Memory(')]
 code=r'''
#include <string>
#include <cwchar>
#include <sstream>
#include <iomanip>
#include <vector>
#include <iostream>
#include <cassert>
#define TEXT(x) L##x
struct FString:std::wstring{using std::wstring::wstring;FString(const std::wstring& v):std::wstring(v){} FString(){} const wchar_t* operator*()const{return c_str();}
 template<class A>static std::wstring Arg(A a){std::wostringstream o;o<<std::fixed<<std::setprecision(0)<<a;return o.str();}
 template<class... A> static FString Printf(std::wstring f,A...a){std::vector<std::wstring> args={Arg(a)...};FString out;size_t n=0;for(size_t i=0;i<f.size();){if(f.compare(i,2,L"%%")==0){out+=L"%";i+=2;}else if(f.compare(i,2,L"%s")==0){out+=args.at(n++);i+=2;}else if(f.compare(i,4,L"%.0f")==0){out+=args.at(n++);i+=4;}else out+=f[i++];}return out;}};
struct FForestWorldOutcome{int Revision=1,PreviousState=0,NextState=3;bool bDisconnected=true;FString Region=L"fire",Persona=L"blacksmith";float AvailabilityA=.4,AvailabilityB=.85;};
FString StateName(int s){return s==0?L"正常":L"严重异常";}
'''+fn+r'''
int main(){FForestWorldOutcome o;
 for(auto role:{L"blacksmith",L"operator",L"novelist"}){o.Persona=role;auto text=Summary(o,false);assert(text.find(L"你搞砸了这次托付")!=text.npos);assert(text.find(L"不会再向你提出新的委托")!=text.npos);assert(text.find(L"40%")!=text.npos);assert(text.find(L"85%")!=text.npos);assert(text.find(L"她伸手")==text.npos);}
 o.bDisconnected=false;assert(Summary(o,false).find(L"不会再") == std::wstring::npos);
 o.bDisconnected=true;assert(Summary(o,true).find(L"不生成第四天")!=std::wstring::npos);
 o.Revision=0;assert(Summary(o,false).empty());
 std::cout<<"PASS actual formatter: three characters, player-facing failure, surviving interactions, percentages, nonterminal and final-night branches\n";
}'''
 with tempfile.TemporaryDirectory() as t:
  cpp=Path(t)/'notice.cpp';out=Path(t)/'notice';cpp.write_text(code)
  subprocess.run(['clang++','-std=c++17','-fsanitize=address,undefined',str(cpp),'-o',str(out)],check=True)
  subprocess.run([str(out)],check=True)
if __name__=='__main__':main()
