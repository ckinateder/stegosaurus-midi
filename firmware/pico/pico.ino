/*********************************************************************
 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 Copyright (c) 2019 Ha Thach for Adafruit Industries
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

/* This sketch is enumerated as USB MIDI device. 
 * Following library is required
 * - MIDI Library by Forty Seven Effects
 *   https://github.com/FortySevenEffects/arduino_midi_library
 */

#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#include <EasyButton.h>

#define LED_PIN 13
#define VSN "0.1.0"

#define MFID_1 0x53
#define MFID_2 0x4d

#define MODIFY_PRESET 0x0
#define MODIFY_PRESET_LENGTH 6 // length of a modify preset message

#define SAVE_TO_EEPROM 0x1

// MIDIUSB Cable
#define MIDICoreUSB_Cable 0 // this corresponds to the virtual "port" or "cable". stick to 0.

// make a type for all messages
using namespace MIDI_NAMESPACE;
typedef Message<MIDI_NAMESPACE::DefaultSettings::SysExMaxSize> MidiMessage;

// USB MIDI object, with 1 cable
Adafruit_USBD_MIDI USB_MIDI_Transport(1);

// Create a new instance of the Arduino MIDI Library,
// and attach usb_midi as the transport.
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, USB_MIDI_Transport, MIDICoreUSB);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDICoreSerial);

// bools
bool echoUSBtoSerial = true; // echo from usb to serial

void setup()
{
  // start serial
  Serial.begin(115200);

  // Manual begin() is required on core without built-in support for TinyUSB such as mbed rp2040
  #if defined(ARDUINO_ARCH_MBED) && defined(ARDUINO_ARCH_RP2040)
    TinyUSB_Device_Init(0);
  #endif

  // set names
  USBDevice.setManufacturerDescriptor     ("CJK Devices");
  USBDevice.setProductDescriptor          ("Stegosaurus (PICO)");
  USB_MIDI_Transport.setStringDescriptor  ("Stegosaurus MIDI");
  //USB_MIDI_Transport.setCableName         (1, "Controller"); // set this if using multiple cable types

  // set handlers
  MIDICoreUSB.setHandleControlChange(onControlChange);
  MIDICoreUSB.setHandleProgramChange(onProgramChange);
  MIDICoreUSB.setHandleSystemExclusive(onSystemExclusive); 

  // set to receieve all channels - in configuration, filter them out
  MIDICoreUSB.begin(MIDI_CHANNEL_OMNI);
  MIDICoreSerial.begin(MIDI_CHANNEL_OMNI); // receive on all channels

  // echo all USB messages to devices on the serial
  MIDICoreUSB.setHandleMessage(onMessageUSB);
  MIDICoreSerial.setHandleControlChange(onControlChange);
  MIDICoreSerial.setHandleProgramChange(onProgramChange);

  pinMode(LED_BUILTIN, OUTPUT);
  // wait until device mounted
  while( !TinyUSBDevice.mounted() ) delay(1);
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
  unsigned int start = 5;
  byte operation = leftNybble(*(data + start));
  byte saveToROM = rightNybble(*(data + start));
  int segmentLength = length - start;

  if(operation == MODIFY_PRESET && segmentLength == MODIFY_PRESET_LENGTH){
    
  }
  else {
    Serial.print(F("Unsupported operation for segment length: "));
    Serial.print(operation, HEX);
    Serial.print(F(", length "));
    Serial.println(segmentLength);
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
  if(echoUSBtoSerial)
    MIDICoreSerial.send(message);
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


byte leftNybble(const byte b){
  return b >> 4;
}

byte rightNybble(const byte b){
  return b & 0x0F;
}