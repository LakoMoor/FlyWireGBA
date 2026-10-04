#!/usr/bin/env python3
"""Reduce pinned flybody OBJ/MJCF assets; optional authoring dependencies only.

pip install mujoco==3.14.0 trimesh==5.1.1 fast-simplification==0.2.0 numpy
Usage: python tools/import_body.py /path/flybody/flybody/fruitfly/assets
The ROM uses generated sprites and does not run MuJoCo.
"""
import argparse, hashlib, json
from pathlib import Path
import mujoco
import numpy as np
import trimesh

p=argparse.ArgumentParser();p.add_argument('source',type=Path);a=p.parse_args()
root=Path(__file__).resolve().parents[1];dest=root/'assets';dest.mkdir(exist_ok=True)
m=mujoco.MjModel.from_xml_path(str(a.source/'fruitfly.xml'))
def rotation(q):
 out=np.zeros(9);mujoco.mju_quat2Mat(out,q);return out.reshape(3,3)
vertices=[];triangles=[];vertex_body=[];face_material=[];materials=[]
for g in range(m.ngeom):
 if m.geom_type[g]!=mujoco.mjtGeom.mjGEOM_MESH or m.geom_group[g]!=1:continue
 name=m.geom(g).name;mesh=int(m.geom_dataid[g]);v0=int(m.mesh_vertadr[mesh]);f0=int(m.mesh_faceadr[mesh])
 v=m.mesh_vert[v0:v0+m.mesh_vertnum[mesh]].astype(np.float64)
 f=m.mesh_face[f0:f0+m.mesh_facenum[mesh]].copy()
 budget=3000 if name=='thorax' else 2400 if name=='head_red' else 1400 if name in ('head','thorax_black','head_black') else 700 if 'abdomen' in name else 900 if 'wing' in name else 180
 # MuJoCo splits vertices along normal seams. Weld those before decimating;
 # otherwise the simplifier removes disconnected triangles and leaves holes.
 t=trimesh.Trimesh(v,f,process=True)
 t.merge_vertices(digits_vertex=6)
 if len(f)>budget:t=t.simplify_quadric_decimation(face_count=budget)
 v=np.asarray(t.vertices)@rotation(m.geom_quat[g]).T+m.geom_pos[g]
 f=np.asarray(t.faces)+sum(len(x) for x in vertices)
 mat=m.geom_matid[g];rgba=m.mat_rgba[mat].copy() if mat>=0 else m.geom_rgba[g].copy()
 materials.append({'name':name,'rgba':rgba.tolist(),'body':int(m.geom_bodyid[g]),'faces':len(f)})
 vertices.append(v);triangles.append(f);vertex_body.extend([m.geom_bodyid[g]]*len(v));face_material.extend([len(materials)-1]*len(f))
 print(name,len(f),flush=True)
bodies=[]
for b in range(m.nbody):
 joints=[]
 for j in range(m.njnt):
  if m.jnt_bodyid[j]!=b or m.jnt_type[j]!=mujoco.mjtJoint.mjJNT_HINGE:continue
  joints.append({'name':m.joint(j).name,'axis':m.jnt_axis[j].tolist(),'pos':m.jnt_pos[j].tolist(),'rest':float(m.qpos_spring[m.jnt_qposadr[j]]),'range':m.jnt_range[j].tolist()})
 bodies.append({'name':m.body(b).name,'parent':int(m.body_parentid[b]),'pos':m.body_pos[b].tolist(),'rotation':rotation(m.body_quat[b]).tolist(),'joints':joints})
np.savez_compressed(dest/'flybody.npz',vertices=np.concatenate(vertices).astype(np.float32),triangles=np.concatenate(triangles).astype(np.int32),vertex_body=np.array(vertex_body,dtype=np.int16),face_material=np.array(face_material,dtype=np.int16))
meta={'source':'https://github.com/TuragaLab/flybody','commit':'d015e9bfe441bd90ae431bac24c55cb74bdbce26','license':'Apache-2.0','xml_sha256':hashlib.sha256((a.source/'fruitfly.xml').read_bytes()).hexdigest(),'modified':'Per-material quadric mesh simplification; offline lighting, illustrative joint animation and palette quantization. Original body transforms and joint anchors retained.','bodies':bodies,'materials':materials}
(dest/'flybody.json').write_text(json.dumps(meta,separators=(',',':'))+'\n')
print('Imported',sum(len(x) for x in triangles),'triangles')
