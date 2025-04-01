#include <iostream>
#include <fstream>
#include <chrono>
#include <string>
#include <memory>
#include <thread>
#include <portaudio.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/security/credentials.h>
#include "yysystem.audioclassification.grpc.pb.h"
#include "yysystem.audioclassification.pb.h"
#include "config.h"
#include <tuple>


using namespace std;
using chrono::system_clock;
using chrono::seconds;
using chrono::milliseconds;
using grpc::Channel;
using grpc::ClientReaderWriter;
using grpc::ClientReaderWriterInterface;
using grpc::ClientContext;
using grpc::Status;
using grpc::CreateChannel;
using grpc::GoogleDefaultCredentials;
using grpc::InsecureChannelCredentials;
using grpc::SslCredentials;
using grpc::SslCredentialsOptions;
using yysystem::audioclassification::YYAudioClassification;
using yysystem::audioclassification::ClassifyStreamRequest;
using yysystem::audioclassification::ClassifyStreamResponse;
using yysystem::audioclassification::StreamingConfig;
using yysystem::audioclassification::Config;


static void MicrophoneThread(shared_ptr<ClientReaderWriterInterface<ClassifyStreamRequest, ClassifyStreamResponse>> streamer, ClassifyStreamRequest& request, atomic<bool>& stream_complete) {
    PaError err;
    PaStream *stream;
    const size_t chunk_size = 3200;
    const int16_t sample_rate = 16000;
    vector<char> chunk(chunk_size);

    //PortAudioの初期化
    err = Pa_Initialize();
    if(err != paNoError){
        cout << "PortAudio initialization failed: " << Pa_GetErrorText(err) << endl;
        stream_complete = true;
        return;
    }else{
        cout << "Success: PortAudio initialization Succeed" << endl;
    }

    //マイクデバイスのインデックスの取得
    PaDeviceIndex deviceIndex = Pa_GetDefaultInputDevice();
    if(deviceIndex == paNoDevice){
        cout << "Error: No default input device found." << endl;
        stream_complete = true;
        return;
    }else{
        cout << "Success: default input device found." << endl;
    }

    //マイクデバイスの情報の取得
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(deviceIndex);
    if(deviceInfo == nullptr){
        cout << "Error: Could not get info." << endl;
        Pa_Terminate();
        stream_complete = true;
        return;
    }else{
        cout << "Success: Could get info." << endl;
    }

    //マイク入力音声のパラメータ設定
    PaStreamParameters inputParameters;
    inputParameters.device = deviceIndex;
    inputParameters.channelCount = 1;
    inputParameters.sampleFormat = paInt16;
    inputParameters.suggestedLatency = deviceInfo->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = nullptr;

    //入力ストリームを開く
    err = Pa_OpenStream(
        &stream,
        &inputParameters,
        nullptr,
        sample_rate,
        chunk_size / 2,
        paClipOff,
        nullptr,
        &streamer
    );
    if (err != paNoError) {
        cout << "PortAudio stream open error: " << Pa_GetErrorText(err) << Pa_Terminate() << endl;
        stream_complete = true;
        return;
    }else{
        cout << "Success: PortAudio stream open" << endl;
    }

    //ストリーム開始
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        cout << "PortAudio stream start error: " << Pa_GetErrorText(err) << Pa_Terminate() <<endl;
        stream_complete = true;
        return;
    }else{
        cout << "Success: PortAudio stream start" << endl;
        cout << "   IsStreamActive: " << Pa_IsStreamActive(stream) << endl;
        cout << "   GetStreamInfo: " << Pa_GetStreamInfo(stream) << endl;
    }

    //ストリームの読み込みと書き込み
    bool firstMessage = true;
    while(!stream_complete){
        err = Pa_ReadStream(stream, chunk.data(), chunk_size/2);
        if(!streamer -> Write(request)){
            cout << "Error writing to stream." << endl;
            break;
        }

        // 音声データをgRPCストリームに書き込む
        ClassifyStreamRequest request;
        request.set_audiobytes(chunk.data(), chunk_size);
        if (!streamer->Write(request)) {
            cout << "Error writing to stream." << endl;
            break;
        }else{
            if(firstMessage){
                cout << "PortAudio stream writing" << endl;
                cout << "終了するためには、Ctrl+Cを押して下さい" << endl;
                cout << "3秒後に開始します" << endl;
                firstMessage = false;
                this_thread::sleep_for(milliseconds(3000));
            }
        }
        this_thread::sleep_for(milliseconds(100));
    }

    //ストリーム停止とクローズ
    Pa_StopStream(stream);
    cout << "stop stream" << endl;
    Pa_CloseStream(stream);
    cout << "close stream" << endl;
    Pa_Terminate();

    //終了メッセージ
    stream_complete = true;
    cout << "Microphone thread finished." << endl;

    return;
}


class YYSystemClient{
    public:
        YYSystemClient(shared_ptr<Channel> channel): stub_(YYAudioClassification::NewStub(channel)){};
    void ClassifyAudioStream(grpc::ClientContext& context,ClassifyStreamRequest& request) {
        //gRPCストリームを開始
        shared_ptr<ClientReaderWriter<ClassifyStreamRequest, ClassifyStreamResponse>> streamer(stub_->ClassifyStream(&context));
        cout << "start gRPC stream" << endl;

        //サーバーに初期リクエストを送信
        streamer->Write(request);
        cout << "send request" << endl;

        //マイク入力スレッドを開始
        atomic<bool> stream_complete(false);
        thread microphone_thread(&MicrophoneThread, streamer, ref(request), ref(stream_complete));
        cout << "start microphone_thread" << endl;

        //レスポンス結果をコンソールに表示
        ClassifyStreamResponse response;
        while(streamer -> Read(&response)){
            const auto& result = response.custom_results().Get(0);
            cout << "音響分類結果｜クラス名: " << result.class_name() << ", 信頼度: " << result.confidence() << ", ID: " << result.class_id() << endl;
        }
        
        //マイク入力スレッド終了を待機
        microphone_thread.join();
        cout << "have waited to finish microphone_thread" << endl;

        //gRPCストリームを終了
        Status status = streamer->Finish();
        if(!status.ok()){
            cout << "RPC failed: " << status.error_message() << endl;
        }else{
            cout << "RPC succeed" << endl;
        }
    }
    private:
        std::unique_ptr<YYAudioClassification::Stub> stub_;
};


int main(int argc, char** argv){
    ClientContext context;
    context.AddMetadata("x-api-key", getApiKey());
    ClassifyStreamRequest request;
    StreamingConfig* streaming_config = request.mutable_streaming_config();

    //config.cc から設定値を読み込んで StreamingConfig に設定
    Config* custom_config = streaming_config->mutable_custom_config();
    custom_config->set_top_n(getTopN());
    Config* default_config = streaming_config->mutable_default_config();
    default_config->set_top_n(getTopN());
    streaming_config->set_use_default_results(getUse_defalut_results());
    streaming_config->set_endpoint_id(getEndpoint_id());
    streaming_config->set_user_id(getUser_id());

    YYSystemClient client(CreateChannel(getTarget(), getCredentials()));
    client.ClassifyAudioStream(context, request);
    return 0;
}