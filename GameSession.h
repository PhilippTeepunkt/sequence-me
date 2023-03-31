#ifndef GameSession_h
#define GameSession_h

#import "Arduino.h"

#define UNINIT_SESSION 255
#define INIT_SESSION 0
#define SETTING_BPM 1
#define RECIEVED 2
#define LISTEN 3
#define SETTING_FREQUENCY 4
//#define SAMPLE_SOUND 4
#define PLAYING_SEQUENCE 5
#define SENDING 6

class GameSession {
  public:
    GameSession(uint8_t seqLen);
    uint8_t bpm = 120; 
    char id[15] = "Player1Session";
    uint8_t seqLen = 0;
    
    char* encode();
    uint8_t decode(char* payload,int len);
    uint8_t* getSequence();
    bool setSequenceSlot(uint8_t slot, uint8_t noteidx);
    uint8_t getState();
    void setState(uint8_t state);
    void reset();
  private:
    uint8_t _state = UNINIT_SESSION; 
    char _encoding[128];
    uint8_t* _sequence;
    uint8_t _decodedNote;
};
#endif
