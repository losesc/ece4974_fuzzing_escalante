#!/usr/bin/env python3
"""Assemble a Lab 1 corpus for json-c:
   - the project's own tests/*.json
   - JSON test strings harvested from tests/test_parse.c
   - a handful of hand-made edge cases
Run from inside the json-c source tree. Writes ./corpus/.
"""
import re, os
os.makedirs('corpus', exist_ok=True)

# 1) project's shipped corpus files
for f in os.listdir('tests'):
    if f.endswith('.json'):
        open(f'corpus/{f}','w').write(open(f'tests/{f}').read())

# 2) harvest the { "json string", flag } table from test_parse.c
src = open('tests/test_parse.c').read()
lits = re.findall(r'\{\s*"((?:[^"\\]|\\.)*)"\s*,', src)
seen=set(); i=0
for lit in lits:
    try: s = bytes(lit,'utf-8').decode('unicode_escape')
    except Exception: s = lit
    if s in seen: continue
    seen.add(s); i+=1
    open(f'corpus/tp_{i:02d}.json','w').write(s)
print(f"harvested {i} parser test strings")

# 3) edge cases you invent (document these in the report)
edge = {
 "edge_deep.json":     '['*3000 + ']'*3000,
 "edge_bignum.json":   '{"n": 123456789012345678901234567890123456789}',
 "edge_intmin.json":   '[-9223372036854775809]',
 "edge_expo.json":     '[1e999999999]',
 "edge_trunc.json":    '{"a": [1,2,',
 "edge_nul.json":      '{"a":"b\x00c"}',
 "edge_dupkeys.json":  '{'+','.join('"k":%d'%i for i in range(2000))+'}',
 "edge_badesc.json":   '"\\u',
 "edge_longstr.json":  '"'+'A'*200000+'"',
 "edge_emptykey.json": '{"":{"":{"":1}}}',
 "edge_neg0.json":     '[-0, -0.0, 0e-0]',
}
for n,c in edge.items():
    open('corpus/'+n,'wb').write(c.encode('latin1',errors='replace'))
print(f"wrote {len(edge)} edge cases")
print(f"corpus total: {len(os.listdir('corpus'))} files")
