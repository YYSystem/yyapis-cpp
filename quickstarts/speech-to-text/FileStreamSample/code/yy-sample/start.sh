#!/bin/sh

export GRPC_DEFAULT_SSL_ROOTS_FILE_PATH="$PWD/roots.pem"

cmake -S . -B build
cmake --build build
./build/Speech ./resources/sample.wav