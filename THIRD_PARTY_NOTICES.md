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
in `docs/connectome.json`. The neural map layout is original project artwork.
The simulated dynamics are not the calibrated Shiu model.

## Anatomical fly body — Apache-2.0

`assets/flybody.npz`, `assets/flybody.json`, `assets/fly.sprites`,
`assets/fly_palette.json`, body screenshots, and the corresponding ROM graphics
are derived from flybody, developed by Google DeepMind and HHMI Janelia Research
Campus. The project's MIT license does not relicense these assets.

- Source: https://github.com/TuragaLab/flybody
- Source revision: `d015e9bfe441bd90ae431bac24c55cb74bdbce26`
- Original model: `flybody/fruitfly/assets/fruitfly.xml` and its OBJ meshes
- Full license: `licenses/flybody-Apache-2.0.txt`

Changes: welded normal-seam vertices, per-material quadric mesh reduction,
illustrative joint animations within the original joint limits, inverse
kinematics for foot placement, offline software rasterization, translucent
wing compositing, and RGB555 palette quantization. Original body transforms,
limb lengths, and joint attachment points are retained. This ROM does not run
MuJoCo, flybody physics, or flybody's learned controllers.

## Cartridge header

Standard Nintendo-compatible GBA boot logo bytes are included solely as the
required cartridge header. Format reference:
https://github.com/devkitPro/gba-tools/blob/master/src/gbafix.c
Nintendo trademarks and logo are not covered by the project's MIT license.

## Test-only emulator

mGBA 0.10.5 is downloaded and built by tests, never bundled with the ROM.
mGBA: https://github.com/mgba-emu/mgba — Mozilla Public License 2.0.
