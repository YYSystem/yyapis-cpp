#!/bin/sh

export GRPC_DEFAULT_SSL_ROOTS_FILE_PATH="$PWD/roots.pem"

cmake --preset=default
cmake --build build
./build/Speech ./resources/add_1000ms_to_xkonnnichiwa.wav