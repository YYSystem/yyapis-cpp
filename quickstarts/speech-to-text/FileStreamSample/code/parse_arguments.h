#ifndef PARSE_ARGUMENTS_H
#define PARSE_ARGUMENTS_H
#include "yysystem.pb.h"
char* ParseArguments(int argc, char** argv, yysystem::StreamingConfig* config);
#endif