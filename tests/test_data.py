"""Validate shipped provenance, root IDs, edge weights and cartridge header."""
import csv, hashlib, json, re
from pathlib import Path
meta=json.loads(Path('docs/connectome.json').read_text())
s=Path('src/connectome.c').read_text()
ids=list(map(int,re.findall(r' (\d+)ULL,',s)))
assert len(ids)==len(set(ids))==meta['neurons']==128
assert set(meta['sugar_seeds']+meta['motor_seeds'])<=set(ids)
edges=[tuple(map(int,e)) for e in re.findall(r'\{(\d+),(\d+),(-?\d+)\}',s)]
rows=list(csv.DictReader(open('docs/connectome_edges.csv')))
assert len(edges)==len(rows)==meta['edges']==2048
for e,r in zip(edges,rows):
 assert ids[e[0]]==int(r['pre_root_id']) and ids[e[1]]==int(r['post_root_id'])
 assert e[2]==int(r['model_weight'])
 raw=int(r['signed_synapses'])
 assert raw and e[2]*raw>0 and abs(e[2])<=96
rom=Path('build/fly.gba').read_bytes()
assert rom[178]==0x96 and (sum(rom[160:190])+0x19)%256==0
assert len(rom)<=32*1024*1024 and rom[3]==0xea
print(f'Provenance and ROM header passed; SHA256 {hashlib.sha256(rom).hexdigest()}')
