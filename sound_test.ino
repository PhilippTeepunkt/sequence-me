#include "AudioTools.h"
#include "AudioLibs/AudioA2DP.h"
#include <WireGuard-ESP32.h>
#include "WiFiClientSecure.h"
#include "MQTT.h"

#define SEQUENCE_LEN 32

// sound generation
uint16_t sample_rate=44100;
uint8_t channels = 1;                        
//SineFromTable<int16_t> sineWave(32000);
SquareWaveGenerator<int16_t> sineWave(32000);
//SineFromTable<int16_t> sineWave2(32000);
GeneratedSoundStream<int16_t> sound(sineWave);
//GeneratedSoundStream<int16_t> sound2(sineWave2);
AnalogAudioStream out; 
VolumeStream volume(out);
//OutputMixer<int16_t> mixer(out, 2);
//StreamCopy copier(mixer, sound);
//StreamCopy copier2(mixer, sound2);// copies sound into i2s
StreamCopy copierOut(volume, sound);

// wifi
const char* ssid = "ssid";
const char* pw = "1234567890"; 
WiFiClient net;
const char* wifiHostname = "Player1_Input";

#include "TTSHandler.h"

// wireguard vpn
static WireGuard wg;
IPAddress local_ip(198,18,7,8);
const char* private_key = "####private_key####";
//const char* local_public_key = "####public_key####";
const char* endpoint_public_key = "####public_key####";
const char* endpoint_address = "vpn.domain.de";
const int endpoint_port = 49999;

// MQTT
const char* brokerId = "BrokerID";
MQTTClient client;

// LOGIC

// interface inputs
const int POTI_PIN = 36;
int potiValue = 0;

hw_timer_t *sequencer = NULL;
uint16_t counter = 0;
float sequenceData[SEQUENCE_LEN];
static float notes_array[] = { // frequencies aleatoric C-scale
    N_C3, N_D3, N_E3, N_F3, N_G3, N_A3, N_B3,
    N_C4, N_D4, N_E4, N_F4, N_G4, N_A4, N_B4,
    N_C5, N_D5, N_E5, N_F5, N_G5, N_A5, N_B5
};

uint8_t bpm = 120;

void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

//helper method for data stream callback
void messageReceive(String &topic, String &payload){
  Serial.println("incomming: "+topic+" - "+payload);  
  if(topic.equals("check-alive")){
    if(payload.equals(wifiHostname)){
      client.publish(wifiHostname,"1");
    }
  }
  if(topic.equals("test")){
    //client.publish("test_answer",test, 4);
    //timerAlarmEnable(My_timer);
    say("BPM");
    int note = 0;
    for(int i = 0; i<SEQUENCE_LEN; i++){
      note = payload[i] - '0';
      sequenceData[i] = notes_array[note];      
    }
  }
}

void connect(){
  //search for wifi 
  Serial.print("\nchecking wifi...");
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(1000);
  }
  Serial.print("\nconnected with wifi!\n"); 

  Serial.println("Adjusting system time...");
  configTime(1 * 60 * 60,0,"0.de.pool.ntp.org","ntp1.t-online.de","time.google.com");
  printLocalTime();

  //VPN
  Serial.print("\nConnect VPN ...\n");
  while(!wg.is_initialized()) {
    if( !wg.begin(
      local_ip,
      private_key,
      endpoint_address,
      endpoint_public_key,
      endpoint_port) ) {
      Serial.println("Failed to initialize WG interface.");
    }
  }
  Serial.println("Connected Wireguard VPN.\n");

  // MQTT
  const IPAddress ip = IPAddress(192,168,178,10);
  //client.begin(brokerIP, 1883, net);
  client.begin(ip, 1883, net);
  client.onMessage(messageReceive);

  //connecting with broker
  Serial.print("\nconnecting with broker...");
  while(!client.connect(wifiHostname)){   
    Serial.print(".");
    delay(5000);
  }
  client.subscribe("check-alive");
  client.subscribe("test");
  Serial.println("\nConnected with Broker.\n");
}

float floatMap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void IRAM_ATTR onSequenceTick(){
  counter = (counter+1)%SEQUENCE_LEN;
  Serial.println(counter);
}

// Arduino Setup
void setup(void) {  
  // Open Serial 
  Serial.begin(115200);
  AudioLogger::instance().begin(Serial, AudioLogger::Warning);

    //WiFi
  WiFi.setHostname(wifiHostname);
  WiFi.begin(ssid,pw);  
  connect();

    // start I2S
  Serial.println("starting I2S...");
  auto config = out.defaultConfig(TX_MODE);
  config.sample_rate = sample_rate; 
  config.channels = channels;
  config.bits_per_sample = 16;
  out.begin(config);
  volume.begin(config);

  // setup sine wave
  auto cfg = sineWave.defaultConfig();
  cfg.channels = channels;
  cfg.sample_rate = sample_rate;
  sineWave.begin(channels, sample_rate, N_B4);
  //sineWave2.begin(channels, sample_rate, N_B4);
  Serial.println("started...");

  // start sequencer
  sequencer = timerBegin(0, 80, true);
  timerAttachInterrupt(sequencer, &onSequenceTick, true);
  //float duration = 1/bpm * 60 * 1000000;
  timerAlarmWrite(sequencer, 500000 , true);
  timerAlarmEnable(sequencer);
  for(uint8_t i = 0; i<SEQUENCE_LEN; i++){
    sequenceData[i] = 0.0;  
  }

  volume.setVolume(0.5);

  // Text to speech
  initializeSpeechHandler();

  //mixer.begin();
}

void loop() {
  // copy sound to out 
  //copier.copy();
  //copier2.copy();
  copierOut.copy();

  client.loop();
  //check for connection
  if(!client.connected()){
    Serial.println("Lost connection with broker. Reconnecting.\n");
    connect();
  }

  // set frequency
  //potiValue = analogRead(POTI_PIN);
  //frequency = (double)floatMap(potiValue, 0, 4095, 100, 1500);
  sineWave.setFrequency(sequenceData[counter]);

  tts.process();
}
