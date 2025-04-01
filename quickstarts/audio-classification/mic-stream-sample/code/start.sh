#!/bin/sh

export GRPC_DEFAULT_SSL_ROOTS_FILE_PATH="$PWD/roots.pem"

cmake --preset=default
cmake --build build
./build/audio-classification