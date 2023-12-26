FROM --platform=aarch64 ubuntu:latest AS arm64

ENV TARGET_ARCH=aarch64

RUN arch=$(uname -m) && echo "Architecture: $arch" && if [ "$arch" != "${TARGET_ARCH}" ]; then exit 1; fi

# Install GCC, CMake, and other necessary tools
RUN apt-get update && \
    apt-get install -y wget gcc g++ make ninja-build

# CMAKE version
ENV CMAKE_VERSION=3.28.1

# Download and install CMake
RUN wget https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-Linux-${TARGET_ARCH}.sh && \
    sh cmake-${CMAKE_VERSION}-Linux-${TARGET_ARCH}.sh --prefix=/usr/local --exclude-subdir && \
    rm cmake-${CMAKE_VERSION}-Linux-${TARGET_ARCH}.sh

# Copy your project files into the container
WORKDIR /src/MemStream
COPY . .

# generate via ninja
RUN cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja -S /src/MemStream -B /src/MemStream/cmake-build-docker
RUN cmake --build /src/MemStream/cmake-build-docker --target MemStream -j 6

FROM --platform=x86_64 ubuntu:latest AS amd64

ENV TARGET_ARCH=x86_64

RUN arch=$(uname -m) && echo "Architecture: $arch" && if [ "$arch" != "${TARGET_ARCH}" ]; then exit 1; fi

# Install GCC, CMake, and other necessary tools
RUN apt-get update && \
    apt-get install -y wget gcc g++ make ninja-build

# CMAKE version
ENV CMAKE_VERSION=3.28.1

# Download and install CMake
RUN wget https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-Linux-${TARGET_ARCH}.sh && \
    sh cmake-${CMAKE_VERSION}-Linux-${TARGET_ARCH}.sh --prefix=/usr/local --exclude-subdir && \
    rm cmake-${CMAKE_VERSION}-Linux-${TARGET_ARCH}.sh

# Copy your project files into the container
WORKDIR /src/MemStream
COPY . .

# generate via ninja
RUN cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja -S /src/MemStream -B /src/MemStream/cmake-build-docker
RUN cmake --build /src/MemStream/cmake-build-docker --target MemStream -j 6

FROM alpine:latest AS output
WORKDIR /src/MemStream/build
COPY --from=arm64 /src/MemStream/build /src/MemStream/build
COPY --from=amd64 /src/MemStream/build /src/MemStream/build
