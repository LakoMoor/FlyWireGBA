# Anatomical body and animation checks

The body graphics derive from [flybody](https://github.com/TuragaLab/flybody),
created by Google DeepMind and HHMI Janelia Research Campus, at revision
`d015e9bfe441bd90ae431bac24c55cb74bdbce26`. Its Apache-2.0 license is bundled in
`licenses/flybody-Apache-2.0.txt`.

The original MJCF hierarchy supplies the body transforms, joint anchors,
axes, limits, and limb lengths. Meshes are reduced per material to 32,980
triangles. Normal-seam vertices are welded before simplification; otherwise
mesh reduction would leave holes in the head, thorax, and eyes.

The reduced articulated model is in `assets/flybody.npz`; hierarchy and source
metadata are in `assets/flybody.json`. The ROM does not simulate this mesh or
MuJoCo physics. It displays shaded, transparent frames rasterized from the
model offline with a depth buffer and composited wing membranes.

There are **64 directions, 38 animation poses, and three camera views**:
7,296 indexed frames. The cameras show the standard view, a lower close view,
and the overhead view. Flight uses a wider framing to keep both wings visible.
The same framing is used throughout a behavior's cycle, regardless of heading.

| Behavior | Poses | Placement |
|---|---:|---|
| Idle | 4 | Six grounded feet, subtle antennal movement |
| Walk | 8 | Alternating tripod stance/swing, solved foot targets |
| Flight | 10 | Eight wing phases; two launch/landing poses with extending legs |
| Feed | 6 | Grounded feet, articulated proboscis and labrum |
| Groom | 8 | Four support feet; front tarsi reach the anterior head |
| Rest | 2 | Grounded support pose |

The cycles are illustrative animations, not measured kinematics. The renderer
uses body graphics from a scientific model; its gait controller is not a
validated biomechanical simulation. The wing cycle does not reproduce biological
wingbeat frequency.

## Verification

`docs/body-validation.json` records each frame's visible bounds and anchor,
the source revision, sprite SHA-256, all joint poses, support coordinates,
and inverse-kinematics target errors.

- Each generated joint value must remain inside its original joint limits.
- Grounded support feet share one plane; walk targets alternate stance and swing.
- Front-tarsus target error during grooming is below half a native LCD pixel.
- Every direction, phase, and view must fit the specimen viewport, including
  antennae, feet, and wing tips. Canvas-edge clipping causes generation to fail.
- `tests/test_body.py` parses every span and palette index, verifies frame offsets
  and their bounds, and checks the recorded joint/support data.
- Native ASan/UBSan tests render all 7,296 combinations through the same sprite
  decoder used by the ARM ROM.
- The mGBA cartridge test captures 288 actual frames: 16 headings × six behaviors
  × three views. State is frozen through emulated RAM for reproducible inspection. Every captured viewport is compared pixel by pixel against native rendering (allowing one RGB level for RGB555 expansion).

The controls and current behavior are outside the specimen viewport so they
cannot cover wing tips. Pause is indicated in the header, leaving the fly visible.

[Standard view turntable](body-turntable-0.png),
[close view turntable](body-turntable-1.png),
[overhead turntable](body-turntable-2.png).

[Every animation pose: standard](body-poses-0.png),
[close](body-poses-1.png), [overhead](body-poses-2.png).

## Optional regeneration

Ordinary builds use the committed sprite asset and require no model downloads
or scientific Python dependencies. To regenerate the source model and frames:

```sh
python3 -m venv /tmp/flybody-assets
/tmp/flybody-assets/bin/pip install mujoco==3.14.0 trimesh==5.1.1 fast-simplification==0.2.0 numpy==2.5.3 numba==0.68.0 scipy==1.18.1 Pillow
# Download/check out flybody at the pinned revision above.
/tmp/flybody-assets/bin/python tools/import_body.py /path/flybody/flybody/fruitfly/assets
/tmp/flybody-assets/bin/python tools/bake_body.py
python3 tools/assets.py
make all test
```

The authoring rasterizer is CPU-based and does not need a graphical display.
`--probe` and `--poses` on `tools/bake_body.py` produce intermediate inspection
sheets in `build/`. The generated sprite data stays in cartridge ROM; playback
needs no extra decompression buffer in EWRAM.
