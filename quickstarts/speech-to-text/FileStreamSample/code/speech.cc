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

#include "parse_arguments.h"
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


static const char kUsage[] = 
  "Usage:\n"
  "    Speech "
  "[--bitrate N] audio.(raw|uraw|flac|amr|awb)\n";


static void MicrophoneThreadMain(shared_ptr<ClientReaderWriterInterface<StreamRequest, StreamResponse>> streamer, StreamRequest& request, atomic<bool>& stream_complete){
  PaError err;
  PaStream *stream;
  const size_t chunk_size = 10000000;
  vector<char> chunk(chunk_size);

  //PortAudioの初期化
  err = Pa_Initialize();
  if(err != paNoError){
      std::cerr << "PortAudio initialization failed: " << Pa_GetErrorText(err) << std::endl;
      return;
  }

  //マイクデバイスのインデックスの取得
  PaDeviceIndex deviceIndex = Pa_GetDefaultInputDevice();
  if(deviceIndex == paNoDevice){
    cerr << "Error: No default input device found." << endl;
    stream_complete = true;
    return;
  }

  //マイクデバイスの情報の取得
  const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(deviceIndex);
  if(deviceInfo == nullptr){
    cerr << "Error: Could not get info." << endl;
    Pa_Terminate();
    stream_complete = true;
    return;
  }else{
    // std::cerr << "defaultLowInputLatency: " << deviceInfo->defaultLowInputLatency << std::endl;
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
  }

  //ストリーム開始
  err = Pa_StartStream(stream);
  if (err != paNoError) {
    std::cerr << "PortAudio stream start error: " << Pa_GetErrorText(err) << Pa_Terminate();
    stream_complete = true;
    return;
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
  streamer->WritesDone();
  stream_complete = true;
  cout << "Microphone thread finished." << endl;
}
//   ClientReaderWriterInterface<StreamRequest, StreamResponse>* streamer,
//   char* file_path
// ) {
//   StreamRequest request;
//   ifstream file_stream(file_path);
//   const size_t chunk_size = 3200;
//   vector<char> chunk(chunk_size);
//   vector<char> silence(chunk_size, 0);
//   int silence_counter = 0;

//   if (!file_stream.is_open()) {
//     std::cerr << "Failed to open file: " << file_path << std::endl;
//     return;
//   }

//   while (true)
//   {
//     streamsize bytes_read = file_stream.rdbuf()->sgetn(&chunk[0], chunk.size());
//     if (bytes_read > 0) {
//       request.set_audiobytes(&chunk[0], bytes_read);
//       cout << "Sending " << bytes_read << " bytes." << endl;
//       streamer->Write(request);
//     } else {
//         cout << "Writes done." << endl;
//         bool isWriteDone = streamer->WritesDone();
//         cout << "isWriteDone " << std::boolalpha << isWriteDone << endl;
//         break;
//     }
//     this_thread::sleep_for(milliseconds(100));
//   }


void ShowResponse(string response){
  cout << "response: " << response <<endl;
}

class YYSystemClient {
  public:
    YYSystemClient(shared_ptr<Channel> channel): stub_(YYSpeech::NewStub(channel)) {};
    void RecognizeStream(grpc::ClientContext& context, StreamRequest& request) {
      shared_ptr<ClientReaderWriter<StreamRequest, StreamResponse>> streamer(stub_->RecognizeStream(&context));
      streamer->Write(request);
      atomic<bool> stream_complete(false);
      thread microphone_thread(&MicrophoneThreadMain, streamer, ref(request), ref(stream_complete));
      cout << "microphone_thread" << endl;
      StreamResponse response;
      if (streamer->Read(&response))
      {
        if (response.has_error())
        {
          cerr << "error: {" << endl;
          cerr << "  code: " << response.error().code() << "," <<endl;
          cerr << "  message: " << response.error().message() << "," << endl;
          cerr << "  details: " << response.error().details() << endl;
          cerr << "}" << endl;
        }
        ShowResponse(response.result().transcript());
        while (streamer->Read(&response))
        {
          cout << "response event" << endl;
          if (response.has_error()) {
            cerr << "error: {" << endl;
            cerr << "  code: " << response.error().code() << "," <<endl;
            cerr << "  message: " << response.error().message() << "," << endl;
            cerr << "  details: " << response.error().details() << endl;
            cerr << "}" << endl;
            break;
          }
          if (response.result().is_final())
          {
            cout << "final " << response.result().is_final() << endl;
            for(int r = 0; r < response.result().words_size(); ++r) {
              const auto& wordInfo = response.result().words(r);
              cout << "response.result.words[" << r << "].word: "<< wordInfo.word() << endl;
            }
          }
          if (request.streaming_config().enable_interim_results() && !response.result().is_final())
          {
            cout << "Interim result: response.result.transcript: " << response.result().transcript() << endl;
          }
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
    std::unique_ptr<YYSpeech::Stub> stub_;
};


int main(int argc, char** argv)
{
  ClientContext context;
  context.AddMetadata("x-api-key", getApiKey());
  context.AddMetadata("yyapis-stream-end-timeout-ms", "1000"); // 1 second
  StreamRequest request;
  StreamingConfig* streaming_config = request.mutable_streaming_config();
  // char* file_path = ParseArguments(argc, argv, streaming_config);
  // if (nullptr == file_path)
  // {
  //   cerr << kUsage;
  //   return -1;
  // }
  YYSystemClient client(CreateChannel(getTarget(), getCredentials()));
  client.RecognizeStream(context, request);
  return 0;
}