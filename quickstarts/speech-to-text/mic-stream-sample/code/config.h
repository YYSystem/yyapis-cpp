#ifndef CONFIG_H
#define CONFIG_H
#include <iostream>
#include <string>
#include <grpcpp/security/credentials.h>

using namespace std;
using grpc::ChannelCredentials;

string getApiKey();
string getStreamingConfig();
string getEncoding();
string getTarget();
shared_ptr<ChannelCredentials> getCredentials();

int getSampleRateHertz();
int getLanguageCode();
int getModel();
bool getEnableWord();
bool getEnableInterimResults();
// long getDeadlineMilliseconds();
int getAudioChannelCount();
int getSegmentationSilenceTimeoutMs();
#endif