#!/usr/bin/env python3
"""Run the real cartridge tests against a separately built mGBA static core."""
import argparse, os, re, shutil, subprocess, sys
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--build',type=Path,required=True);a=p.parse_args()
source=a.source.resolve();build=a.build.resolve();os.chdir(Path(__file__).resolve().parents[1])
flags=(build/'CMakeFiles/mgba.dir/flags.make').read_text()
defs=re.search(r'^C_DEFINES = (.*)$',flags,re.M).group(1).split()
libs=['-lz','-ledit','-lpthread']
if sys.platform=='darwin':libs+=['-framework','CoreFoundation']
else:libs+=['-lm','-ldl']
subprocess.run([os.environ.get('CC','cc'),'-O2',*defs,'-I'+str(source/'include'),'-I'+str(build/'include'),'-Isrc','tools/emulator_check.c',str(build/'libmgba.a'),*libs,'-o','build/emulator_check'],check=True)
nm=os.environ.get('LLVM_NM',shutil.which('llvm-nm') or '/opt/homebrew/opt/llvm/bin/llvm-nm')
symbols=subprocess.check_output([nm,'-n','build/fly.elf'],text=True)
address=re.search(r'^(\w+) B fly$',symbols,re.M).group(1)
subprocess.run(['build/emulator_check','build/fly.gba',address],check=True)
