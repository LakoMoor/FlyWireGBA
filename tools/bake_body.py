#!/usr/bin/env python3
"""Bake articulated 3D flybody into indexed, transparent GBA sprites.

Optional authoring dependencies: numpy, numba, Pillow. Committed output is
used by ordinary native/ARM builds, so CI does not need these packages.
"""
import argparse, hashlib, json, math, struct
from pathlib import Path
import numpy as np
from numba import njit
from PIL import Image, ImageDraw
from scipy.optimize import least_squares

ROOT=Path(__file__).resolve().parents[1]
MODEL=json.loads((ROOT/'assets/flybody.json').read_text())
DATA=np.load(ROOT/'assets/flybody.npz')
V=DATA['vertices'].astype(np.float64);F=DATA['triangles'];VB=DATA['vertex_body'];FM=DATA['face_material']
B=MODEL['bodies'];NB=len(B);P=np.array([b['parent'] for b in B])
BP=np.array([b['pos'] for b in B]);BR=np.array([b['rotation'] for b in B])
JOINTS=[j for b in B for j in b['joints']];JN={j['name']:i for i,j in enumerate(JOINTS)}
JBODY=np.array([i for i,b in enumerate(B) for _ in b['joints']]);JAXIS=np.array([j['axis'] for j in JOINTS]);JPOS=np.array([j['pos'] for j in JOINTS])
RANGE=np.array([j['range'] for j in JOINTS]);REST=np.array([j['rest'] for j in JOINTS])
RGBA=np.array([m['rgba'] for m in MODEL['materials']])[FM]
BODY={b['name']:i for i,b in enumerate(B)}

@njit
def axisrot(a,t):
 c=math.cos(t);s=math.sin(t);x,y,z=a
 return np.array([[c+x*x*(1-c),x*y*(1-c)-z*s,x*z*(1-c)+y*s],
                  [y*x*(1-c)+z*s,c+y*y*(1-c),y*z*(1-c)-x*s],
                  [z*x*(1-c)-y*s,z*y*(1-c)+x*s,c+z*z*(1-c)]])

@njit
def forward(q):
 r=BR.copy();p=BP.copy()
 for b in range(1,NB):
  for j in range(len(q)):
   if JBODY[j]==b:
    t=axisrot(JAXIS[j],q[j]);p[b]+=r[b]@(JPOS[j]-t@JPOS[j]);r[b]=r[b]@t
  parent=P[b];p[b]=p[parent]+r[parent]@p[b];r[b]=r[parent]@r[b]
 return r,p

@njit
def transform(q):
 r,p=forward(q);v=np.empty_like(V)
 for i in range(len(V)):v[i]=r[VB[i]]@V[i]+p[VB[i]]
 return v

def setq(q,name,value):q[JN[name]]=np.clip(value,*RANGE[JN[name]])
def base_pose():
 q=np.zeros(len(JOINTS))
 for side in ('left','right'):
  for axis,val in [('yaw',1.5),('roll',.7),('pitch',-1.)]:setq(q,'wing_'+axis+'_'+side,val)
 # The proboscis is tucked away during locomotion.
 setq(q,'rostrum',.183);setq(q,'haustellum',.7)
 return q

LEGS=[(leg,side) for side in ('left','right') for leg in ('T1','T2','T3')]
CLAW=[BODY['claw_'+leg+'_'+side] for leg,side in LEGS]
LEG_JOINTS=[np.array([JN[n+'_'+leg+'_'+side] for n in ('coxa_abduct','coxa_twist','coxa','femur_twist','femur','tibia','tarsus')]) for leg,side in LEGS]

@njit
def foot(q,body):
 r,p=forward(q);return p[body]+r[body]@np.array([0.,.004,0.])

@njit
def solve_leg(q,body,joints,target):
 q=q.copy()
 for iteration in range(45):
  current=foot(q,body);err=target-current
  if np.sqrt(np.sum(err*err))<.00003:break
  jac=np.zeros((3,len(joints)))
  for col in range(len(joints)):
   j=joints[col];probe=q.copy();probe[j]+=.0001;jac[:,col]=(foot(probe,body)-current)/.0001
  delta=jac.T@np.linalg.solve(jac@jac.T+np.eye(3)*.000002,err)
  for col in range(len(joints)):
   j=joints[col];q[j]=min(RANGE[j,1],max(RANGE[j,0],q[j]+min(.22,max(-.22,delta[col]))))
 return q

def reach(q,body,joints,target):
 candidate=solve_leg(q,body,joints,target)
 if np.linalg.norm(foot(candidate,body)-target)<.0001:return candidate
 def residual(values):
  trial=q.copy();trial[joints]=values
  return np.r_[foot(trial,body)-target,(values-q[joints])*.00005]
 seeds=[candidate[joints],np.clip(REST[joints],RANGE[joints,0],RANGE[joints,1]),(RANGE[joints,0]+RANGE[joints,1])/2]
 for seed in seeds:
  result=least_squares(residual,np.clip(seed,RANGE[joints,0]+1e-8,RANGE[joints,1]-1e-8),bounds=(RANGE[joints,0],RANGE[joints,1]),max_nfev=300,gtol=1e-10,ftol=1e-10,xtol=1e-10)
  trial=q.copy();trial[joints]=result.x
  if np.linalg.norm(foot(trial,body)-target)<np.linalg.norm(foot(candidate,body)-target):candidate=trial
 return candidate

COUNTS=[4,8,10,6,8,2]
NAMES=['idle','walk','flight','feed','groom','rest']
def make_poses():
 global GROUND
 base=base_pose();feet=np.array([foot(base,b) for b in CLAW]);ground=float(np.mean(feet[:,2]));feet[:,2]=ground
 GROUND=ground
 # Six claws share one support plane. Each pose is solved in the original
 # joint coordinate system, preserving limb lengths and attachment points.
 for i in range(6):base=reach(base,CLAW[i],LEG_JOINTS[i],feet[i])
 poses=[];checks=[]
 for state,count in enumerate(COUNTS):
  for phase in range(count):
   q=base.copy();angle=phase*2*math.pi/(8 if state==2 else count);targets=[]
   for side in ('left','right'):setq(q,'antenna_'+side,.045*math.sin(angle+(side=='right')*.5))
   if state==1:
    for i,(leg,side) in enumerate(LEGS):
     u=(phase/count+(.5 if ((leg=='T2')!=(side=='right')) else 0))%1
     target=feet[i].copy()
     if u<.6:target[0]+=.015*(1-2*u/.6)
     else:
      swing=(u-.6)/.4;target[0]+=.015*(-1+2*swing);target[2]+=.018*math.sin(swing*math.pi)
     q=reach(q,CLAW[i],LEG_JOINTS[i],target);targets.append((i,target))
   elif state==2:
    for i,j in enumerate(JOINTS):
     if any(leg in j['name'] for leg in ('T1','T2','T3')):
      fold=0. if phase==8 else .5 if phase==9 else 1.
      q[i]=np.clip(REST[i]*fold,*RANGE[i])
    if phase>=8:angle=(2 if phase==8 else 3)*2*math.pi/8
    for side in ('left','right'):
     setq(q,'wing_yaw_'+side,1.25*math.sin(angle));setq(q,'wing_roll_'+side,.25+.7*math.cos(angle));setq(q,'wing_pitch_'+side,.6+.75*math.sin(angle+math.pi/2))
   elif state==3:
    setq(q,'head',-.10);setq(q,'rostrum',-.85+.05*math.sin(angle));setq(q,'haustellum',-.45+.09*math.cos(angle))
    for side in ('left','right'):setq(q,'labrum_'+side,.22+.1*math.sin(angle))
   elif state==4:
    # Front tarsi brush the anterior/lateral head surface, rather than
    # passing through eyes; the other four feet retain their support plane.
    for i in (0,3):
     target=np.array([.108+.003*math.sin(angle),(.044 if i==0 else -.044),-.012+.002*math.cos(angle)])
     q=reach(q,CLAW[i],LEG_JOINTS[i],target);targets.append((i,target))
   elif state==5:setq(q,'head',-.04)
   actual=[foot(q,b).tolist() for b in CLAW]
   error=max([float(np.linalg.norm(np.array(actual[i])-t)) for i,t in targets]+[0.])
   assert error<.002,(NAMES[state],phase,error) # Less than half a native pixel.
   assert np.all(q>=RANGE[:,0]-1e-8) and np.all(q<=RANGE[:,1]+1e-8)
   checks.append({'state':NAMES[state],'phase':phase,'foot_target_error':error,'feet':actual,'q':q.tolist()});poses.append(q)
 return poses,checks

@njit
def raster(v,faces,rgba,width,height,heading,elevation,scale):
 # flybody: X anterior, Y left, Z dorsal. Heading zero points down-screen.
 a=heading*math.pi/128;ce=math.cos(elevation);se=math.sin(elevation)
 rot=np.array([[math.sin(a),-math.cos(a),0.],[math.cos(a)*se,math.sin(a)*se,-ce],[math.cos(a)*ce,math.sin(a)*ce,se]])
 center=np.array([-.04,0.,-.04]);screen=(v-center)@rot.T
 screen[:,0]=screen[:,0]*scale+width/2;screen[:,1]=screen[:,1]*scale+height/2
 rgb=np.zeros((height,width,3));rgb[:,:,:]=np.array([225.,226.,215.]);alpha=np.zeros((height,width))
 depth=np.full((height,width),-1e10)
 light=np.array([.15,-.45,.88]);light/=np.sqrt(np.sum(light*light))
 # Opaque body first. Transparent membrane is blended against visible body.
 for transparent in range(2):
  for i in range(len(faces)):
   opacity=rgba[i,3]
   if (opacity<.9)!=bool(transparent):continue
   f=faces[i];p0,p1,p2=screen[f[0]],screen[f[1]],screen[f[2]]
   den=(p1[1]-p2[1])*(p0[0]-p2[0])+(p2[0]-p1[0])*(p0[1]-p2[1])
   if abs(den)<1e-8:continue
   n=np.cross(v[f[1]]-v[f[0]],v[f[2]]-v[f[0]]);norm=math.sqrt(np.sum(n*n))
   if norm<1e-12:continue
   n/=norm;illum=.33+.67*abs(np.dot(n,light))
   color=np.sqrt(rgba[i,:3]*illum)*255
   if transparent:color=np.array([188.,201.,201.]);opacity=.53
   x0=max(0,int(math.floor(min(p0[0],p1[0],p2[0]))));x1=min(width-1,int(math.ceil(max(p0[0],p1[0],p2[0]))))
   y0=max(0,int(math.floor(min(p0[1],p1[1],p2[1]))));y1=min(height-1,int(math.ceil(max(p0[1],p1[1],p2[1]))))
   for y in range(y0,y1+1):
    for x in range(x0,x1+1):
     u=((p1[1]-p2[1])*(x+.5-p2[0])+(p2[0]-p1[0])*(y+.5-p2[1]))/den
     w=((p2[1]-p0[1])*(x+.5-p2[0])+(p0[0]-p2[0])*(y+.5-p2[1]))/den
     t=1-u-w
     if u<0 or w<0 or t<0:continue
     z=u*p0[2]+w*p1[2]+t*p2[2]
     if z<depth[y,x]-1e-6:continue
     rgb[y,x]=color*opacity+rgb[y,x]*(1-opacity)
     alpha[y,x]=opacity+alpha[y,x]*(1-opacity)
     depth[y,x]=z
 return rgb,alpha

SW,SH=192,160
SCALES=[230.,245.,230.]
ELEVATIONS=[54.,40.,90.]
GROUND=-.1234
def render_pose(q,heading=24,flat=False,scale=230.,elevation=None):
 v=transform(q)
 rgb,alpha=raster(v,F,RGBA,SW*2,SH*2,heading,math.radians(elevation if elevation is not None else 90 if flat else 54),scale*2)
 im=Image.fromarray(np.uint8(np.clip(rgb,0,255))).resize((SW,SH),Image.Resampling.BOX)
 mask=Image.fromarray(np.uint8(np.clip(alpha*255,0,255))).resize((SW,SH),Image.Resampling.BOX)
 return im,mask

def anchor(heading,flat,scale,elevation=None):
 a=heading*math.pi/128;e=math.radians(elevation if elevation is not None else 90 if flat else 54)
 return [round(SW/2+.04*math.sin(a)*scale),round(SH/2+(.04*math.cos(a)*math.sin(e)-(GROUND+.04)*math.cos(e))*scale)]

def encode_frame(indexed,mask,origin):
 keep=np.array(mask)>48;arr=np.array(indexed);ys=np.where(keep.any(axis=1))[0]
 assert len(ys) and not keep[0].any() and not keep[-1].any() and not keep[:,0].any() and not keep[:,-1].any(),'Sprite touches canvas boundary'
 start,end=int(ys[0]),int(ys[-1]);out=bytearray([*origin,start,end-start+1]);bounds=[SW,SH,0,0]
 for y in range(start,end+1):
  spans=[];x=0
  while x<SW:
   if not keep[y,x]:x+=1;continue
   first=x
   while x<SW and keep[y,x]:x+=1
   spans.append(bytes([first,x-first])+arr[y,first:x].tobytes());bounds[0]=min(bounds[0],first);bounds[2]=max(bounds[2],x)
  out.append(len(spans))
  for span in spans:out.extend(span)
 bounds[1]=start;bounds[3]=end+1
 return bytes(out),bounds

def bake(poses,checks):
 # Reserve UI, food, stone and floor indices. Fit 144 sprite colors to all
 # behaviors and both camera elevations, then round to the GBA's RGB555.
 slots=list(range(16,80))+list(range(128,160))+list(range(208,256))
 # Keep one scale per behavior and camera, independent of direction/phase.
 # Flight needs a wider camera to contain the entire downstroke. Ground
 # poses retain their larger scale. Bounds are checked before rasterizing.
 scales=[]
 for view,nominal in enumerate(SCALES):
  se=math.sin(math.radians(ELEVATIONS[view]));ce=math.cos(math.radians(ELEVATIONS[view]));per_state=[];start=0
  for state,n in enumerate(COUNTS):
   scale=min(nominal,190. if view==1 else 180.) if state==2 else nominal
   for q in poses[start:start+n]:
    v=transform(q)
    for h in range(0,256,4):
     a=h*math.pi/128;xx=v[:,0]*math.sin(a)-v[:,1]*math.cos(a);yy=(v[:,0]*math.cos(a)+v[:,1]*math.sin(a))*se-(v[:,2]-GROUND)*ce
     scale=min(scale,84/max(abs(xx.min()),abs(xx.max())),(56 if view==2 else 68)/max(.001,-yy.min()),(55 if view==2 else 42)/max(.001,yy.max()))
   per_state.append(float(math.floor(scale)));start+=n
  scales.append(per_state)
 print('Camera scales',scales,flush=True)
 samples=[]
 for i,q in enumerate(poses):
  state=next(s for s in range(6) if i<sum(COUNTS[:s+1]))
  for heading in (0,32,64,96,128,160,192,224):
   im,mask=render_pose(q,heading,scale=scales[0][state]);samples.append(im)
 atlas=Image.new('RGB',(SW*16,SH*math.ceil(len(samples)/16)),(225,226,215))
 for i,im in enumerate(samples):atlas.paste(im,((i%16)*SW,(i//16)*SH))
 quant=atlas.quantize(colors=len(slots),method=Image.Quantize.MEDIANCUT,dither=Image.Dither.NONE)
 colors=np.array(quant.getpalette(),dtype=np.uint8).reshape(-1,3)[:len(slots)];colors=(colors//8)*8
 # Only available sprite colors are quantization candidates. Padding entries
 # repeat the first color and are remapped to the same legal palette slot.
 palette=Image.new('P',(1,1));palette.putpalette(colors.flatten().tolist()+colors[0].tolist()*(256-len(colors)))
 mapping=np.array(slots+[slots[0]]*(256-len(slots)),dtype=np.uint8)
 (ROOT/'assets/fly_palette.json').write_text(json.dumps({str(k):c.tolist() for k,c in zip(slots,colors)},separators=(',',':'))+'\n')
 frames=[];records=[];sheets=[];count=64
 for view in range(3):
  flat=view==2;sheet=Image.new('RGB',(SW*10,SH*6),(225,226,215));draw=ImageDraw.Draw(sheet)
  offset=0
  for state,n in enumerate(COUNTS):
   scale=scales[view][state]
   for phase in range(n):
    q=poses[offset+phase]
    for direction in range(count):
     heading=direction*4;im,mask=render_pose(q,heading,flat,scale,ELEVATIONS[view])
     indexed=im.quantize(palette=palette,dither=Image.Dither.NONE);indexed=Image.fromarray(mapping[np.array(indexed)],mode='P')
     origin=anchor(heading,flat,scale,ELEVATIONS[view])
     payload,bounds=encode_frame(indexed,mask,origin);frames.append(payload)
     records.append({'view':view,'state':state,'phase':phase,'direction':direction,'bounds':bounds,'anchor':origin})
     if direction==6:
      sheet.paste(im,(phase*SW,state*SH));draw.text((phase*SW+4,state*SH+4),NAMES[state]+' '+str(phase),fill=(20,25,25))
   offset+=n
   print('Baked',view,NAMES[state],len(frames),flush=True)
  sheets.append(sheet)
 total=len(frames);header=struct.pack('<4sHHHHHH',b'FLY3',count,sum(COUNTS),3,SW,SH,1)
 offsets=[len(header)+(total+1)*4]
 for f in frames:offsets.append(offsets[-1]+len(f))
 blob=header+struct.pack('<'+'I'*len(offsets),*offsets)+b''.join(frames)
 (ROOT/'assets/fly.sprites').write_bytes(blob)
 (ROOT/'src/body_frames.h').write_text('/* Generated by tools/bake_body.py; flybody-derived graphics, Apache-2.0. */\n#define BODY_DIRECTIONS 64\n#define BODY_POSES '+str(sum(COUNTS))+'\n#define BODY_VIEWS 3\nextern const unsigned char fly_sprites[];\n')
 # Each checked bounding box includes all visible feet, antennae and wings.
 # Viewer anchoring is derived from their extrema, never guessed per angle.
 fits=[]
 for view in range(3):
  selected=[r for r in records if r['view']==view]
  low=min(r['bounds'][1]-r['anchor'][1] for r in selected);high=max(r['bounds'][3]-r['anchor'][1] for r in selected)
  left=min(r['bounds'][0]-r['anchor'][0] for r in selected);right=max(r['bounds'][2]-r['anchor'][0] for r in selected)
  fits.append({'min_dy':low,'max_dy':high,'min_dx':left,'max_dx':right,'vertical_span':high-low})
 validation={'source_commit':MODEL['commit'],'sprite_sha256':hashlib.sha256(blob).hexdigest(),'frames':total,'directions':count,'pose_counts':COUNTS,'views':3,'scale':scales,'camera_elevation':ELEVATIONS,'ground_plane':GROUND,'mesh_triangles':len(F),'max_foot_target_error':max(c['foot_target_error'] for c in checks),'pose_checks':checks,'fit':fits,'frame_bounds':records}
 (ROOT/'docs/body-validation.json').write_text(json.dumps(validation,separators=(',',':'))+'\n')
 for view,sheet in enumerate(sheets):sheet.save(ROOT/f'docs/body-poses-{view}.png')
 print('Sprite bytes',len(blob),'fit',fits,flush=True)

def main():
 p=argparse.ArgumentParser();p.add_argument('--probe',action='store_true');p.add_argument('--poses',action='store_true');a=p.parse_args()
 if a.probe:
  q=base_pose();r,p0=forward(q)
  for name in ('thorax','head','claw_T1_left','claw_T2_left','claw_T3_left'):print(name,p0[BODY[name]])
  canvas=Image.new('RGB',(SW*4,SH*4),(225,226,215))
  for i in range(16):
   im,mask=render_pose(q,i*16);canvas.paste(im,((i%4)*SW,(i//4)*SH))
  canvas.resize((1024,896),Image.Resampling.NEAREST).save(ROOT/'build/body-probe.png')
  return
 if a.poses:
  poses,checks=make_poses();canvas=Image.new('RGB',(8*SW,6*SH),(225,226,215));draw=ImageDraw.Draw(canvas)
  offset=0
  for state,count in enumerate(COUNTS):
   for phase in range(count):
    im,mask=render_pose(poses[offset+phase]);canvas.paste(im,(phase*SW,state*SH));draw.text((phase*SW+3,state*SH+3),NAMES[state]+' '+str(phase),fill=(20,25,25))
   offset+=count
  canvas.save(ROOT/'build/body-poses.png');(ROOT/'build/body-poses.json').write_text(json.dumps(checks));return
 poses,checks=make_poses();bake(poses,checks)

if __name__=='__main__':main()
