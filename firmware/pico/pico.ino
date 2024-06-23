/* This sketch is enumerated as USB MIDI device.
 * Following library is required
 * - MIDI Library by Forty Seven Effects
 *   https://github.com/FortySevenEffects/arduino_midi_library
 */

#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#include <EasyButton.h>

#define VSN "0.1.0"
#define LED_PIN 13
#define MFID_1 0x53
#define MFID_2 0x4d
#define SLOTS_PER_PRESET 16
#define SAVE_TO_EEPROM 0x1
#define MIDICoreUSB_Cable 0 // this corresponds to the virtual "port" or "cable". stick to 0.
#define PULLUP true
#define DEBOUNCE 40
#define SHORT_PRESS_DURATION 100
#define LONG_PRESS_DURATION 750
#define SWITCH_1_PIN 10
#define SWITCH_2_PIN 11
#define SWITCH_3_PIN 12
#define SWITCH_4_PIN 13

// main storage
byte MEMORY[128][SLOTS_PER_PRESET][6]; // 128 presets, SLOTS_PER_PRESET slots, 6 bytes each
byte CURRENT_PRESET = 0;

// enums
// NONE of these can be 0
enum Action
{
  ControlChange = 0x01,
  ProgramChange = 0x02,
  PresetUp = 0x03,   // on stegosaurus
  PresetDown = 0x04, // on stegosaurus
};
enum Trigger
{
  EnterPreset = 0x01,
  ExitPreset = 0x02,
  ShortPress = 0x03,
  LongPress = 0x04,
  DoublePress = 0x05,
};
enum SwitchType
{
  NA = 0x1,
  Momentary = 0x2,
  Latch = 0x3
};

// buttons
EasyButton button1(SWITCH_1_PIN, DEBOUNCE, PULLUP, true);
EasyButton button2(SWITCH_2_PIN, DEBOUNCE, PULLUP, true);
EasyButton button3(SWITCH_3_PIN, DEBOUNCE, PULLUP, true);
EasyButton button4(SWITCH_4_PIN, DEBOUNCE, PULLUP, true);

// bools
bool midiThru = true; // passthrough serial and usb midi

// make a type for all messages
using namespace MIDI_NAMESPACE;
typedef Message<MIDI_NAMESPACE::DefaultSettings::SysExMaxSize> MidiMessage;

// USB MIDI object, with 1 cable
Adafruit_USBD_MIDI USB_MIDI_Transport(1);

// Create a new instance of the Arduino MIDI Library,
// and attach usb_midi as the transport.
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, USB_MIDI_Transport, MIDICoreUSB);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDICoreSerial);

void setup()
{
  // start serial
  Serial.begin(115200);

  // set pins
  pinMode(LED_BUILTIN, OUTPUT);

// Manual begin() is required on core without built-in support for TinyUSB such as mbed rp2040
#if defined(ARDUINO_ARCH_MBED) && defined(ARDUINO_ARCH_RP2040)
  TinyUSB_Device_Init(0);
#endif

  // ------------------- set up connections -------------------
  // set names
  USBDevice.setManufacturerDescriptor("CJK Devices");
  USBDevice.setProductDescriptor("Stegosaurus (PICO)");
  USB_MIDI_Transport.setStringDescriptor("Stegosaurus MIDI");
  // USB_MIDI_Transport.setCableName         (1, "Controller"); // set this if using multiple cable types

  // set handlers
  MIDICoreUSB.setHandleControlChange(onControlChange);
  MIDICoreUSB.setHandleProgramChange(onProgramChange);
  MIDICoreUSB.setHandleSystemExclusive(onSystemExclusive);

  MIDICoreSerial.setHandleControlChange(onControlChange);
  MIDICoreSerial.setHandleProgramChange(onProgramChange);

  // echo all USB messages to devices on the serial
  MIDICoreUSB.setHandleMessage(onMessageUSB);

  // set to receieve all channels - in configuration, filter them out
  MIDICoreUSB.begin(MIDI_CHANNEL_OMNI);
  MIDICoreSerial.begin(MIDI_CHANNEL_OMNI); // receive on all channels

  // wait until device mounted
  while (!TinyUSBDevice.mounted())
    delay(1);

  // ------------------- set up buttons -------------------
  button1.onPressedFor(SHORT_PRESS_DURATION, buttonPressed);
  button2.onPressedFor(SHORT_PRESS_DURATION, buttonPressed);
  button3.onPressedFor(SHORT_PRESS_DURATION, buttonPressed);
  button4.onPressedFor(SHORT_PRESS_DURATION, buttonPressed);

  // begin buttons
  button1.begin();
  button2.begin();
  button3.begin();
  button4.begin();

  // ------------------- set up memory -------------------
  // set all memory to 0
  memset(MEMORY, 0, sizeof(MEMORY));

  // populate memory with some defaults
  for (int i = 0; i < 128; i++)
  {
    writeSlotToMemory(i, 0, Action::ControlChange, Trigger::EnterPreset, 0, SwitchType::Latch, 0, randomByte(0, 127), 127);
    writeSlotToMemory(i, 1, Action::ProgramChange, Trigger::ShortPress, 1, SwitchType::Latch, 0, 17, 0);
    writeSlotToMemory(i, 2, Action::ControlChange, Trigger::EnterPreset, 0, SwitchType::NA, 0, 18, randomByte(0, 127));
    writeSlotToMemory(i, 3, Action::ProgramChange, Trigger::ExitPreset, 0, SwitchType::NA, 0, 18, 0);
  }
  loadPreset(0);
}
byte pres = 0;
void loop()
{
  // -- listen and read
  MIDICoreUSB.read();
  MIDICoreSerial.read();

  // -- any button logic can go here --
  button1.read();
  button2.read();
  button3.read();
  button4.read();

  /*
  delay(1500);
  loadPreset(pres);

  pres++;
  if (pres >= 127)
    pres = 0;
  */
}

// -- button handlers --
void buttonPressed()
{
  digitalWrite(LED_PIN, HIGH); // LED flash
  printPreset(CURRENT_PRESET);
}

static void onControlChange(byte channel, byte number, byte value)
{
  xprintf("ControlChange from channel: %d, number: %d, value: %d\n", channel, number, value);
}

static void onProgramChange(byte channel, byte number)
{
  xprintf("ProgramChange from channel: %d, number: %d\n", channel, number);
}

// echo all messages over usb
static void onMessageUSB(const MidiMessage &message)
{
  // xprintf("USB: ");
  // printHexArray(message, message.length);
  if (midiThru)
    MIDICoreSerial.send(message);
}

// send midi thru
static void onMessageSerial(const MidiMessage &message)
{
  // xprintf("Serial: ");
  // printHexArray(message.data(), message.length());
  if (midiThru)
    MIDICoreSerial.send(message);
}

static void sendControlChange(byte channel, byte number, byte value)
{
  xprintf("Sending ControlChange: CH %d, CC %d, VAL %d\n", channel, number, value);
  MIDICoreUSB.sendControlChange(channel, number, value);
  MIDICoreSerial.sendControlChange(channel, number, value);
}

static void sendProgramChange(byte channel, byte number)
{
  xprintf("Sending ProgramChange: CH %d, PC %d\n", channel, number);
  MIDICoreUSB.sendProgramChange(channel, number);
  MIDICoreSerial.sendProgramChange(channel, number);
}

// -- message creation
void writeSlotToMemory(byte preset, byte slot, Action msgType, byte trigger, byte switchNum, byte switchType, byte channel, byte data1, byte data2)
{
  // combine switch number and type (4 bits each)
  // left nybble is switch number, right nybble is switch type
  byte switchNumType = (switchNum << 4) | switchType;

  // write to memory
  MEMORY[preset][slot][0] = msgType;
  MEMORY[preset][slot][1] = trigger;
  MEMORY[preset][slot][2] = switchNumType;
  MEMORY[preset][slot][3] = channel;
  MEMORY[preset][slot][4] = data1;
  MEMORY[preset][slot][5] = data2;
}

void readSlotFromMemory(byte preset, byte slot, byte *msgType, byte *trigger, byte *switchNum, byte *switchType, byte *channel, byte *data1, byte *data2)
{
  *msgType = MEMORY[preset][slot][0];
  *trigger = MEMORY[preset][slot][1];
  *switchNum = leftNybble(MEMORY[preset][slot][2]);
  *switchType = rightNybble(MEMORY[preset][slot][2]);
  *channel = MEMORY[preset][slot][3];
  *data1 = MEMORY[preset][slot][4];
  *data2 = MEMORY[preset][slot][5];
}
void printSlot(byte preset, byte slot)
{
  byte msgType, trigger, switchNum, switchType, channel, data1, data2;
  readSlotFromMemory(preset, slot, &msgType, &trigger, &switchNum, &switchType, &channel, &data1, &data2);
  xprintf("Preset [%d][%d]: %d %d %d %d %d %d %d\n", preset, slot, msgType, trigger, switchNum, switchType, channel, data1, data2);
}

/*
Route to the correct action
*/
void action(byte msgType, byte channel, byte data1, byte data2)
{
  // apply action
  switch (msgType)
  {
  case Action::ControlChange:
    sendControlChange(channel, data1, data2);
    break;
  case Action::ProgramChange:
    sendProgramChange(channel, data1);
    break;
  default:
    break;
  }
}

/*
Read a preset from memory
*/
void readPresetFromMemory(byte preset, byte *data)
{
  for (int i = 0; i < SLOTS_PER_PRESET; i++)
  {
    for (int j = 0; j < 6; j++)
    {
      data[i * 6 + j] = MEMORY[preset][i][j];
    }
  }
}

/*
Load preset from memory and apply required EnterPreset and ExitPreset actions
*/
void loadPreset(byte preset)
{

  xprintf("Leaving preset %d\n", CURRENT_PRESET);

  // apply ExitPreset actions from CURRENT_PRESET
  for (int i = 0; i < SLOTS_PER_PRESET; i++)
  {
    byte msgType, trigger, switchNum, switchType, channel, data1, data2;
    readSlotFromMemory(CURRENT_PRESET, i, &msgType, &trigger, &switchNum, &switchType, &channel, &data1, &data2);
    if (trigger == Trigger::ExitPreset)

      action(msgType, channel, data1, data2);
  }

  CURRENT_PRESET = preset;
  xprintf("Loading preset %d\n", preset);

  // apply EnterPreset actions from CURRENT_PRESET
  for (int i = 0; i < SLOTS_PER_PRESET; i++)
  {
    byte msgType, trigger, switchNum, switchType, channel, data1, data2;
    readSlotFromMemory(CURRENT_PRESET, i, &msgType, &trigger, &switchNum, &switchType, &channel, &data1, &data2);
    if (trigger == Trigger::EnterPreset)
      action(msgType, channel, data1, data2);
  }
}

/*
Using CURRENT_PRESET, execute the action for the switchNum
*/
void executeSwitchAction(Trigger trigger, byte switchNum)
{
  for (int i = 0; i < SLOTS_PER_PRESET; i++)
  {
    byte msgType, slotTrigger, slotSwitchNum, switchType, channel, data1, data2;
    readSlotFromMemory(CURRENT_PRESET, i, &msgType, &slotTrigger, &slotSwitchNum, &switchType, &channel, &data1, &data2);
    if (slotTrigger == trigger && slotSwitchNum == switchNum)
      action(msgType, channel, data1, data2);
  }
}

/*
Print preset from memory, omitting empty slots
*/
void printPreset(byte preset)
{
  xprintf("Preset %03d:", preset);
  byte printed = 0;
  byte data[SLOTS_PER_PRESET * 6];
  readPresetFromMemory(preset, data);
  for (int i = 0; i < SLOTS_PER_PRESET; i++)
  {
    // check if empty
    byte empty[6] = {0, 0, 0, 0, 0, 0};
    if (memcmp(data + i * 6, empty, 6) == 0)
      continue;
    else
    {
      xprintf("\n [%02d]: ", i);
      printHexArray(data + i * 6, 6);
      printed++;
    }
  }
  if (printed == 0)
    xprintf(" (empty)");
  else
    xprintf("\n %d/%d slots used", printed, SLOTS_PER_PRESET);

  xprintf("\n");
}

// -- utility
/*
Take 8 bytes and convert them to a 64 bit integer for storage
*/
uint64_t bytesTo64(byte b1, byte b2, byte b3, byte b4, byte b5, byte b6, byte b7, byte b8)
{
  return ((uint64_t)b1 << 56) | ((uint64_t)b2 << 48) | ((uint64_t)b3 << 40) | ((uint64_t)b4 << 32) | ((uint64_t)b5 << 24) | ((uint64_t)b6 << 16) | ((uint64_t)b7 << 8) | (uint64_t)b8;
}

// convert 64 bit integer to 8 bytes
void uint64ToBytes(uint64_t x, byte *b)
{
  for (int i = 7; i >= 0; i--)
  {
    b[i] = (x >> (i * 8)) & 0xFF;
  }
}

/*
Take a 64 bit integer and print it to 8 bytes
*/
void printHex64Bits(uint64_t x)
{
  // print each byte
  for (int i = 7; i >= 0; i--)
  {
    byte b = (x >> (i * 8)) & 0xFF;
    if (b < 16)
      Serial.print('0');
    Serial.print(b, HEX);
    if (i > 0)
      Serial.print(' ');
  }
  Serial.println();
}
/*
Print an array of bytes in hex
*/
void printHexArray(const byte *data, unsigned int size)
{
  while (size > 0)
  {
    byte b = *data++;
    if (b < 16)
      Serial.print('0');
    Serial.print(b, HEX);
    if (size > 1)
      Serial.print(' ');
    size = size - 1;
  }
}

/*
Printf for arduino
*/
void xprintf(const char *format, ...)
{
  char buffer[256]; // or smaller or static &c.
  va_list args;
  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);
  Serial.print(buffer);
}

void flashLed()
{
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
}

byte leftNybble(const byte b)
{
  return b >> 4;
}

byte rightNybble(const byte b)
{
  return b & 0x0F;
}

byte randomByte(byte min, byte max)
{
  return random(min, max + 1);
}

// sysex
// -- message handlers --
bool checkVendor(const byte *data, unsigned int size)
{
  if (size < 5) // if not long enough to contain the needed bytes, then it doesn't match
    return false;
  return (*(data + 1) == 0x00 && *(data + 2) == MFID_1 && *(data + 3) == MFID_2);
}

static void onSystemExclusive(byte *data, unsigned int length)
{
  // check last byte in msg - if it is F0, then more is coming
  // if it is F7, it is the last
  byte lastBit = *(data + length - 1); // last byte in data

  // assign last bit
  bool last = false;
  if (lastBit == 0xF7)
    last = true;

  // print
  xprintf("SysEx Message (%dB): ", length);
  printHexArray(data, length);
  if (last)
  {
    Serial.println(F(" (end)"));
  }
  else
  {
    Serial.println(F(" (tbc)"));
  }

  // ExitPreset if not last bit
  if (last && checkVendor(data, length))
    parseSysExMessage(data, length);
  else
    Serial.println(F("Unexplained issue! The SysEx message is not getting processed"));
}

static void parseSysExMessage(byte *data, unsigned int length)
{
  unsigned int start = 5;
  byte operation = leftNybble(*(data + start));
  byte saveToROM = rightNybble(*(data + start));
  int segmentLength = length - start;
  /*
  if (false)
  {
  }
  else
  {
    Serial.print(F("Unsupported operation for segment length: "));
    Serial.print(operation, HEX);
    Serial.print(F(", length "));
    Serial.println(segmentLength);
  }
  */
}