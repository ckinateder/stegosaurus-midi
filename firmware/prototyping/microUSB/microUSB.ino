// includes
#include <USB-MIDI.h>

// MIDIUSB Cable
#define MIDICoreUSB_Cable 0 // this corresponds to the virtual "port" or "cable". stick to 0.

// make a type for all messages
using namespace MIDI_NAMESPACE;
typedef Message<MIDI_NAMESPACE::DefaultSettings::SysExMaxSize> MidiMessage;

// create midi instances
USBMIDI_CREATE_INSTANCE(MIDICoreUSB_Cable, MIDICoreUSB);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDICoreSerial);


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  // set handlers
  MIDICoreUSB.setHandleControlChange(onControlChange);
  MIDICoreUSB.setHandleProgramChange(onProgramChange);

  // echo all USB messages to devices on the serial
  MIDICoreUSB.setHandleMessage(echoUSBtoSerial);

  // set to receieve all channels - in configuration, filter them out
  MIDICoreSerial.begin(MIDI_CHANNEL_OMNI); // receive on all channels
  MIDICoreUSB.begin(MIDI_CHANNEL_OMNI); // receive on all channels
}

void loop() {
  // put your main code here, to run repeatedly:
  MIDICoreUSB.read();
}

static void onControlChange(byte channel, byte number, byte value) {
  Serial.print(F("ControlChange from channel: "));
  Serial.print(channel);
  Serial.print(F(", number: "));
  Serial.print(number);
  Serial.print(F(", value: "));
  Serial.println(value);
}

static void onProgramChange(byte channel, byte number) {
  Serial.print(F("ProgramChange from channel: "));
  Serial.print(channel);
  Serial.print(F(", number: "));
  Serial.println(number);
}

static void echoUSBtoSerial(const MidiMessage& message) {
  Serial.println("Echoing message from USB to DIN.");
  MIDICoreSerial.send(message);
}