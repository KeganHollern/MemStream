BUILD_ARGS ?=

# TODO: implement this into our dockerfile as an arg... (Debug/Release)
# TODO: implement this into our output path `./build/arch/type/memstream.so`
BUILD_TYPE ?= Debug

.PHONY: linux
linux:
	docker build -t memstream/linux $(BUILD_ARGS) .
	docker run --rm -v $(PWD):/host memstream/linux cp -r /src/MemStream/build /host
	docker rmi memstream/linux

.PHONY: windows
windows:
	echo "TODO"
