"""Validate every generated frame, support/joint checks, and all view bounds."""
import hashlib,json,struct
from pathlib import Path
model=json.loads(Path('assets/flybody.json').read_text())
validation=json.loads(Path('docs/body-validation.json').read_text())
blob=Path('assets/fly.sprites').read_bytes()
magic,directions,poses,views,width,height,version=struct.unpack_from('<4sHHHHHH',blob)
assert (magic,directions,poses,views,version)==(b'FLY3',64,38,3,1)
assert hashlib.sha256(blob).hexdigest()==validation['sprite_sha256']
assert model['commit']==validation['source_commit']
assert sum(validation['pose_counts'])==poses
frames=directions*poses*views
assert validation['frames']==len(validation['frame_bounds'])==frames
offsets=struct.unpack_from('<'+'I'*(frames+1),blob,16)
assert offsets[0]==16+(frames+1)*4 and offsets[-1]==len(blob)
slots=set(range(16,80))|set(range(128,160))|set(range(208,256))
for index,record in enumerate(validation['frame_bounds']):
 data=memoryview(blob)[offsets[index]:offsets[index+1]]
 ax,ay,first,rows=data[:4];at=4
 assert [ax,ay]==record['anchor'] and 0<=first<first+rows<=height
 bounds=[width,first,0,first+rows]
 for y in range(first,first+rows):
  n=data[at];at+=1;previous=0
  for span in range(n):
   x,length=data[at:at+2];at+=2
   assert length>0 and previous<=x<x+length<=width
   previous=x+length;bounds[0]=min(bounds[0],x);bounds[2]=max(bounds[2],x+length)
   assert set(data[at:at+length])<=slots
   at+=length
 assert at==len(data) and bounds==record['bounds']
 anchor_y=78 if record['view']==2 else 90
 assert 0<=88-ax+bounds[0]<88-ax+bounds[2]<=178
 assert 18<=anchor_y-ay+bounds[1]<anchor_y-ay+bounds[3]<=136
joints=[j for b in model['bodies'] for j in b['joints']]
assert len(validation['pose_checks'])==poses
for pose in validation['pose_checks']:
 assert len(pose['q'])==len(joints)
 for value,joint in zip(pose['q'],joints):assert joint['range'][0]-1e-8<=value<=joint['range'][1]+1e-8
 assert pose['foot_target_error']<.002
 if pose['state'] in ('idle','feed','rest'):
  assert max(f[2] for f in pose['feet'])-min(f[2] for f in pose['feet'])<.0001
assert len([b for b in model['bodies'] if b['name'].startswith('coxa_')])==6
assert Path('licenses/flybody-Apache-2.0.txt').exists()
print(f'Anatomy/joint/foot checks and all {frames:,} sprite frames passed')
