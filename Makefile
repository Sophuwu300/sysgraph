ifeq ($(shell command -v cmake), )
COMPCMD = g++ -o build/sysgraph main.cpp
COMPMSG = g++
else
ifeq ($(shell command -v ninja), )
COMPCMD = cmake -S . -B build && cmake --build build
COMPMSG = cmake
else
COMPCMD = cmake -GNinja -S . -B build && cmake --build build
COMPMSG = cmake and ninja
endif
CMAKELISTS = CMakeLists.txt
endif

ifeq ($(shell command -v strip), )
STRIPCMD = echo Strip not found. Binary size may be larger than expected.
else
STRIPCMD = strip --strip-all build/sysgraph
endif
ifeq ($(shell command -v upx), )
UPXCMD = echo UPX not found. Binary size may be larger than expected.
else
UPXCMD = upx --best build/sysgraph
endif

build: main.cpp $(CMAKELISTS)
	@echo "Compiling with $(COMPMSG)"
	@sleep 2
	@mkdir -p build
	@$(COMPCMD) && $(STRIPCMD) && $(UPXCMD) && echo "Done! Run make test or make install then make clean" || echo "Failed!"

clean: build
	@rm -rf build
	@echo "Cleaned up build directory."

install: build/sysgraph
	@echo Installing to /usr/local/bin/sysgraph
	@sudo install build/sysgraph /usr/local/bin/sysgraph && echo "Done!" || echo "Failed! Try running as root."

test: build/sysgraph
	@echo "Running sysgraph"
	@./build/sysgraph