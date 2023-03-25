#define NO_AUDIO_EXAMPLES
#include "SimpleTTS.h"
#include "AudioCodecs/CodecMP3Helix.h"
#include "AudioDictionaryURL.h"
#include "AudioTools.h"

const char *url = "https://philippteepunkt.github.io/sequence-me/tts_audio/";

AnalogAudioStream speechOut;
VolumeStream speechVolume(speechOut);
URLStream in("Hivenet","38459950178051317424");
//URLStream in();
AudioDictionaryURL dictionary(in, url, "mp3");
MP3DecoderHelix mp3;
TextToSpeechQueue tts(speechVolume, mp3, dictionary);

void initializeSpeechHandler(Client &net) {
  // setup out
  auto cfg = speechOut.defaultConfig();
  cfg.sample_rate = 44100;
  cfg.channels = 1;
  cfg.bits_per_sample = 32;
  speechOut.begin(cfg);

  //in.setClient(net);

  // setting the volume
  speechVolume.setVolume(10.0);
}

void say(String instruction){
  if (tts.isEmpty()) {
    if(instruction == "BPM"){
     tts.say("BPM"); 
    } else if(instruction == "FREQUENCY"){
     tts.say("frequency");       
    } else if(instruction == "LISTEN"){
     tts.say("LISTEN");
    } else if(instruction == "SAMPLE"){
     tts.say("SAMPLE");
    } else if(instruction == "SENDING"){
     tts.say("SENDING");
    } else if(instruction == "SEQUENCE"){
     tts.say("SEQUENCE");
    } else {
      return;
    }
  }
}