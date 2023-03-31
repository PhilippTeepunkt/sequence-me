#include "AudioTools.h"
#include "AudioLibs/AudioA2DP.h"
#include <WireGuard-ESP32.h>
#include "WiFiClientSecure.h"
#include "MQTT.h"
#include "GameSession.h"
#include "TTSHandler.h"

#define SEQUENCE_LEN 32
#define NUM_NOTES 21
#define BUTTON_PIN 33
#define SENDING_BUTTON_PIN 27
#define POTI_PIN 36
#define LED_BUTTON_PIN 21
#define LED_PIN 15

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
GameSession gs(SEQUENCE_LEN);

// wifi
const char* ssid = "ssid";
const char* pw = "password"; 
WiFiClient net;
const char* wifiHostname = "Player1_Input";

// wireguard vpn
static WireGuard wg;
IPAddress local_ip(0,0,0,0);
const char* private_key = "privatekey";
//const char* local_public_key = "...";
const char* endpoint_public_key = "publickey";
const char* endpoint_address = "vpn domain";
const int endpoint_port = 49999;

// MQTT
const char* brokerId = "Hivenet-NAS";
MQTTClient client;

// ============ LOGIC ==============

// interface inputs
int potiValue = 0;
int currentButtonState = HIGH;
int lastButtonState = HIGH;
int currentSendingButtonState = HIGH;
int lastSendingButtonState = HIGH;

// Sequencer
hw_timer_t *sequencer = NULL;
float timer_duration = 0.0;
uint16_t counter = 0;
float sequenceData[SEQUENCE_LEN];
static float notes_array[] = { // frequencies aleatoric C-scale
    N_C3, N_D3, N_E3, N_F3, N_G3, N_A3, N_B3,
    N_C4, N_D4, N_E4, N_F4, N_G4, N_A4, N_B4,
    N_C5, N_D5, N_E5, N_F5, N_G5, N_A5, N_B5
};

// GameLoop
uint8_t lastState = 255;
long timestamp = 0;
uint8_t netResult = 255;
bool selfPublish = false;
uint8_t lastSelectedNote = 255;

void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void messageReceived(MQTTClient *cl, char topic[], char bytes[], int length) {
  String topicstr = topic;
  Serial.println("incomming:");
  Serial.print(topic);
  Serial.print("-");
  Serial.print(bytes);
  Serial.println(" ");
  if(topicstr.equals("check-alive")){
    client.publish(wifiHostname,"alive");
  }

  if(topicstr.equals("test")){
    //client.publish("test_answer",test, 4);
    //timerAlarmEnable(My_timer);
    int note = 0;
    for(int i = 0; i<SEQUENCE_LEN; i++){
      if(i<length){
       sequenceData[i] = notes_array[bytes[i]]; 
      }
    }
    timerAlarmEnable(sequencer);
  }

  if(topicstr.equals(gs.id)){
    if(selfPublish){
      selfPublish = false;
      return;
    }
    netResult = gs.decode(bytes, length);
    if(netResult == INIT_SESSION){
      gs.setState(INIT_SESSION);
    }else if(netResult == RECIEVED){
      write_session();
      gs.setState(RECIEVED);
      lastState = INIT_SESSION;
      digitalWrite(LED_BUTTON_PIN, HIGH);
    }
  }
}

//helper method for data stream callback

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
  client.setCleanSession(false);
  client.begin(ip, 1883, net);
  client.onMessageAdvanced(messageReceived);

  //connecting with broker
  Serial.print("\nconnecting with broker...");
  while(!client.connect(wifiHostname)){   
    Serial.print(".");
    delay(5000);
  }
  client.subscribe("check-alive");
  client.subscribe("test");
  client.subscribe(gs.id,1);
  Serial.println("\nConnected with Broker.\n");
}

float floatMap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float getClosestNote(float freq){
  uint8_t closestNote = 0;
  float distance = abs(notes_array[closestNote]-freq);
  for(uint8_t n = 0; n < NUM_NOTES; n++){
    distance = abs(notes_array[n]-freq);
    Serial.println(distance);
    if(abs(notes_array[closestNote]-freq)>distance){
      closestNote = n;            
    }
  }
  lastSelectedNote = closestNote;
  return notes_array[closestNote];
}

void IRAM_ATTR onSequenceTick(){
  counter = (counter+1)%SEQUENCE_LEN;
  digitalWrite(LED_PIN, LOW);
  if(gs.getState()!=SETTING_FREQUENCY && counter%4==0){
    digitalWrite(LED_PIN, HIGH);    
  }
  //Serial.println(counter);
}

void write_bpm(){
  for(uint8_t i = 0; i<SEQUENCE_LEN; i++){
    if(i%4==0){
     sequenceData[i] = N_B3;
    }else {
     sequenceData[i] = 0.0; 
    }
  }
}

void write_empty(){
  for(uint8_t i = 0; i<SEQUENCE_LEN; i++){
    sequenceData[i] = 0.0;
  }
}

void write_session(){
  uint8_t* seq = gs.getSequence();
  for(uint8_t i = 0; i<gs.seqLen; i++){
    if(seq[i]<NUM_NOTES){
     sequenceData[i] = notes_array[seq[i]]; 
    }        
  }
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

  // init sequencer
  sequencer = timerBegin(0, 80, true);
  timerAttachInterrupt(sequencer, &onSequenceTick, true);
  timer_duration = 1.0/gs.bpm * 15 * 1000000;
  //timerAlarmWrite(sequencer, 1000000 , true);
  timerAlarmWrite(sequencer, (int)timer_duration, true);
  write_empty();

  volume.setVolume(0.5);

  // Text to speech
  initializeSpeechHandler(net);

  //mixer.begin();
  timestamp = millis();
  pinMode(BUTTON_PIN, INPUT_PULLUP); // config GIOP33 as input pin and enable the internal pull-up resistor
  pinMode(SENDING_BUTTON_PIN, INPUT_PULLUP); //
  pinMode(LED_BUTTON_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  //check for connection
  if(!client.connected()){
    Serial.println("Lost connection with broker. Reconnecting.\n");
    connect();
  }
  client.loop();

  if(gs.getState()==UNINIT_SESSION){
    //startup delay
    if(millis() < timestamp + 6000){
      return;   
    }

    //initialize session
    Serial.println("Initialize Session.");
    gs.setState(INIT_SESSION);
    selfPublish = true;
    client.publish(gs.id,gs.encode(),true, 1);
    say("INITIALIZED");
    lastState = gs.getState();
    
    // start bpm selection
    gs.setState(SETTING_BPM);
    say("BPM");
    write_bpm();
    timerAlarmEnable(sequencer); 
  }

  // read send and reset button
  currentSendingButtonState = digitalRead(SENDING_BUTTON_PIN);
  if(lastSendingButtonState == HIGH && currentSendingButtonState == LOW){
    timestamp = millis();
    if(gs.getState() == PLAYING_SEQUENCE){
      gs.setState(SENDING);
      say("SENDING");
      timerAlarmDisable(sequencer);
      counter = 0;
      selfPublish = true;       
      while(!client.publish(gs.id,gs.encode(),true, 1)){
        selfPublish = true;
        Serial.println("Failed sending Sequence!");               
      }
      gs.reset();
      lastState = gs.getState();
      gs.setState(INIT_SESSION);
    }    
    lastSendingButtonState = currentSendingButtonState;
    return;
  }else if (lastSendingButtonState == LOW && currentSendingButtonState == LOW){
      if(millis()-timestamp >= 5000){
        client.publish(gs.id,"",true,1);
        gs.reset(); 
        timestamp = millis();
        Serial.println("RESET");
        timerAlarmDisable(sequencer);
        counter = 0;
        lastSendingButtonState = currentSendingButtonState;
        return;         
      }
   }
   lastSendingButtonState = currentSendingButtonState;

  // read arcade button
  currentButtonState = digitalRead(BUTTON_PIN);
  if(lastButtonState == LOW && currentButtonState == HIGH){  
    digitalWrite(LED_BUTTON_PIN, LOW);          
  }else if (lastButtonState == HIGH && currentButtonState == LOW){

    // sample tone on click
    if(gs.getState() == PLAYING_SEQUENCE){
      if(lastSelectedNote!=255){
        sequenceData[counter] = notes_array[lastSelectedNote]; 
        gs.setSequenceSlot(counter,lastSelectedNote);
        digitalWrite(LED_BUTTON_PIN, HIGH);
        Serial.println("Sample");                     
      }
    }
    
    // end setting frequency, start sequencing
    if(gs.getState() == SETTING_FREQUENCY){
      sineWave.setFrequency(0.0);
      lastState = gs.getState();
      gs.setState(PLAYING_SEQUENCE);
      say("SEQUENCE");
      timer_duration = 1.0/gs.bpm * 15 * 1000000;
      timerAlarmWrite(sequencer, timer_duration , true);
      counter = 0;
      timerAlarmEnable(sequencer);      
    }
    
    // end setting bpm, start tone selection
    if(gs.getState()==SETTING_BPM || gs.getState() == LISTEN){
      if(gs.getState()==SETTING_BPM){
        write_empty();
      }
      timerAlarmDisable(sequencer);
      say("FREQUENCY"); 
      lastState = gs.getState();
      gs.setState(SETTING_FREQUENCY);         
    }
    
    // trigger listening to incoming sequence
    if(gs.getState() == RECIEVED){
       digitalWrite(LED_BUTTON_PIN, HIGH);
       say("LISTEN");
       lastState = gs.getState();
       gs.setState(LISTEN);
       counter = 0;
       timerAlarmEnable(sequencer); 
    }    
  }
  lastButtonState = currentButtonState;

  // reading potentiometer
  potiValue = analogRead(POTI_PIN);
  if(gs.getState()==SETTING_BPM){
    if(millis()-timestamp > 500){
      gs.bpm = (int)floatMap(potiValue, 0, 4095, 50, 250);
      timer_duration = 1.0/gs.bpm * 15 * 1000000;
      timerAlarmWrite(sequencer, timer_duration , true);
      copierOut.copy(); 
      timestamp = millis();
    }
    sineWave.setFrequency(sequenceData[counter]);
    copierOut.copy();
    return;
  }

  // set frequency
  //frequency = (double)floatMap(potiValue, 0, 4095, 100, 1500);
  if(gs.getState() == SETTING_FREQUENCY){
    sineWave.setFrequency(getClosestNote((float)floatMap(potiValue, 0, 4095, notes_array[0], notes_array[NUM_NOTES-1])));
    copierOut.copy();
  }

  if(gs.getState() == PLAYING_SEQUENCE ||gs.getState() == LISTEN){
    sineWave.setFrequency(sequenceData[counter]);
    copierOut.copy();
  }
}
