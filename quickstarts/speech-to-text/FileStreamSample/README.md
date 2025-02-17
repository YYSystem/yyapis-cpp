# FileStreamSample

This is a sample project to demonstrate how to use `yyapis-cpp` to send audio data to `Speech-to-Text` service.

## Prerequisites

- [CMake](https://cmake.org/download/)
- [Ninja](https://github.com/ninja-build/ninja)

## Build

Clone `yyapis-cpp` project from yyapis github repository:

```bash
git clone https://github.com/YYSystem/yyapis-cpp.git
```

Move to FileStreamSmaple directory:

```bash
cd yyapis-cpp/speech-to-text/FileStreamSmaple
```

Download `yysystem.proto` from [YYAPIs Developer Console](https://api-web.yysystem2021.com) and put it in `yyapis-cpp/speech-to-text/FileStreamSmaple/protos/`

The directory structure should look like this:

```bash
yyapis-cpp/
  speech-to-text/
    FileStreamSmaple/
      protos/
        yysystem.proto
      codes/
        CmakeLists.txt
        ...
```

Back to `yyapis-cpp/speech-to-text/FileStreamSmaple` directory, and clone `vcpkg` from github repository:

```bash
git clone https://github.com/microsoft/vcpkg
```

The directory structure should be like this:

```bash
yyapis-cpp/
  speech-to-text/
    FileStreamSmaple/
      protos/yysystem.proto
      codes/
        CmakeLists.txt
        ...
      vcpkg/
        bootstrap-vcpkg.sh
        ...
```

Move to `vcpkg` directory:

```bash
cd vcpkg
```

Run `bootstrap-vcpkg.sh` script:

```bash
./bootstrap-vcpkg.sh
```

Move to `yyapis-cpp/speech-to-text/FileStreamSmaple/codes` directory, and run `vcpkg` to install dependencies:

```bash
cmake --preset=default
```

Generate build files:

```bash
cmake --build build
```

Run the sample:

```bash
GRPC_DEFAULT_SSL_ROOTS_FILE_PATH="$PWD/roots.pem"
```

```bash
./build/FileStreamSample ./resources/sample.wav
```
