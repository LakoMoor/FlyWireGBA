PYTHON ?= python3
CC ?= cc
CORE = src/sim.c src/render.c src/connectome.c src/palette.c src/sprites.S
.PHONY: all test preview release clean
all:
	$(PYTHON) tools/build.py
test:
	mkdir -p build
	$(CC) -O1 -g -fsanitize=address,undefined -Isrc tests/test_sim.c $(CORE) -o build/test_sim
	build/test_sim
	$(PYTHON) tests/test_data.py
	$(PYTHON) tests/test_body.py
preview:
	mkdir -p build
	$(CC) -O2 -Isrc tools/preview.c $(CORE) -o build/preview
	build/preview
release: all test
	$(PYTHON) tools/package_release.py
clean:
	rm -rf build
