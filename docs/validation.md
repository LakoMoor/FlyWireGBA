# Build and cartridge validation

Validated on 4 October 2026. Local macOS build: LLVM 23. The Linux CI/release
build may have a different binary hash; its SHA-256 is published in the release
manifest and `SHA256SUMS`.

Local ROM: `dist/fly.gba`, 16,777,216 bytes.

SHA-256: `df541929673e7dd5688fd3b42fe9e0e76cb7f4355663e6d286ba0609d568012e`.

## Memory

| Resource | Used | Available |
|---|---:|---:|
| Padded cartridge ROM | 16 MiB | 32 MiB |
| EWRAM static data | 39,712 bytes | 262,144 bytes |
| IWRAM executable code | 26,428 bytes | 32,768 bytes |
| Framebuffer, included in EWRAM above | 38,400 bytes | — |
| Bitmap VRAM pages, including page gap | 81,920 bytes | 98,304 bytes |

The previous triangle buffer is removed. Shaded body frames reside in cartridge
ROM and are drawn directly from transparent row spans, without a decompression
buffer. The linker caps IWRAM code at 30 KiB to leave stack space; the actual
map is in `build/fly.map`.

## Actual ROM in mGBA 0.10.5

The statically built mGBA core executes the cartridge's ARM7 instructions,
reads emulated RAM, and captures the actual framebuffer. It uses mGBA's built-in
BIOS implementation. The screenshots/GIF are cartridge output.

Checks passed:

- Boot and time advancement: 58 model ticks after 120 hardware frames, including boot.
- Autonomous/manual switching, movement, turning, flight, landing, and grooming.
- Sugar contact, neural response, and food intake. The feeding test places the
  fly on sugar through emulated RAM, then checks circuit-driven intake.
- Standard, close, and overhead views, neural map, both telemetry pages, help,
  pause, and resume.
- 288 visual captures: 16 headings × six behaviors × three views, with state
  frozen through emulated RAM to inspect attachments and occlusion. Each viewport is compared pixel by pixel against native rendering, allowing one RGB level for color expansion.

Representative hardware-timer readings after the model update:

| Scene | Displayed FPS (rounded down) | Approximate refresh | CPU computation/render share |
|---|---:|---:|---:|
| Standard arena | 29 | 30 FPS | 67% |
| Overhead arena | 29 | 30 FPS | 73% |
| General telemetry | 29 | 30 FPS | 62% |

These are representative measurements, not a guaranteed worst-case minimum.
The ROM measures them continuously. Simulation advances at 30 Hz independently
of rendering. Compared with the previous procedural mesh, the checked standard
scene improved from about 12 to 30 FPS.

Reproduce with a separately built static mGBA core:

```sh
make all test
python3 tools/run_emulator_check.py --source /path/mgba-0.10.5 --build /path/mgba-build
python3 tools/export_media.py  # Pillow
```

CI builds the checksum-pinned mGBA source with Qt, SDL, OpenGL, FFmpeg, PNG,
SQLite, and LibZip disabled. mGBA is not included in the ROM.

## Anatomy and all model positions

- Original flybody hierarchy and attachment points; 32,980 reduced triangles.
- 64 headings, 38 animation poses, three camera views: **7,296 frames**.
- Every generated span, palette index, offset, and viewport bound checked.
- Every combination rendered under native ASan/UBSan using the ROM decoder.
- Joint angles checked against original limits; grounded support feet share
  one plane; walking uses solved tripod stance/swing targets.
- Maximum solved foot-target error: 0.00007023 model units,
  below half a native pixel.
- Flight includes extended-leg launch/landing poses and folded flight legs.
- Header/footer labels and pause indication leave the specimen unobstructed.
- Regression checks in native rendering and actual mGBA output ensure a distant
  obstacle cannot cover the fly's compound eyes in the reported grooming pose.

See [body-model.md](body-model.md), [body-validation.json](body-validation.json),
and the animation/turntable sheets linked there.

## Simulation and data

ASan/UBSan checks cover movement, takeoff/landing, grooming, rest, obstacles,
view changes, neuron selection, page switching, pause, and six autonomous minutes.
Position, energy, and hunger remain bounded; the fly moves and feeds.

The sugar-contact test produced 17,936 spikes and 67 food-intake ticks over 300
model ticks; hunger fell from 480 to 3. Data tests check 128 unique root IDs,
published sensory/motor seeds, all 2,048 edges against retained source IDs,
signs and quantized weights, and the cartridge header/checksum/size.

Physical GBA/flash-cartridge operation has not been tested. Audio and saved
sessions are not implemented. Neural dynamics, behavioral control, and body
animations are illustrative; these checks do not establish biological accuracy.
