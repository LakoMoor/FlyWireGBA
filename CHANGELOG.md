# Changelog

## 0.2.0

- Added PET Tamagotchi mode with eight care actions and seven condition indicators.
- Food intake remains gated by MN9; care, sleep, play, injury and neglect affect the pet.
- Added age, bond, stress, death and a three-second held-button new-pet action.
- Added checksum-protected, two-bank SRAM persistence and interrupted-write recovery.
- Added native sanitizer and real-cartridge pet/save checks, English help and a care GIF.

## 0.1.1

- Corrected obstacle depth order: distant rocks no longer cover the fly's head.
- Added a regression check for compound-eye visibility beside a distant rock.

## 0.1.0

- First Game Boy Advance release of FlyWireGBA.
- Redesigned 240x160 interface with specimen viewport, separate telemetry panel,
  persistent navigation, selected-neuron connectivity and readable metric cards.
- Anatomical flybody-derived model with original joint attachments, compound eyes,
  segmented abdomen, veined translucent wings, and an articulated tripod gait.
- Pre-rendered 3D graphics for 64 directions, 38 animation poses, and three views;
  validated foot placement, joint limits, and frame bounds.
- Flight, landing, grooming and feeding animations, plus close and overhead views.
- Real FlyWire FAFB v783 subgraph: 128 neurons and 2048 directed anatomical edges.
- Hardware-timed 30 Hz simulation, autonomous/manual control and live telemetry.
- Sanitizer tests, provenance/header validation and real mGBA cartridge tests.
- GitHub CI and automatic ROM publication for version tags.

This is a reduced demonstrator, not a complete or biologically calibrated brain model.
