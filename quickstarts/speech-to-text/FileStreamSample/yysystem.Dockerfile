FROM ubuntu:latest

ARG CMAKE_VERSION=3.22.1
# if arm set below arg to 1
ARG VCPKG_FORCE_SYSTEM_BINARIES

ARG HOME=/usr/src
ARG MY_INSTALL_DIR=$HOME/.local
ARG DEBIAN_FRONTEND=noninteractive

ENV PATH $MY_INSTALL_DIR/bin:$PATH
ENV VCPKG_FORCE_SYSTEM_BINARIES $VCPKG_FORCE_SYSTEM_BINARIES

RUN apt-get update \
 && apt-get install -y \
      build-essential libssl-dev git wget \
      curl zip unzip tar ninja-build \
      autoconf libtool pkg-config

WORKDIR $MY_INSTALL_DIR
RUN wget https://github.com/Kitware/CMake/releases/download/v$CMAKE_VERSION/cmake-$CMAKE_VERSION.tar.gz \
 && tar xvf cmake-$CMAKE_VERSION.tar.gz

WORKDIR $MY_INSTALL_DIR/cmake-$CMAKE_VERSION
RUN ./bootstrap \
 && make \
 && make install

WORKDIR $HOME
RUN git clone https://github.com/microsoft/vcpkg

WORKDIR $HOME/vcpkg
RUN ./bootstrap-vcpkg.sh \
 && ./vcpkg install grpc nlohmann-json

COPY ./code/ $HOME/code/

WORKDIR $HOME/code/yy-sample
RUN mkdir -p build && cmake \
    -DCMAKE_PREFIX_PATH=$MY_INSTALL_DIR \
    -S . \
    -B build \
 && cmake --build build
CMD ["/bin/sh", "-c", "./build/Speech ./resources/sample.wav"]