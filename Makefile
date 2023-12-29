BUILD_ARGS ?=

# TODO: implement this into our dockerfile as an arg... (Debug/Release)
# TODO: implement this into our output path `./build/arch/type/memstream.so`
BUILD_TYPE ?= Debug


#TODO: this shit doesn't mount the disk properly on windows
# this makes windows builders need to run this through WSL
#	> wls
#	> make linux
#	> exit
.PHONY: linux
linux:
	docker build -t memstream/linux $(BUILD_ARGS) . && docker run --rm -v ${CURDIR}:/host memstream/linux && docker rmi memstream/linux
