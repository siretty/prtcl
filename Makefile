
BLD ?= dbg/clang
SCH ?= prtcl-schemes-iisph-wkbb18
SCENE ?= teapot-slow-fill.json

all: build 

.PHONY: clean-output
clean-output:
	rm -f output/*.vtk

.PHONY: build
build:
	mkdir -p bld/$(BLD)
	ninja -C bld/$(BLD)

.PHONY: run
run:
	mkdir -p output
	./bld/$(BLD)/schemes/$(SCH) --load-json=share/scenes/$(SCENE)

.PHONY: run-rr
run-rr:
	rr record ./bld/$(BLD)/schemes/$(SCH) --load-json=share/scenes/$(SCENE)

.PHONY: run-debug
run-debug:
	gdb -q ./bld/$(BLD)/schemes/$(SCH) -ex "run --load-json=share/scenes/$(SCENE)"

