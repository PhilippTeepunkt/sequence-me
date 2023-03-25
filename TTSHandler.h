#define NO_AUDIO_EXAMPLES
#include "SimpleTTS.h"
#include "AudioCodecs/CodecMP3Helix.h"
#include "AudioDictionary.h"
#include "AudioDictionaryURL.h"

simple_tts::AudioDictionaryEntry MyAudioDictionaryValues[] = {
    {"BPM",new MemoryStream(BPM_mp3,BPM_mp3_len)},
    {"FREQUENCY",new MemoryStream(Frequency_full_mp3,Frequency_full_mp3_len)},
    /*{"LISTEN",new MemoryStream(Listen_mp3,Listen_mp3_len)},
    {"SAMPLE",new MemoryStream(Sample_mp3,Sample_mp3_len)},
    {"SENDING",new MemoryStream(Sending_mp3,Sending_mp3_len)},
    {"SEQUENCE",new MemoryStream(Sequence_mp3,Sequence_mp3_len)},*/
    {nullptr, nullptr}
};

AnalogAudioStream speechOut;
URLStream in(net); 
AudioDictionaryURL dictionary(in, url, "mp3");
MP3DecoderHelix mp3;
TextToSpeechQueue tts(speechVolume, mp3, dictionary);

void initializeSpeechHandler() {
  // setup out
  auto cfg = speechOut.defaultConfig();
  cfg.sample_rate = 44100;
  cfg.channels = 1;
  cfg.bits_per_sample = 32;
  speechOut.begin(cfg);

  // setting the volume
  speechVolume.setVolume(10.0);
}

void say(String instruction){
  if (tts.isEmpty()) {
    if(instruction == "BPM"){
     tts.say("BPM"); 
    } else if(instruction == "FREQUENCY"){
     tts.say("FREQUENCY");       
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
    //delay(2000);
  }
}
