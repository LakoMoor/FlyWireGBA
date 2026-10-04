# Contributing and releases

Build with Python 3, Clang with the ARM backend, LLD and llvm-objcopy. On Ubuntu:

```sh
sudo apt-get install clang lld llvm cmake build-essential pkg-config zlib1g-dev libedit-dev python3-pil
make all test
bash tools/ci_check.sh
```

CI builds the actual cartridge, runs ASan/UBSan simulation tests, validates the
connectome provenance, all model frames/joint limits, and cartridge header, and runs the ROM in a checksum-pinned mGBA core.
Candidate ROMs, archives, checksums and screenshots are retained as CI artifacts.

To publish a new version, update `VERSION` and add the matching section to
`CHANGELOG.md`, then commit those changes and push a version tag:

```sh
git tag -a v0.1.2 -m 'FlyWireGBA v0.1.2'
git push origin v0.1.2
```

The Release workflow rebuilds and tests the tag before publishing the `.gba`,
ZIP, `SHA256SUMS` and manifest. Tag and `VERSION` must agree. `vX.Y.Z-rc.N`,
`-beta.N` and `-alpha.N` tags create prereleases. Publishing uses the repository's
automatic `GITHUB_TOKEN`; no personal token or external service is needed.

A failed build never publishes a release. Keep version tags immutable; fix a
failed tagged release by preparing a new version rather than moving its tag.

Original code/artwork is MIT. The FlyWire-derived data and ROMs containing it
retain CC BY-NC 4.0 terms. Body graphics derive from flybody under Apache-2.0; preserve THIRD_PARTY_NOTICES.md and licenses/ in distributions.
