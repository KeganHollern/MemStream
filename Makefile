BUILD_ARGS ?= ""

.PHONY: linux
linux:
	docker build -t memstream/linux $(BUILD_ARGS) .
	docker run --rm -v $(PWD):/host memstream/linux cp -r /src/MemStream/build /host
	docker rmi memstream/linux