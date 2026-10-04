#!/usr/bin/env python3
"""Extract a reproducible 128-node sugar/MN9 subgraph from Shiu FlyWire v783.
Requires numpy and pyarrow. Never executes the notebook. No fabricated edges.
"""
import argparse, ast, csv, hashlib, json, re
from pathlib import Path
import numpy as np
import pyarrow.parquet as pq

p=argparse.ArgumentParser()
p.add_argument('parquet', type=Path)
p.add_argument('notebook', type=Path)
p.add_argument('--output', type=Path, default=Path('src/connectome.c'))
a=p.parse_args()
source='\n'.join(''.join(c.get('source', [])) for c in json.loads(a.notebook.read_text())['cells'])
sugar=ast.literal_eval(re.search(r'neu_sugar\s*=\s*(\[[\s\S]*?\])',source).group(1))
motor=[720575940660219265,720575940645521262]
t=pq.read_table(a.parquet,columns=['Presynaptic_ID','Postsynaptic_ID','Presynaptic_Index','Postsynaptic_Index','Excitatory x Connectivity'])
pre=t['Presynaptic_Index'].combine_chunks().to_numpy()
post=t['Postsynaptic_Index'].combine_chunks().to_numpy()
weight=t['Excitatory x Connectivity'].combine_chunks().to_numpy()
n=int(max(pre.max(),post.max()))+1
roots=np.zeros(n,dtype=np.int64)
roots[pre]=t['Presynaptic_ID'].combine_chunks().to_numpy()
roots[post]=t['Postsynaptic_ID'].combine_chunks().to_numpy()
lookup={int(r):i for i,r in enumerate(roots)}
sugar=[r for r in sugar if r in lookup];motor=[r for r in motor if r in lookup]
if len(motor)<1 or len(sugar)<10:raise ValueError('Missing annotated seeds in this dataset')
si=np.array([lookup[r] for r in sugar]);mi=np.array([lookup[r] for r in motor])
# Rank nodes by three-hop forward/reverse anatomical connectivity. Positive edges
# determine reachability; retain inhibitory edges in the final induced subgraph.
pos=np.maximum(weight,0).astype(float)
out=np.bincount(pre,weights=pos,minlength=n);inc=np.bincount(post,weights=pos,minlength=n)
f=np.zeros(n);b=np.zeros(n);f[si]=1;b[mi]=1
fs=f.copy();bs=b.copy()
for _ in range(3):
 f=np.bincount(post,weights=pos*(f/np.maximum(out,1))[pre],minlength=n)
 b=np.bincount(pre,weights=pos*(b/np.maximum(inc,1))[post],minlength=n)
 fs+=f;bs+=b
score=np.sqrt(fs*bs)+fs*0.0001
seed=list(si)+list(mi)
rank=sorted(range(n),key=lambda i:(-score[i],int(roots[i])))
chosen=seed+[i for i in rank if i not in seed][:128-len(seed)]
local=np.full(n,-1,dtype=np.int32);local[chosen]=np.arange(128)
mask=(local[pre]>=0)&(local[post]>=0)
aggregate={}
for u,v,w in zip(local[pre[mask]],local[post[mask]],weight[mask]):
 if u!=v:aggregate[(int(u),int(v))]=aggregate.get((int(u),int(v)),0)+int(w)
items=sorted(((u,v,w) for (u,v),w in aggregate.items() if w),key=lambda e:(-abs(e[2]),e[0],e[1]))[:2048]
items.sort()
# Quantized, saturating signed weights; dynamics are deliberately not calibrated.
edges=[(u,v,max(-96,min(96,(1 if w>0 else -1)*max(1,round(abs(w)**0.5*7))))) for u,v,w in items]
groups=[0]*len(si)+[2]*len(mi)+[1]*(128-len(si)-len(mi))
a.output.parent.mkdir(parents=True,exist_ok=True)
a.output.write_text('#include "fly.h"\n/* FlyWire v783 real induced subgraph; see docs/connectome.json. */\nconst unsigned edge_count='+str(len(edges))+';\nconst Edge edges[]={\n'+''.join(' {%d,%d,%d},\n'%e for e in edges)+'};\nconst uint64_t root_ids[]={\n'+''.join(' %dULL,\n'%int(roots[i]) for i in chosen)+'};\nconst uint8_t neuron_group[]={'+','.join(map(str,groups))+'};\n')
a.output.write_text(a.output.read_text()+'const char root_labels[NEURONS][19]={\n'+''.join(' \"'+str(int(roots[i]))+'\",\n' for i in chosen)+'};\n')
Path('docs').mkdir(exist_ok=True)
with open('docs/connectome_edges.csv','w') as out_file:
 w=csv.writer(out_file);w.writerow(['pre_root_id','post_root_id','signed_synapses','model_weight'])
 for original,scaled in zip(items,edges):w.writerow([int(roots[chosen[original[0]]]),int(roots[chosen[original[1]]]),original[2],scaled[2]])
meta={'source':'https://github.com/philshiu/Drosophila_brain_model','dataset':'FlyWire FAFB v783','source_sha256':hashlib.sha256(a.parquet.read_bytes()).hexdigest(),'notebook_sha256':hashlib.sha256(a.notebook.read_bytes()).hexdigest(),'missing_notebook_seeds':[r for r in [720575940620900446,720575940645521262] if r not in lookup],'neurons':128,'edges':len(edges),'sugar_seeds':sugar,'motor_seeds':motor,'selection':'3-hop forward/reverse anatomical relevance; strongest 2048 induced edges','weight_transform':'sign(w)*round(sqrt(abs(w))*7), clipped to +/-96','model':'Illustrative integer LIF, 30 Hz; not the calibrated Shiu model','body':'Articulated flybody-derived graphics and heuristic locomotion; MN9 gates feeding','node_positions':'Synthetic graph layout, not anatomical coordinates','license':'FlyWire data CC BY-NC 4.0; Shiu code MIT'}
Path('docs/connectome.json').write_text(json.dumps(meta,indent=2)+'\n')
print(json.dumps({'neurons':128,'edges':len(edges),'sugar_seeds':len(sugar),'motor_seeds':len(motor)}))
