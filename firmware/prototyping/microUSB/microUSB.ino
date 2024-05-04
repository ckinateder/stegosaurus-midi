// includes
#include <USB-MIDI.h>
#include <EasyButton.h>
#include <time.h>

#define VSN "0.1.0"

// MIDIUSB Cable
#define MIDICoreUSB_Cable 0 // this corresponds to the virtual "port" or "cable". stick to 0.

// make a type for all messages
using namespace MIDI_NAMESPACE;
typedef Message<MIDI_NAMESPACE::DefaultSettings::SysExMaxSize> MidiMessage;

// create midi instances
USBMIDI_CREATE_INSTANCE(MIDICoreUSB_Cable, MIDICoreUSB);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDICoreSerial);

// bools
bool echoUSBtoSerial = true;
bool echoUSBtoUSB = true;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  while(!Serial);

  // print welcome
  xprintf("Welcome to Stegosaurus v%s!\n", VSN);
  xprintf("Echoing USB messages over Serial: %s.\n", echoUSBtoSerial ? "true" : "false");
  xprintf("Echoing USB messages back over USB (only for debugging!): %s.\n", echoUSBtoUSB ? "true" : "false");

  // set handlers
  MIDICoreUSB.setHandleControlChange(onControlChange);
  MIDICoreUSB.setHandleProgramChange(onProgramChange);
  MIDICoreUSB.setHandleSystemExclusive(onSystemExclusive); 
  //MIDICoreUSB.setHandleSystemExclusive(onSystemExclusiveChunk);

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
  if(lastBit == 0xF7) {
    last = true;
  } 

  // print
  Serial.print(F("SysEx Message: "));
  printBytes(data, length);
  if (last) {
    Serial.println(F(" (end)"));
  } else {
    Serial.println(F(" (tbc)"));
  }
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