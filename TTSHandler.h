#define NO_AUDIO_EXAMPLES
#include "SimpleTTS.h"
#include "AudioCodecs/CodecMP3Helix.h"
#include "AudioDictionaryURL.h"
#include "AudioTools.h"

const char *url = "https://philippteepunkt.github.io/sequence-me/tts_audio/";

using namespace audio_tools;

AnalogAudioStream speechOut;
VolumeStream speechVolume(speechOut);
URLStream in("Hivenet","38459950178051317424"); // doubled because the URLStream::setClient() interface doesn't work
//URLStream in;
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
  
  // setting the volume
  speechVolume.setVolume(0.0);
  speechOut.writeSilence(speechOut.available());
}

void say(String instruction){
  if (tts.isEmpty()) {
    speechVolume.setVolume(10.0);
    if(instruction == "INITIALIZED"){
     //tts.say("initialized");   
     tts.processWord("initialized");   
    } else if(instruction == "BPM"){
     tts.processWord("bpm");
     //tts.say("bpm"); 
    } else if(instruction == "FREQUENCY"){
     tts.processWord("frequency");
     //tts.say("frequency");       
    } else if(instruction == "LISTEN"){
     tts.processWord("listen");
     //tts.say("LISTEN");
    } else if(instruction == "SAMPLE"){
     tts.processWord("sample");
     //tts.say("SAMPLE");
    } else if(instruction == "SENDING"){
     //tts.say("SENDING");
     tts.processWord("sending");
    } else if(instruction == "SEQUENCE"){
     //tts.say("SEQUENCE");
     tts.processWord("sequence");
    } else {
      Serial.println("Unknown instruction: "+instruction);
      return;
    }
  }
}
