FROM ubuntu:24.04

RUN apt-get update && \
    apt-get install -y \
    wget \
    build-essential \
    cmake \
    libgtest-dev

WORKDIR /work

COPY . .