
BLD ?= dbg/clang
SCH ?= prtcl-schemes-iisph-wkbb18
SCENE ?= teapot-slow-fill.json

all: build 

.PHONY: clean-output
clean-output:
	rm -f output/*.vtk

.PHONY: build
build:
	ninja -C bld/$(BLD)

.PHONY: run
run:
	./bld/$(BLD)/schemes/$(SCH) --load-json=share/scenes/$(SCENE)

.PHONY: run-rr
run-rr:
	rr record ./bld/$(BLD)/schemes/$(SCH) --load-json=share/scenes/$(SCENE)

.PHONY: run-debug
run-debug:
	gdb -q ./bld/$(BLD)/schemes/$(SCH) -ex "run --load-json=share/scenes/$(SCENE)"

