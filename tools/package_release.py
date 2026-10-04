#!/usr/bin/env python3
"""Create a versioned ROM, deterministic release archive and SHA-256 checksums."""
import argparse, hashlib, json, re, subprocess, zipfile
from pathlib import Path
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--tag');a=p.parse_args()
version=(root/'VERSION').read_text().strip();tag=a.tag or 'v'+version
if not re.fullmatch(r'v\d+\.\d+\.\d+(?:-(?:alpha|beta|rc)\.\d+)?',tag):p.error('Expected a semantic version tag, e.g. v0.1.0')
if tag!='v'+version:p.error('Tag must match VERSION; update VERSION before tagging')
rom=(root/'dist/fly.gba').read_bytes()
if len(rom)<192 or rom[178]!=0x96 or (sum(rom[160:190])+0x19)%256:p.error('Invalid GBA ROM; build and test first')
out=root/'dist/release';out.mkdir(parents=True,exist_ok=True)
for old in out.iterdir():
 if old.is_file():old.unlink()
name=f'flywire-gba-{tag}'
(out/(name+'.gba')).write_bytes(rom)
try:commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True,stderr=subprocess.DEVNULL).strip()
except subprocess.CalledProcessError:commit='local-development'
metadata={'project':'FlyWireGBA','version':version,'tag':tag,'commit':commit,'rom_sha256':hashlib.sha256(rom).hexdigest(),'rom_bytes':len(rom),'data_license':'CC BY-NC 4.0','connectome':json.loads((root/'docs/connectome.json').read_text()),'validation_suite':'Release workflow requires native ASan/UBSan, provenance/header validation and real mGBA cartridge checks'}
(out/'manifest.json').write_text(json.dumps(metadata,indent=2)+'\n')
notes=(root/'CHANGELOG.md').read_text().split('## '+version+'\n',1)[1].split('\n## ',1)[0].strip()
(out/'release-notes.md').write_text(notes+'\n\nDownload `'+name+'.gba` and open it in mGBA or load it on a compatible GBA flash cartridge. Verify with `SHA256SUMS`.\n\nFlyWire-derived data: CC BY-NC 4.0. See the attribution included in the ZIP.\n')
files={name+'.gba':rom,'README.md':(root/'README.md').read_bytes(),'LICENSE':(root/'LICENSE').read_bytes(),'THIRD_PARTY_NOTICES.md':(root/'THIRD_PARTY_NOTICES.md').read_bytes(),'manifest.json':(out/'manifest.json').read_bytes(),'CHANGELOG.md':(root/'CHANGELOG.md').read_bytes()}
files['CONTRIBUTING.md']=(root/'CONTRIBUTING.md').read_bytes()
for doc in sorted((root/'docs').rglob('*')):
 if doc.is_file() and doc.suffix in {'.png','.gif','.md','.csv','.json'}:files[doc.relative_to(root).as_posix()]=doc.read_bytes()
with zipfile.ZipFile(out/(name+'.zip'),'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as archive:
 for filename,data in sorted(files.items()):
  info=zipfile.ZipInfo(name+'/'+filename,date_time=(1980,1,1,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=0o100644<<16;archive.writestr(info,data)
assets=[out/(name+'.gba'),out/(name+'.zip'),out/'manifest.json']
(out/'SHA256SUMS').write_text(''.join(hashlib.sha256(f.read_bytes()).hexdigest()+'  '+f.name+'\n' for f in assets))
print('Release assets: '+str(out))
