// includes
#include <USB-MIDI.h>
#include <EasyButton.h>
#include <time.h>

#define LED_PIN 13
#define VSN "0.1.0"

#define MFID_1 0x53
#define MFID_2 0x4d

// MIDIUSB Cable
#define MIDICoreUSB_Cable 0 // this corresponds to the virtual "port" or "cable". stick to 0.

// make a type for all messages
using namespace MIDI_NAMESPACE;
typedef Message<MIDI_NAMESPACE::DefaultSettings::SysExMaxSize> MidiMessage;

// create midi instances
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDICoreSerial);
USBMIDI_CREATE_INSTANCE(MIDICoreUSB_Cable, MIDICoreUSB);

// bools
bool echoUSBtoSerial = true;
bool echoUSBtoUSB = true;

void setup() {
  Serial.begin(9600);
  while(!Serial); // wait for Serial to be ready
  
  // print welcome
  xprintf("Welcome to Stegosaurus v%s!\n", VSN);
  xprintf("Echoing USB messages over Serial: %s.\n", echoUSBtoSerial ? "true" : "false");
  xprintf("Echoing USB messages back over USB (only for debugging!): %s.\n", echoUSBtoUSB ? "true" : "false");

  // setup LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  // LED off

  // set handlers
  MIDICoreUSB.setHandleControlChange(onControlChange);
  MIDICoreUSB.setHandleProgramChange(onProgramChange);
  MIDICoreUSB.setHandleSystemExclusive(onSystemExclusive); 

  // echo all USB messages to devices on the serial
  MIDICoreUSB.setHandleMessage(onMessageUSB);

  // set to receieve all channels - in configuration, filter them out
  MIDICoreSerial.begin(MIDI_CHANNEL_OMNI); // receive on all channels
  MIDICoreUSB.begin(MIDI_CHANNEL_OMNI); // receive on all channels
}

void loop() {
  // -- listen and read
  MIDICoreUSB.read();
  MIDICoreSerial.read();

  // -- any button logic can go here --

}

// -- message handlers --
static void onSystemExclusive(byte *data, unsigned int length) {
  // check last byte in msg - if it is F0, then more is coming
  // if it is F7, it is the last
  byte lastBit = *(data + length - 1); // last byte in data

  // assign last bit
  bool last = false;
  if(lastBit == 0xF7)
    last = true;

  // print
  xprintf("SysEx Message (%dB): ", length);
  printBytes(data, length);
  if (last) {
    Serial.println(F(" (end)"));
  } else {
    Serial.println(F(" (tbc)"));
  }

  // exit if not last bit
  if(last && checkVendor(data, length))
    parseSysExMessage(data, length);
  else
    Serial.println(F("Unexplained issue! The SysEx message is not getting processed"));
}

static void parseSysExMessage(byte *data, unsigned int length) {
  
}

static void onControlChange(byte channel, byte number, byte value) {
  xprintf("ControlChange from channel: %d, number: %d, value: %d\n", channel, number, value);
}

static void onProgramChange(byte channel, byte number) {
  xprintf("ProgramChange from channel: %d, number: %d\n", channel, number);
}

// echo all messages over usb
static void onMessageUSB(const MidiMessage& message) {
  if(echoUSBtoSerial){
    //Serial.println("Echoing message from USB to DIN.");
    MIDICoreSerial.send(message);
  }
  if(echoUSBtoUSB){
    MIDICoreUSB.send(message);
  }
  // flashLed();
}

bool checkVendor(const byte *data, unsigned int size) {
  if(size < 5) // if not long enough to contain the needed bytes, then it doesn't match
    return false;
  return ( *(data + 1) == 0x00 && *(data + 2) == MFID_1 && *(data + 3) == MFID_2 );
}

// -- utility 
void printBytes(const byte *data, unsigned int size) {
  while (size > 0) {
    byte b = *data++;
    if (b < 16) Serial.print('0');
    Serial.print(b, HEX);
    if (size > 1) Serial.print(' ');
    size = size - 1;
  }
}

void xprintf(const char *format, ...)
{
  char buffer[256];  // or smaller or static &c.
  va_list args;
  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);
  Serial.print(buffer);
}

void flashLed(){
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
}