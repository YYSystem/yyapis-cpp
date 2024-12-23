#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <grpcpp/grpcpp.h>
#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/security/credentials.h>
#include "config.h"

using namespace std;
using nlohmann::json;
using grpc::ChannelCredentials;
// using grpc::GoogleDefaultCredentials;
using grpc::InsecureChannelCredentials;
using grpc::SslCredentials;
using grpc::SslCredentialsOptions;

static const string apiKey = "PPWx81yiL9BCeZdoWpxjKjcL9LoSpsvz";
static const string encoding = "LINEAR16";
static const int sampleRateHertz = 16000;
// static const int sampleRateHertz = 48000;
// static const int languageCode = 0;
// static const int languageCode = 1;
// static const int languageCode = 2;
static const int languageCode = 4;
// static const int model = 0;
// static const int model = 1;
// static const int model = 2;
// static const int model = 3;
// static const int model = 4;
static const int model = 10;
// static const int model = 11;
static const bool enableWord = true;
static const bool enableInterimResults = true;
// static const int audioChannelCount = 1;
static const int audioChannelCount = 2;
static const long deadlineMilliseconds = 10*1000;
enum Endpoint { Local, Docker, Test, Production };
static const vector<string> endpoints = {
  "localhost",
  "host.docker.internal",
  "api-test.yysystem2021.com",
  "api-grpc.yysystem2021.com" };
// static const int endpoint_idx = Endpoint::Production;
// static const int endpoint_idx = Endpoint::Test;
static const int endpoint_idx = Endpoint::Local;
int port = 50051;
static const int segmentationSilenceTimeoutMs = 100;

string getApiKey () { return apiKey; }
string getStreamingConfig () {
  static const json streamingConfig_json = {
    { "encoding", encoding },
    { "sample_rate_hertz", sampleRateHertz },
    { "language_code", languageCode },
    { "model", model },
    { "enable_word", enableWord },
    { "enable_interim_results", enableInterimResults },
    { "audio_channel_count", audioChannelCount },
    { "segmentation_silence_timeout_ms", segmentationSilenceTimeoutMs}
  };
  static const string streamingConfig = streamingConfig_json.dump();
  return streamingConfig;
}
string getEncoding() { return encoding; }
int getSampleRateHertz() { return sampleRateHertz; }
int getLanguageCode() { return languageCode; }
int getModel() { return model; }
bool getEnableWord() { return enableWord; }
bool getEnableInterimResults() { return enableInterimResults; }
long getDeadlineMilliseconds() { return deadlineMilliseconds; }
int getAudioChannelCount() { return audioChannelCount; }
int getSegmentationSilenceTimeoutMs() { return segmentationSilenceTimeoutMs; }
string getTarget () {
  string colon = ":";
  string port_str = to_string( port );
  string target = endpoints[ endpoint_idx ] + colon + port_str;
  return target;
}
shared_ptr<ChannelCredentials> getCredentials () {
  switch ( endpoint_idx ) {
    case Endpoint::Production:
    case Endpoint::Test:
      return SslCredentials(SslCredentialsOptions());
    case Endpoint::Docker:
    case Endpoint::Local:
    default:
      return InsecureChannelCredentials();
  }
}