#include "Arduino.h"
#include "GameSession.h"

GameSession::GameSession(uint8_t seqLen) {
  this->bpm = 120;
  this->seqLen = seqLen;
  this->_sequence = (uint8_t*)malloc(seqLen * sizeof(uint8_t));
  for(uint8_t i = 0; i<seqLen; i++){
    this->_sequence[i] = 255;    
  }
}

/*String converter(uint8_t *str){
    return String((char *)str);
}*/

char* GameSession::encode() {
  Serial.println(this->seqLen);
  if(this->_state == INIT_SESSION){
    strncpy(this->_encoding,"I;",2); 
  }else if(this->_state == SENDING){
    strncpy(this->_encoding,"N;",2);
    this->_encoding[2] = this->bpm;
    this->_encoding[3] = ';';
    this->_encoding[4] = this->seqLen;
    this->_encoding[5] = ';';
    for(uint8_t i = 6; i<this->seqLen+6; i++){
      this->_encoding[i] = this->_sequence[i-6]; 
    }
    this->_encoding[this->seqLen+6] = ';';
    Serial.println("Encoded:");
    Serial.print("BPM:" );
    Serial.println(this->bpm);
    Serial.print("SeqLen: ");
    Serial.println(this->seqLen);
    Serial.print("Sequence: ");
    for(uint8_t l = 0; l<this->seqLen; l++){
      Serial.print(this->_sequence[l]);      
    }
    Serial.println(" ");
  }
  return this->_encoding;
}

uint8_t GameSession::decode(char* payload, int len){
    if(len < 2){
      return UNINIT_SESSION;
    }
    
    //parse instruction
    if(payload[0] == 'I'){
      this->reset();
      return INIT_SESSION;
    }

    //parse BPM
    if(len < 4){
      return UNINIT_SESSION;
    }
    this->bpm = (uint8_t)payload[2];
    
    //parse sequence length
    if(len < 6){
      return UNINIT_SESSION;
    }
    this->seqLen = (uint8_t)payload[4];

    //parse sequence
    int index = 6;
    for(uint8_t c = 0; c<this->seqLen; c++){
      if(this->_sequence[c]==255){ // no overwrite and no mixing
       this->_sequence[c]=(uint8_t)payload[index+c]; 
      }
    }      
    Serial.println("Decoded:");
    Serial.print("BPM:" );
    Serial.println(this->bpm);
    Serial.print("SeqLen: ");
    Serial.println(this->seqLen);
    Serial.print("Sequence: ");
    for(uint8_t l = 0; l<this->seqLen; l++){
      Serial.print(this->_sequence[l]);      
    }
    Serial.println(" ");
    return RECIEVED;
}

bool GameSession::setSequenceSlot(uint8_t slot, uint8_t noteidx){
  if(slot<this->seqLen && this->_sequence[slot]==255){
    this->_sequence[slot]=noteidx;
    return true;    
  }
  return false;
}

void GameSession::reset(){
  Serial.println("Reset session.");
  this->bpm = 0;
  this->setState(UNINIT_SESSION);
  for(uint8_t i = 0; i<this->seqLen; i++){
    this->_sequence[i] = 255;    
  }
}

void GameSession::setState(uint8_t state) {
  this->_state = state;
}

uint8_t GameSession::getState() {
  return this->_state;
}

uint8_t* GameSession::getSequence() {
  return _sequence;
}
