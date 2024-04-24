#include <MIDI.h>
#include <time.h>
#define LED_PIN 13
#define SWITCH_1_PIN 5
#define SWITCH_2_PIN 4
#define SWITCH_3_PIN 3
#define SWITCH_4_PIN 2

// make 
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

byte HELIX_CHANNEL = 1;

struct pc {
  byte programNumber;
  byte channel;
};

struct cc {
  byte controlNumber;
  byte controlValue;
  byte channel;
  char msg[];
};

// probably switch to a dict type of messages so it's updatable
cc hxPresetDown = {72, 0, HELIX_CHANNEL, "hx preset down"};
cc hxPresetUp = {72, 127, HELIX_CHANNEL, "hx preset up"};
cc hxSnapshotDown = {69, 8, HELIX_CHANNEL, "hx snapshot down"};
cc hxSnapshotUp = {69, 9, HELIX_CHANNEL, "hx snapshot down"};

// the setup() method runs once, when the sketch starts

void setup() {
  // initialize the digital pin as an output.
  Serial.begin(9600);
  MIDI.begin();
  pinMode(LED_PIN, OUTPUT);
  pinMode(SWITCH_1_PIN, INPUT_PULLUP);
  pinMode(SWITCH_2_PIN, INPUT_PULLUP);
  pinMode(SWITCH_3_PIN, INPUT_PULLUP);
  pinMode(SWITCH_4_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, LOW);  // LED off
}

// the loop() methor runs over and over again,
// as long as the board has power

void writeTime(char buffer[]){
  time_t timer;
  struct tm* tm_info;

  timer = time(NULL);
  tm_info = localtime(&timer);

  strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
}

void loop() {
  char buffer[26];
  writeTime(buffer);

  digitalWrite(LED_PIN, LOW);  // LED off
  if(!digitalRead(SWITCH_1_PIN)){
    MIDI.sendControlChange(hxPresetUp.controlNumber, hxPresetUp.controlValue, hxPresetUp.channel);
    Serial.printf("[%s] [btn 1] %s\n", buffer, hxPresetUp.msg);
    digitalWrite(LED_PIN, HIGH);  // LED flash
  }
  if(!digitalRead(SWITCH_2_PIN)){
    MIDI.sendControlChange(hxPresetDown.controlNumber, hxPresetDown.controlValue, hxPresetDown.channel);
    Serial.printf("[%s] [btn 2] %s\n", buffer, hxPresetDown.msg);
    digitalWrite(LED_PIN, HIGH);  // LED flash
  }
  if(!digitalRead(SWITCH_3_PIN)){
    MIDI.sendControlChange(hxSnapshotUp.controlNumber, hxSnapshotUp.controlValue, hxSnapshotUp.channel);
    Serial.printf("[%s] [btn 3] %s\n", buffer, hxSnapshotUp.msg);
    digitalWrite(LED_PIN, HIGH);  // LED flash
  }
  if(!digitalRead(SWITCH_4_PIN)){
    MIDI.sendControlChange(hxSnapshotDown.controlNumber, hxSnapshotDown.controlValue, hxSnapshotDown.channel);
    Serial.printf("[%s] [btn 4] %s\n", buffer, hxSnapshotDown.msg);
    digitalWrite(LED_PIN, HIGH);  // LED flash
  }
  delay(200);
}

