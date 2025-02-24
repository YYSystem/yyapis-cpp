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
using yysystem::audioclassification::YYAudioClassification; // 名前空間の変更
using yysystem::audioclassification::ClassifyStreamRequest;  // メッセージ型の変更
using yysystem::audioclassification::ClassifyStreamResponse; // メッセージ型の変更
using yysystem::audioclassification::StreamingConfig;       // メッセージ型の変更
using yysystem::audioclassification::Config;                // メッセージ型の変更


#define SAMPLE_RATE (44100)
#define FRAMES_PER_BUFFER (512)
#define NUM_CHANNELS (1)


static void MicrophoneThreadMain(shared_ptr<ClientReaderWriterInterface<ClassifyStreamRequest, ClassifyStreamResponse>> streamer, ClassifyStreamRequest& request, atomic<bool>& stream_complete) {
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
        deviceInfo->defaultSampleRate,
        chunk_size / 2,
        paClipOff,
        nullptr,
        nullptr  
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
    }
    while(!stream_complete){
        err = Pa_ReadStream(stream, &chunk[0], chunk_size / 2);
        cerr << "IsStreamActive: " << Pa_IsStreamActive(stream) << endl;
        cerr << "GetStreamInfo: " << Pa_GetStreamInfo(stream) << endl;
        if(err != paNoError){
          cerr << "PortAudio stream read error: " << Pa_GetErrorText(err) << endl;
          break;
        }
        request.set_audiobytes(&chunk[0], chunk_size);
        if(!streamer->Write(request)){
          cerr << "Error writing to stream: " << endl;
          break;
        }
        this_thread::sleep_for(milliseconds(1000));
    }

    //ストリーム停止とクローズ
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();

    //終了メッセージ
    stream_complete = true;
    std::cout << "Microphone thread finished." << std::endl;

    return;
}


class YYSystemClient{
    public:
        YYSystemClient(shared_ptr<Channel> channel): stub_(YYAudioClassification::NewStub(channel)){};
        void ClassifyAudioStream(grpc::ClientContext& context, ClassifyStreamRequest& request){
            shared_ptr<ClientReaderWriter<ClassifyStreamRequest, ClassifyStreamResponse>> streamer(stub_->ClassifyStream(&context));
            streamer->Write(request);
            atomic<bool> stream_complete(false);
            thread microphone_thread(&MicrophoneThreadMain, streamer, ref(request), ref(stream_complete));
            cout << "microphone_thread" << endl;
            ClassifyStreamResponse response;
            if (streamer->Read(&response)) {
                // ClassifyStreamResponse(response);
                while (streamer->Read(&response)) {
                    // ClassifyStreamResponse(response);
                }
            }
            
            cout << "Read stream ended" << endl;
            cout << "microphpone_thread.join()" << endl;
            microphone_thread.join();
            cout << "streamer Finish" << endl;
            Status status = streamer->Finish();
            cout << "thread joined" << endl;
            if (!status.ok())
            {
                cerr << "RPC failed " << status.error_code() << ": " << status.error_message() << endl;
            }
        }
        private:
            std::unique_ptr<YYAudioClassification::Stub> stub_;
};


// void ShowResponse(string response){
//     std::cout << "response: " << response <<std::endl;
// }


int main(int argc, char** argv){
    ClientContext context;
    context.AddMetadata("x-api-key", getApiKey());
    context.AddMetadata("yyapis-stream-end-timeout-ms", "1000");
    ClassifyStreamRequest request;
    StreamingConfig* streaming_config = request.mutable_streaming_config();
    YYSystemClient client(CreateChannel(getTarget(), getCredentials()));
    client.ClassifyAudioStream(context, request);
    return 0;
}