
#include <iostream>
#include <fstream>
#include <chrono>
#include <string>
#include <memory>
#include <thread>

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


// static void MicrophoneThreadMain(
//   ClientReaderWriterInterface<StreamRequest, StreamResponse>* streamer,
//   char* file_path
// ) {
//   StreamRequest request;
//   ifstream file_stream(file_path);
//   const size_t chunk_size = 3200;
//   // const size_t chunk_size = 8192;
//   vector<char> chunk(chunk_size);
//   while (true)
//   {
//     streamsize bytes_read = file_stream.rdbuf()->sgetn(&chunk[0], chunk.size());
//     request.set_audiobytes(&chunk[0], bytes_read);
//     cout << "Sending " << bytes_read << " bytes." << endl;
//     streamer->Write(request);
//     if (bytes_read < chunk.size())
//     {
//       cout << "Writes done." << endl;
//       bool isWriteDone = streamer->WritesDone();
//       cout << "isWriteDone " << std::boolalpha << isWriteDone << endl;
//       break;
//     }
//     // this_thread::sleep_for(milliseconds(240));
//     this_thread::sleep_for(milliseconds(100));
//   }
// }

static void MicrophoneThreadMain(
  ClientReaderWriterInterface<StreamRequest, StreamResponse>* streamer,
  char* file_path
) {
  StreamRequest request;
  ifstream file_stream(file_path);
  const size_t chunk_size = 3200;
  // const size_t chunk_size = 8192;
  vector<char> chunk(chunk_size);
  vector<char> silence(chunk_size, 0);
  int silence_counter = 0;

  while (true)
  {
    streamsize bytes_read = file_stream.rdbuf()->sgetn(&chunk[0], chunk.size());
    if (bytes_read > 0) {
      request.set_audiobytes(&chunk[0], bytes_read);
      cout << "Sending " << bytes_read << " bytes." << endl;
      streamer->Write(request);
    } else {
      // if (silence_counter == 10) {
      //   StreamRequest configRequest;
      //   configRequest.mutable_streaming_config()->set_encoding("LINEAR16");
      //   cout << "Sending StreamingConfig " << endl;
      //   streamer->Write(configRequest);
      // }
      // if (silence_counter < 30) {
      //   request.set_audiobytes(&silence[0], silence.size());
      //   cout << "Sending silence " << silence.size() << " bytes." << endl;
      //   streamer->Write(request);
      //   silence_counter++;
      // } else {
        cout << "Writes done." << endl;
        bool isWriteDone = streamer->WritesDone();
        cout << "isWriteDone " << std::boolalpha << isWriteDone << endl;
        break;
      // }
    }
    // this_thread::sleep_for(milliseconds(240));
    this_thread::sleep_for(milliseconds(100));
  }
}


class YYSystemClient {
  public:
    YYSystemClient(shared_ptr<Channel> channel): stub_(YYSpeech::NewStub(channel)) {};
    void RecognizeStream(grpc::ClientContext& context, StreamRequest& request, char* file_path) {
      shared_ptr<ClientReaderWriter<StreamRequest, StreamResponse>> streamer(stub_->RecognizeStream(&context));
      streamer->Write(request);
      thread microphone_thread(&MicrophoneThreadMain, streamer.get(), file_path);
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
            cout << "response.result.transcript: " << response.result().transcript() << endl;
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
  system_clock::time_point deadline = system_clock::now() + chrono::milliseconds(getDeadlineMilliseconds());
  // context.set_deadline(deadline);
  StreamRequest request;
  StreamingConfig* streaming_config = request.mutable_streaming_config();
  char* file_path = ParseArguments(argc, argv, streaming_config);
  if (nullptr == file_path)
  {
    cerr << kUsage;
    return -1;
  }
  YYSystemClient client(CreateChannel(getTarget(), getCredentials()));
  client.RecognizeStream(context, request, file_path);
  return 0;
}