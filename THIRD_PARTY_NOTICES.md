# Third-party attribution

## FlyWire connectome — CC BY-NC 4.0

`src/connectome.c`, `docs/connectome_edges.csv`, `docs/connectome.json` and
ROMs built with them contain a modified subset of the FlyWire FAFB v783 data.
The data remains subject to Creative Commons Attribution-NonCommercial 4.0
International; the project's MIT license does not override these terms.

- FlyWire Consortium: https://flywire.ai/
- Data guidelines: https://flywire.ai/guidelines
- License: https://creativecommons.org/licenses/by-nc/4.0/
- Dorkenwald et al. (2024), *Neuronal wiring diagram of an adult brain*,
  Nature. https://doi.org/10.1038/s41586-024-07558-y
- Shiu et al. (2024), *A Drosophila computational brain model reveals
  sensorimotor processing*, Nature. https://doi.org/10.1038/s41586-024-07763-9
- Data mirror and annotated seed IDs used:
  https://github.com/philshiu/Drosophila_brain_model

Modifications: 128 neurons selected by three-hop anatomical relevance between
sugar sensory seeds and MN9; strongest 2048 induced directed edges retained;
weights aggregated, transformed and quantized. Details and source hashes are
in `docs/connectome.json`. Schematic layout and procedural fly body are original
project artwork. The simulated dynamics are not the calibrated Shiu model.

## Cartridge header

Standard Nintendo-compatible GBA boot logo bytes are included solely as the
required cartridge header. Format reference:
https://github.com/devkitPro/gba-tools/blob/master/src/gbafix.c
Nintendo trademarks and logo are not covered by the project's MIT license.

## Test-only emulator

mGBA 0.10.5 is downloaded and built by tests, never bundled with the ROM.
mGBA: https://github.com/mgba-emu/mgba — Mozilla Public License 2.0.
