#ifndef CONFIG_H
#define CONFIG_H
#include <iostream>
#include <string>
#include <grpcpp/security/credentials.h>

using namespace std;
using grpc::ChannelCredentials;

string getApiKey();
string getStreamingConfig();
int getTopN();
bool getUse_defalut_results();
string getUser_id();
string getEndpoint_id();
string getTarget();
shared_ptr<ChannelCredentials> getCredentials();
#endif