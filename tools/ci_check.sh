#!/usr/bin/env bash
set -euo pipefail
# Build and test only public, pinned emulator source, outside the checkout.
make all test
work_dir="${RUNNER_TEMP:-${TMPDIR:-/tmp}}/flywire-gba-mgba"
archive="$work_dir/mgba.tar.gz"
mkdir -p "$work_dir"
if [[ ! -f "$work_dir/build/libmgba.a" ]]; then
  curl --fail --location --retry 3 \
    https://github.com/mgba-emu/mgba/archive/refs/tags/0.10.5.tar.gz -o "$archive"
  python3 - "$archive" <<'PY'
import hashlib, sys
from pathlib import Path
expected='91d6fbd32abcbdf030d58d3f562de25ebbc9d56040d513ff8e5c19bee9dacf14'
assert hashlib.sha256(Path(sys.argv[1]).read_bytes()).hexdigest()==expected, 'mGBA archive checksum mismatch'
PY
  tar -xzf "$archive" -C "$work_dir"
  cmake -S "$work_dir/mgba-0.10.5" -B "$work_dir/build" \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DBUILD_QT=OFF -DBUILD_SDL=OFF -DBUILD_GL=OFF \
    -DBUILD_GLES2=OFF -DBUILD_GLES3=OFF -DBUILD_TEST=OFF \
    -DUSE_FFMPEG=OFF -DUSE_LIBZIP=OFF -DUSE_PNG=OFF -DUSE_SQLITE3=OFF \
    -DBUILD_STATIC=ON -DBUILD_SHARED=OFF
  cmake --build "$work_dir/build" --parallel 2
fi
python3 tools/run_emulator_check.py --source "$work_dir/mgba-0.10.5" --build "$work_dir/build"
python3 tools/export_media.py
