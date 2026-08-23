FROM ubuntu:24.04

RUN apt-get update && \
    apt-get install -y \
    wget \
    build-essential \
    clang \
    cmake \
    libgtest-dev \
    libboost-dev \
    libmimalloc-dev \
    curl

# Node.js -- only needed to build example-napi/ (cmake-js drives that CMake
# configure/build from npm, see example-napi/package.json); the plain native
# build (test-ppv-lab etc.) doesn't touch this.
RUN curl -fsSL https://deb.nodesource.com/setup_22.x | bash - && \
    apt-get install -y nodejs

WORKDIR /work

COPY . .