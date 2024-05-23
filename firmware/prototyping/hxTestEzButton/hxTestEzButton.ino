#include <MIDI.h>
#include <EasyButton.h>
#include <time.h>
#define LED_PIN 13
#define SWITCH_1_PIN 5
#define SWITCH_2_PIN 4
#define SWITCH_3_PIN 3
#define SWITCH_4_PIN 2

// make midi instance
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

// init time buffer
char timeBuffer[26];

// set button params
const int debounce = 40;
const bool pullup = true;
int shortPressDuration = 100;
int longPressDuration = 750;
EasyButton button1(SWITCH_1_PIN, debounce, pullup, true);
EasyButton button2(SWITCH_2_PIN, debounce, pullup, true);
EasyButton button3(SWITCH_3_PIN, debounce, pullup, true);
EasyButton button4(SWITCH_4_PIN, debounce, pullup, true);

// set channels
byte HELIX_CHANNEL = 16;

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
cc hxSnapshotUp = {69, 9, HELIX_CHANNEL, "hx snapshot up"};

void button1Pressed(){
    writeTime(timeBuffer);
    MIDI.sendControlChange(hxPresetUp.controlNumber, hxPresetUp.controlValue, hxPresetUp.channel);
    xprintf("[%s] [btn 1] %s\n", timeBuffer, hxPresetUp.msg);
    digitalWrite(LED_PIN, HIGH);  // LED flash
}
void button2Pressed(){
    writeTime(timeBuffer);
    MIDI.sendControlChange(hxPresetDown.controlNumber, hxPresetDown.controlValue, hxPresetDown.channel);
    xprintf("[%s] [btn 2] %s\n", timeBuffer, hxPresetDown.msg);
    digitalWrite(LED_PIN, HIGH);  // LED flash
}
void button3Pressed(){
    writeTime(timeBuffer);
    MIDI.sendControlChange(hxSnapshotUp.controlNumber, hxSnapshotUp.controlValue, hxSnapshotUp.channel);
    xprintf("[%s] [btn 3] %s\n", timeBuffer, hxSnapshotUp.msg);
    digitalWrite(LED_PIN, HIGH);  // LED flash
}
void button4Pressed(){
    writeTime(timeBuffer);
    MIDI.sendControlChange(hxSnapshotDown.controlNumber, hxSnapshotDown.controlValue, hxSnapshotDown.channel);
    xprintf("[%s] [btn 4] %s\n", timeBuffer, hxSnapshotDown.msg);
    digitalWrite(LED_PIN, HIGH);  // LED flash
}
void setup() {
  // init serial and MIDI
  Serial.begin(115200);
  MIDI.begin(MIDI_CHANNEL_OMNI);
  
  // onboard LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  // LED off

  // set callbacks
  button1.onPressedFor(shortPressDuration, button1Pressed);
  button2.onPressedFor(shortPressDuration, button2Pressed);
  button3.onPressedFor(shortPressDuration, button3Pressed);
  button4.onPressedFor(shortPressDuration, button4Pressed);

  // begin buttons
  button1.begin();
  button2.begin();
  button3.begin();
  button4.begin();

  MIDI.setHandleControlChange(onControlChange);
  MIDI.setHandleProgramChange(onProgramChange);
}


static void onControlChange(byte channel, byte number, byte value) {
  xprintf("ControlChange from channel: %d, number: %d, value: %d\n", channel, number, value);
}

static void onProgramChange(byte channel, byte number) {
  xprintf("ProgramChange from channel: %d, number: %d\n", channel, number);
}

// the loop() methor runs over and over again,
// as long as the board has power

void loop() {
  MIDI.read();
  button1.read();
  button2.read();
  button3.read();
  button4.read();
}

//utility functions
void xprintf(const char *format, ...)
{
  char buffer[256];  // or smaller or static &c.
  va_list args;
  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);
  Serial.print(buffer);
}

void writeTime(char buffer[]){
  time_t timer;
  struct tm* tm_info;

  timer = time(NULL);
  tm_info = localtime(&timer);

  strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
}

void sendCC(cc toSend) {
  MIDI.sendControlChange(toSend.controlNumber, toSend.controlValue, toSend.channel);
}
void sendPC(pc toSend) {
  MIDI.sendProgramChange(toSend.programNumber, toSend.channel);
}