#!/usr/bin/env python3
"""Dependency-free GBA ROM builder: Clang/LLD."""
import os, shutil, subprocess, sys
from pathlib import Path
root=Path(__file__).resolve().parents[1];os.chdir(root);Path('build').mkdir(exist_ok=True)
def find(name,extra=()):
 for candidate in [os.environ.get(name.upper().replace('-','_').replace('.','_')),shutil.which(name),*extra]:
  if candidate and Path(candidate).is_file():return str(candidate)
 raise SystemExit('Missing tool: '+name)
clang=find('clang',['/opt/homebrew/opt/llvm/bin/clang'])
# Prefer upstream Clang, rather than Apple's host-only distribution.
if not os.environ.get('CLANG') and Path('/opt/homebrew/opt/llvm/bin/clang').exists():clang='/opt/homebrew/opt/llvm/bin/clang'
linker=find('ld.lld',list(Path.home().glob('.rustup/toolchains/*/lib/rustlib/*/bin/gcc-ld/ld.lld')))
objcopy=find('llvm-objcopy',['/opt/homebrew/opt/llvm/bin/llvm-objcopy'])
flags=['--target=arm-none-eabi','-mcpu=arm7tdmi','-marm','-mfloat-abi=soft','-O2','-ffreestanding','-fno-builtin','-fno-unwind-tables','-fno-asynchronous-unwind-tables','-Wall','-Wextra','-Werror','-Isrc']
objects=[]
for source in ['src/start.S','src/gba.c','src/sim.c','src/render.c','src/connectome.c','src/palette.c']:
 obj='build/'+Path(source).stem+'.o';objects.append(obj)
 subprocess.run([clang,*flags,'-c',source,'-o',obj],check=True)
subprocess.run([linker,'-T','gba.ld','-Map=build/fly.map','--gc-sections',*objects,'-o','build/fly.elf'],check=True)
subprocess.run([objcopy,'-O','binary','build/fly.elf','build/fly.gba'],check=True)
rom=bytearray(Path('build/fly.gba').read_bytes())
logo=bytes.fromhex('24ffae51699aa2213d84820a84e409ad11248b98c0817f21a352be199309ce2010464a4af82731ec58c7e83382e3cebf85f4df94ce4b09c194568ac01372a7fc9f844d73a3ca9a615897a327fc039876231dc7610304ae56bf38840040a70efdff52fe036f9530f197fbc08560d68025a963be03014e38e2f9a234ffbb3e0344780090cb88113a9465c07c6387f03cafd625e48b380aac7221d4f807')
assert len(logo)==156
rom[4:160]=logo;rom[160:172]=b'FLYWIRE GBA ';rom[172:176]=b'FLYE';rom[176:178]=b'01';rom[178]=0x96
rom[179:189]=bytes(10);rom[189]=(-sum(rom[160:189])-0x19)&255
size=1<<(len(rom)-1).bit_length();rom.extend(bytes([255])*(max(size,32768)-len(rom)))
Path('build/fly.gba').write_bytes(rom)
Path('dist').mkdir(exist_ok=True)
Path('dist/fly.gba').write_bytes(rom)
print(f'Built build/fly.gba: {len(rom):,} bytes. Header checksum: {rom[189]:02x}')
