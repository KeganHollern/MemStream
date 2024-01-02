BUILD_ARGS ?=

# Build type can be set as Debug/Release
# TODO: implement this into our output path `./build/arch/type/memstream.so`
BUILD_TYPE ?= Debug

# Check if Docker is running
DOCKER_CHECK := $(shell command -v docker && docker info > /dev/null 2>&1)


#TODO: this shit doesn't mount the disk properly on windows
# this makes windows builders need to run this through WSL
#      > wls
#      > make linux
#      > exit

.PHONY: linux
linux:
ifeq ($(DOCKER_CHECK),)
	@echo "Docker is not running. Please start Docker and try again."
else
	@echo "Building for Linux..."
	docker build -t memstream/linux $(BUILD_ARGS) .
	docker run --rm -v ${CURDIR}:/host memstream/linux
	docker rmi memstream/linux
endif
