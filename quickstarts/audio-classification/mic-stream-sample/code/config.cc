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
static const string apiKey = "YOUR API KEY";
static const int topN = 1;
static const bool use_default_results = true;
static const string user_id = "YOUR_USER_ID";
static const string endpoint_id = "YOUR ENDPOINT_ID";
enum Endpoint { Production };
static const vector<string> endpoints = {
  "api-grpc-2.yysystem2021.com"
};
static const int endpoint_idx = Endpoint::Production;
int port = 443;


//設定を返す
string getApiKey(){return apiKey;}
string getStreamingConfig(){
    static const json streamingConfig_json = {
        {"custom_config", {{"top_n", topN}}},
        {"default_config", {{"top_n", topN}}},
        {"use_default_results", use_default_results},
        {"user_id", user_id},
        {"endpoint_id", endpoint_id}
    };
    static const string streamingConfig = streamingConfig_json.dump();
    return streamingConfig;
}
int getTopN(){return topN;}
bool getUse_defalut_results(){return use_default_results;}
string getUser_id(){return user_id;}
string getEndpoint_id(){return endpoint_id;}
string getTarget () {
    string colon = ":";
    string port_str = to_string( port );
    string target = endpoints[ endpoint_idx ] + colon + port_str;
    printf( "target: %s\n", target.c_str() );
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