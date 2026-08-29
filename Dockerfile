FROM ubuntu:24.04

RUN apt-get update && \
    apt-get install -y \
    wget \
    gnupg \
    lsb-release \
    software-properties-common \
    build-essential \
    clang \
    cmake \
    lcov \
    libgtest-dev \
    libboost-dev \
    libmimalloc-dev

# Ubuntu's own clang-format/clang-tidy are LLVM 18, too old for this repo's
# .clang-format -- pull LLVM 20 from apt.llvm.org directly (not via llvm.sh,
# whose distro-version check breaks on new Ubuntu point releases).
RUN wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | gpg --dearmor -o /etc/apt/trusted.gpg.d/apt.llvm.org.gpg && \
    echo "deb http://apt.llvm.org/noble/ llvm-toolchain-noble-20 main" > /etc/apt/sources.list.d/llvm.list && \
    apt-get update && \
    apt-get install -y clang-format-20 clang-tidy-20 && \
    ln -sf /usr/bin/clang-format-20 /usr/bin/clang-format && \
    ln -sf /usr/bin/clang-tidy-20 /usr/bin/clang-tidy

WORKDIR /work

COPY . .