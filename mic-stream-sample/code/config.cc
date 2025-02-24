//必要なライブラリを準備する
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <grpcpp/grpcpp.h>
#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/security/credentials.h>
#include "config.h"


//命令の名前を省略する
using namespace std;
using nlohmann::json;
using grpc::ChannelCredentials;
using grpc::InsecureChannelCredentials;
using grpc::SslCredentials;
using grpc::SslCredentialsOptions;


//設定を保存する
static const string apiKey = "xRB2HzIYUqZ8Et9ibX2YUOt74BxFeLPE";
static const string encoding = "LINEAR16";
static const int sampleRateHertz = 16000;
static const int languageCode = 4; //日本語
static const int model = 10; //後で調べる
static const bool enableWord = false; //後で調べる
static const bool enableInterimResults = true; //後で調べる
static const int audioChannelCount = 1;
enum Endpoint{Production};
static const vector<string> endpoints = {
    "api-grpc-2.yysystem2021.com"
};
static const int endpoint_idx = Endpoint::Production;
int port = 443;
static const int segmentationSilenceTimeoutMs = 100;


//設定を返す
string getApiKey(){return apiKey;}
string getStreamingConfig(){
    static const json streamingConfig_json = {
        {"encoding", encoding},
        {"sample_rate_hertz", sampleRateHertz},
        {"language_code", languageCode},
        {"model", model},
        {"enable_word", enableWord},
        {"enable_interim_results", enableInterimResults},
        {"audio_channel_count", audioChannelCount},
        {"segmentation_silence_timeout_ms", segmentationSilenceTimeoutMs}
    };
    static const string streamingConfig = streamingConfig_json.dump();
    return streamingConfig;
}
string getEncoding(){return encoding;}
int getSampleRateHertz(){return sampleRateHertz;}
int getLanguageCode(){return languageCode;}
int getModel(){return model;}
bool getEnableWord(){return enableWord;}
bool getEnableInterimResults(){return enableInterimResults;}
int getAudioChannelCount(){return audioChannelCount;}
int segmentationSilenceTimeoutMS(){return segmentationSilenceTimeoutMs;}
string getTarget(){
    string colon = ":";
    string port_str = to_string(port);
    string target = endpoints[endpoint_idx] + colon + port_str;
    printf("target: %s¥n", target.c_str());
    return target;
}
//gRPC通信で使用する認証情報を取得する関数を定義する
shared_ptr<ChannelCredentials>
getCredentials(){
    switch(endpoint_idx){
        case Endpoint::Production:
        default:
            return SslCredentials(SslCredentialsOptions());
    }
}