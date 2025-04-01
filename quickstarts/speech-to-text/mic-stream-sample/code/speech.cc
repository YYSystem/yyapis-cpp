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
#include "yysystem.grpc.pb.h"
#include "yysystem.pb.h"
#include "config.h"


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
using yysystem::YYSpeech;
using yysystem::StreamingConfig;
using yysystem::StreamRequest;
using yysystem::StreamResponse;


//録音された音声データを格納する変数
std::vector<char> recorded_audio_data;


static void MicrophoneThread(shared_ptr<ClientReaderWriterInterface<StreamRequest, StreamResponse>> streamer, StreamRequest& request, atomic<bool>& stream_complete) {
    PaError err;
    PaStream *stream;
    const size_t chunk_size = 3200;
    vector<char> chunk(chunk_size);

    //PortAudioの初期化
    err = Pa_Initialize();
    if(err != paNoError){
        std::cerr << "PortAudio initialization failed: " << Pa_GetErrorText(err) << std::endl;
        stream_complete = true;
        return;
    }else{
        std::cout << "Success: PortAudio initialization Succeed" << std::endl;
    }

    //マイクデバイスのインデックスの取得
    PaDeviceIndex deviceIndex = Pa_GetDefaultInputDevice();
    if(deviceIndex == paNoDevice){
        std::cerr << "Error: No default input device found." << std::endl;
        stream_complete = true;
        return;
    }else{
        std::cout << "Success: default input device found." << std::endl;
    }

    //マイクデバイスの情報の取得
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(deviceIndex);
    if(deviceInfo == nullptr){
        std::cerr << "Error: Could not get info." << std::endl;
        Pa_Terminate();
        stream_complete = true;
        return;
    }else{
        std::cout << "Success: Could get info." << std::endl;
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
        getSampleRateHertz(),
        chunk_size / 2,
        paClipOff,
        nullptr,
        &streamer
    );
    if (err != paNoError) {
        std::cerr << "PortAudio stream open error: " << Pa_GetErrorText(err) << Pa_Terminate();
        stream_complete = true;
        return;
    }else{
        std::cout << "Success: PortAudio stream open" << std::endl;
    }

    //ストリーム開始
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio stream start error: " << Pa_GetErrorText(err) << Pa_Terminate();
        stream_complete = true;
        return;
    }else{
        std::cout << "Success: PortAudio stream start" << std::endl;
        cerr << "   IsStreamActive: " << Pa_IsStreamActive(stream) << endl;
        cerr << "   GetStreamInfo: " << Pa_GetStreamInfo(stream) << endl;
    }

    //ストリームの読み込みと書き込み
    bool firstMessage = true;
    while(!stream_complete){
        err = Pa_ReadStream(stream, chunk.data(), chunk_size/2);
        if(!streamer -> Write(request)){
            cerr << "Error writing to stream." << endl;
            break;
        }else{
            recorded_audio_data.insert(recorded_audio_data.end(), chunk.begin(), chunk.end());
        }

        // 音声データをgRPCストリームに書き込む
        StreamRequest request;
        request.set_audiobytes(chunk.data(), chunk_size);
        if (!streamer->Write(request)) {
            cerr << "Error writing to stream." << endl;
            break;
        }else{
            if(firstMessage){
                cout << "PortAudio stream writing" << endl;
                cout << "終了するためには、Ctrl+Cを押して下さい" << endl;
                firstMessage = false;
            }
            recorded_audio_data.insert(recorded_audio_data.end(), chunk.begin(), chunk.end());
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
    std::cout << "Microphone thread finished." << std::endl;

    return;
}


class YYSystemClient{
    public:
        YYSystemClient(shared_ptr<Channel> channel): stub_(YYSpeech::NewStub(channel)){};
    void RecognizeStream(grpc::ClientContext& context,StreamRequest& request) {
        //gRPCストリームを開始
        shared_ptr<ClientReaderWriter<StreamRequest, StreamResponse>> streamer(stub_->RecognizeStream(&context));
        cout << "start gRPC stream" << endl;

        //サーバーに初期リクエストを送信
        streamer->Write(request);
        cout << "send request" << endl;

        //マイク入力スレッドを開始
        atomic<bool> stream_complete(false);
        thread microphone_thread(&MicrophoneThread, streamer, ref(request), ref(stream_complete));
        cout << "start microphone_thread" << endl;

        //レスポンス結果をコンソールに表示
        StreamResponse response;
        while(streamer -> Read(&response)){
            cout << "音声認識結果: " << response.result().transcript() << endl;
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
        std::unique_ptr<YYSpeech::Stub> stub_;
};


int main(int argc, char** argv){
    ClientContext context;
    context.AddMetadata("x-api-key", getApiKey());
    context.AddMetadata("yyapis-stream-end-timeout-ms", "1000");
    StreamRequest request;
    StreamingConfig* streaming_config = request.mutable_streaming_config();
    YYSystemClient client(CreateChannel(getTarget(), getCredentials()));
    client.RecognizeStream(context, request);
    return 0;
}