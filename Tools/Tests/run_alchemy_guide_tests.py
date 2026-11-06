#!/usr/bin/env python3
"""Compile the production operation bodies with a small STL shim; never launch UE.

Checks the 22 documented examples and operation-order/rounding/lock/seal cases.
This is an arithmetic regression, not a Slate, reflection, save or PIE test.
"""
import json
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'Plugins/DialogueMemoryTable/Source/DialogueMemoryTable/Private/AlchemyGameplaySubsystem.cpp'
text = SOURCE.read_text()
guide = json.loads((ROOT / 'Config/ForestMajorGuide.json').read_text())


def method(name):
    match = re.search(r'(?:int32|void) UAlchemyGameplaySubsystem::' + name + r'\(', text)
    assert match, name
    start = text.index('{', match.start())
    level = 1
    end = start + 1
    while level:
        level += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end]


definitions = {}
major_text = text.split('UAlchemyGameplaySubsystem::MakeMajorCards()', 1)[1].split('UAlchemyGameplaySubsystem::MakeCourtCards()', 1)[0]
for line in major_text.splitlines():
    match = re.search(r'MakeCard\(TEXT\("([^"]+)"\).*?EAlchemyCardType::(\w+), (\d+),', line)
    if match:
        card_id, kind, cost = match.groups()
        bracket = re.search(r'EAlchemyElement::Fire, 0, (\d+)\)', line)
        definitions[card_id] = (kind, int(cost), int(bracket.group(1)) if bracket else 0)

assert len(definitions) == 22
assert len(guide['entries']) == 22
assert {r['id'] for r in guide['entries']} == set(definitions)
assert len({r['id'] for r in guide['entries']}) == 22
for row in guide['entries']:
    assert all(row[key].strip() for key in ['howToUse', 'reverse', 'caution'])

expected = {
    '0': (12, 0, 0), 'I': (7, 0, 2), 'II': (0, 1, 3), 'III': (4, 3, 0),
    'IV': (7, 0, 0), 'V': (3, 1, 0), 'VI': (1, 3, 0), 'VII': (6, 1, 1),
    'VIII': (3, 2, 0), 'IX': (5, 0, 0), 'X': (0, 3, 2), 'XI': (9, 1, 0),
    'XII': (-3, -1, 0), 'XIII': (5, 3, 0), 'XIV': (3, 0, 0), 'XV': (3, 0, 0),
    'XVI': (2, 2, 0), 'XVII': (0, 6, 0), 'XVIII': (0, 0, 7),
    'XIX': (6, 2, 0), 'XX': (1, 0, 3), 'XXI': (6, 2, 0),
}
expected_cost = {key: 1 for key in expected}
expected_cost.update({'0': 2, 'I': 2, 'VII': 3, 'XI': 2, 'XIV': 2, 'XIX': 2, 'XXI': 4})


def sequence(tokens):
    out = []
    for index, token in enumerate(tokens):
        if ':' in token:
            element, value = token.split(':')
            assert element in ['fire', 'wind', 'water'] and 1 <= int(value) <= 9
            out.append('{"p%d",EAlchemyCardType::Pip,EAlchemyElement::%s,%d,0,0}' % (index, element.title(), int(value)))
        else:
            kind, cost, bracket = definitions[token]
            out.append('{"%s",EAlchemyCardType::%s,EAlchemyElement::Fire,0,%d,%d}' % (token, kind, cost, bracket))
    return '{' + ','.join(out) + '}'


cases = [(row['id'], row['example'], expected[row['id']]) for row in guide['entries']]
cases += [
    ('sum', ['fire:3', 'fire:3'], (6, 0, 0)),
    ('order', ['fire:3', 'fire:1', 'IV'], (8, 0, 0)),
    ('empty', [], (0, 0, 0)),
    ('no mimic source', ['fire:3', '0'], (3, 0, 0)),
    ('reverse half', ['fire:5', 'XV', 'IV'], (2, 0, 0)),
    ('negative truncate', ['fire:5', 'XII', 'XVI'], (-2, 0, 0)),
    ('reverse discard', ['fire:5', 'wind:1', 'XV', 'XIII'], (5, 2, 0)),
    ('reverse subpot', ['fire:5', 'XV', 'I', 'fire:2', 'wind:1'], (3, -1, 0)),
    ('incomplete subpot', ['fire:1', 'I', 'water:2'], (1, 0, 2)),
    ('sun in subpot', ['fire:1', 'I', 'wind:2', 'XIX', 'fire:2'], (3, 4, 0)),
    ('single balance', ['fire:3', 'VIII'], (0, 0, 0)),
    ('main tie', ['fire:2', 'wind:2', 'IX'], (2, 0, 0)),
    ('double prefix', ['fire:5', 'XV', 'XV', 'fire:2'], (3, 0, 0)),
]

shim = r'''
#include <string>
#include <vector>
#include <set>
#include <cmath>
#include <limits>
#include <iostream>
using int32=int;
#define TEXT(x) x
#define INDEX_NONE -1
#define MAX_int32 std::numeric_limits<int>::max()
struct FName:std::string {using std::string::string;bool IsNone()const{return empty();}};
const FName NAME_None;
template<class T>using TArray=std::vector<T>;
template<class T>struct TSet:std::set<T>{void Add(const T& x){this->insert(x);}};
struct FMath {template<class T>static T Abs(T x){return std::abs(x);} template<class T>static T Max(T a,T b){return a>b?a:b;} static int TruncToInt(float x){return static_cast<int>(x);}};
enum class EAlchemyElement{Fire,Wind,Water};
enum class EAlchemyCardType{Pip,Transform,Bracket,Prefix,Court,Seal};
struct FAlchemyVector {int Fire=0,Wind=0,Water=0;};
struct FAlchemyCardSequenceItem {FName CardId;EAlchemyCardType Type;EAlchemyElement Element;int PipValue,Cost,BracketLength;};
class UAlchemyGameplaySubsystem {
public:
 static int32 GetAxis(const FAlchemyVector&,EAlchemyElement);
 static void SetAxis(FAlchemyVector&,EAlchemyElement,int32);
 static int32 AbsMaxAxis(const FAlchemyVector&);
 static int32 AbsMinNonZeroAxis(const FAlchemyVector&);
 static void AddVectors(FAlchemyVector&,const FAlchemyVector&,int32);
 static void ComputeAlchemySequence(const TArray<FAlchemyCardSequenceItem>&,FAlchemyVector&,int32&,TSet<FName>&);
};
'''
knowledge=json.loads((ROOT/'Config/ForestCommissionKnowledge.json').read_text())
for entry in knowledge['entries']:
    tokens=[element+':'+str(value) for element,value in zip(['fire','wind','water'],entry['target']) if value]
    assert len(tokens)<=({1:3,2:4,3:4,4:4,5:3,6:4,7:4,8:5,9:6}[entry['commission_id']])
    cases.append(('reviewed commission '+str(entry['commission_id']),tokens,tuple(entry['target'])))
cpp = shim + '\n'.join(method(name) for name in ['GetAxis', 'SetAxis', 'AbsMaxAxis', 'AbsMinNonZeroAxis', 'AddVectors', 'ComputeAlchemySequence'])
cpp += '\nint main(){\n'
for label, tokens, actual in cases:
    cpp += '{FAlchemyVector v;int cost=0;TSet<FName> ids;UAlchemyGameplaySubsystem::ComputeAlchemySequence(%s,v,cost,ids);if(v.Fire!=%d || v.Wind!=%d || v.Water!=%d){std::cerr<<"%s failed: "<<v.Fire<<","<<v.Wind<<","<<v.Water;return 1;}}\n' % (sequence(tokens), *actual, label)
    if label in expected_cost:
        cpp += '{FAlchemyVector v;int cost=0;TSet<FName> ids;UAlchemyGameplaySubsystem::ComputeAlchemySequence(%s,v,cost,ids);if(cost!=%d){std::cerr<<"%s cost failed";return 1;}}\n' % (sequence(tokens), expected_cost[label], label)
cpp += 'std::cout<<"PASS: %d production arithmetic cases; 22 guide definitions/examples validated\\n";}\n' % len(cases)
with tempfile.TemporaryDirectory(prefix='forest-alchemy-guide-') as temp:
    source = Path(temp) / 'test.cpp'
    binary = Path(temp) / 'test'
    source.write_text(cpp)
    subprocess.run(['c++', '-std=c++17', '-O0', str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
