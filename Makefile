all: rebuild
BINARY_NAME_SOUND := ./build/sgsound_test

install:
	@cmake --install build

rebuild: clean
	@mkdir build
	@cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -B build .
	@cmake --build build
	@$(MAKE) run

clean:
	@rm -rf build

run:
	@$(BINARY_NAME_SOUND)

debug:
	@gdb $(BINARY_NAME)



