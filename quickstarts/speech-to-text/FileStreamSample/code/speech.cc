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


static void MicrophoneThreadMain(
  ClientReaderWriterInterface<StreamRequest, StreamResponse>* streamer,
  char* file_path
) {
  StreamRequest request;
  ifstream file_stream(file_path);
  const size_t chunk_size = 3200;
  vector<char> chunk(chunk_size);
  vector<char> silence(chunk_size, 0);
  int silence_counter = 0;

  if (!file_stream.is_open()) {
    std::cerr << "Failed to open file: " << file_path << std::endl;
    return;
  }

  while (true)
  {
    streamsize bytes_read = file_stream.rdbuf()->sgetn(&chunk[0], chunk.size());
    if (bytes_read > 0) {
      request.set_audiobytes(&chunk[0], bytes_read);
      // cout << "Sending " << bytes_read << " bytes." << endl;
      streamer->Write(request);
    } else {
        cout << "Writes done." << endl;
        bool isWriteDone = streamer->WritesDone();
        cout << "isWriteDone " << std::boolalpha << isWriteDone << endl;
        break;
    }
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
        if (response.has_error()) { showErrorResponse(response); }
        else { showResponse(response); }
        while (streamer->Read(&response))
        {
          if (response.has_error())
          {
            showErrorResponse(response);
            break;
          }
          showResponse(response);
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
    void showErrorResponse(StreamResponse& response)
    {
      cerr << "Response Error: " << response.error().DebugString() << endl;
    }
    void showResponse(StreamResponse& response)
    {
      if (response.result().is_final())
      {
        cout << "Final Result: " << response.result().transcript() << endl;
      }
      else
      {
        cout << "Interim Result: " << response.result().transcript() << endl;
      }
    }
};


int main(int argc, char** argv)
{
  ClientContext context;
  context.AddMetadata("x-api-key", getApiKey());
  context.AddMetadata("yyapis-stream-end-timeout-ms", "1000"); // 1 second
  // system_clock::time_point deadline = system_clock::now() + chrono::milliseconds(getDeadlineMilliseconds());
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