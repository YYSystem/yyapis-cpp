#include <iostream>
#include <getopt.h>
#include "parse_arguments.h"
#include "config.h"

char* ParseArguments(int argc, char** argv, yysystem::StreamingConfig* config) {
  static struct option long_options[] = {
    { "bitrate", required_argument, nullptr, 'b' }, { nullptr, 0, nullptr, 0}
  };
  config->set_enable_interim_results(getEnableInterimResults());
  config->set_language_code(getLanguageCode());
  config->set_sample_rate_hertz(getSampleRateHertz());
  config->set_model(getModel());
  config->set_enable_word(getEnableWord());
  config->set_audio_channel_count(getAudioChannelCount());
  config->set_segmentation_silence_timeout_ms(getSegmentationSilenceTimeoutMs());
  int opt;
  int option_index = 0;
  while ((opt = getopt_long(argc, argv, "b:", long_options, &option_index)) != -1) {
    std::cout << "opt: " << opt << ", optind: " << optind << ", optarg: " << optarg << std::endl;
    switch(opt) {
      case 'b':
        config->set_sample_rate_hertz(atoi(optarg));
        if (0 == config->sample_rate_hertz()) return nullptr;
        break;
      default:
        return nullptr;
    }
  };
  if (optind == argc) {
    std::cout << "Missing the audio file path" << std::endl;
    return nullptr;
  }
  char* ext = strrchr(argv[optind], '.');
  if (ext == nullptr || 0 == strcasecmp(ext, ".raw")) {
    config->set_encoding("LINEAR16");
  } else if (ext == nullptr || 0 == strcasecmp(ext, ".ulaw")) {
    config->set_encoding("MULAW");
  } else if (ext == nullptr || 0 == strcasecmp(ext, ".flac")) {
    config->set_encoding("FLAC");
  } else if (ext == nullptr || 0 == strcasecmp(ext, ".amr")) {
    config->set_encoding("AMR");
    config->set_sample_rate_hertz(8000);
  } else if (ext == nullptr || 0 == strcasecmp(ext, ".awb")) {
    config->set_encoding("AMR_WB");
    config->set_sample_rate_hertz(16000);
  } else {
    config->set_encoding("LINEAR16");
  }
  return argv[optind];
}