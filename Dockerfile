FROM ubuntu:24.04

RUN apt-get update && \
    apt-get install -y \
    wget \
    build-essential \
    clang \
    cmake \
    libgtest-dev \
    libboost-dev \
    libmimalloc-dev

WORKDIR /work

COPY . .