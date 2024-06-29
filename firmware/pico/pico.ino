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
#define BYTES_PER_SLOT 7
#define SLOTS_PER_PRESET 16
#define NUM_PRESETS 128
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
#define NA 0

/*
ALL BIT LOGIC IS INDEXED FROM LEFT TO RIGHT
*/

// main storage
// first slot may look like [32, 0, ...] - switch type (bit in order) latching, switch state (for latching),
// first slot holds metadata for preset
byte MEMORY[NUM_PRESETS][SLOTS_PER_PRESET + 1][BYTES_PER_SLOT]; // NUM_PRESETS presets, SLOTS_PER_PRESET slots, bytes each
byte CURRENT_PRESET = 0;

// enums
// NONE of these can be 0

/*
  Action type
  8 bits
*/
enum Action
{
  ControlChange = 0x01,
  ProgramChange = 0x02,
  PresetUp = 0x03,   // on stegosaurus
  PresetDown = 0x04, // on stegosaurus
};

/*
  Trigger type
  4 bits
*/
enum Trigger
{
  EnterPreset = 0x1,
  ExitPreset = 0x2,
  ShortPress = 0x3,
  LongPress = 0x4,
  DoublePress = 0x5,
};

/*
  Switch type
  4 bits
*/
enum SwitchType
{
  Momentary = 0,
  Latch = 1,
};

/*
  Switch number
  4 bits
*/
// DO NOT CHANGE THESE VALUES. They must climb and be and SW_OFFSET away from 0.
const byte SW_OFFSET = 1;
enum Switch
{
  SW1 = 1, // DON'T CHANGE
  SW2 = 2, // DON'T CHANGE
  SW3 = 3, // DON'T CHANGE
  SW4 = 4, // DON'T CHANGE
};

// declare defaults
void writeMidiSlotToMemory(byte preset, byte slot, byte msgType, byte trigger, byte switchNum, byte channel, byte data1 = NA, byte data2 = NA, byte data3 = NA);

// switchs
EasyButton switch1(SWITCH_1_PIN, DEBOUNCE, PULLUP, true);
EasyButton switch2(SWITCH_2_PIN, DEBOUNCE, PULLUP, true);
EasyButton switch3(SWITCH_3_PIN, DEBOUNCE, PULLUP, true);
EasyButton switch4(SWITCH_4_PIN, DEBOUNCE, PULLUP, true);

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
  MIDICoreSerial.setHandleMessage(onMessageSerial);

  // set to receieve all channels - in configuration, filter them out
  MIDICoreUSB.begin(MIDI_CHANNEL_OMNI);
  MIDICoreSerial.begin(MIDI_CHANNEL_OMNI); // receive on all channels

  // wait until device mounted
  while (!TinyUSBDevice.mounted())
    delay(1);

  // ------------------- set up switchs -------------------
  switch1.onPressedFor(SHORT_PRESS_DURATION, switch1Pressed);
  switch2.onPressedFor(SHORT_PRESS_DURATION, switch2Pressed);
  switch3.onPressedFor(SHORT_PRESS_DURATION, switch3Pressed);
  switch4.onPressedFor(SHORT_PRESS_DURATION, switch4Pressed);

  // begin switchs
  switch1.begin();
  switch2.begin();
  switch3.begin();
  switch4.begin();

  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB
  }

  // ------------------- set up memory -------------------
  // set all memory to 0
  memset(MEMORY, NA, sizeof(MEMORY));

  // populate memory with some defaults
  for (int i = 0; i < NUM_PRESETS; i++)
  {
    // set switch types
    setSwitchTypes(i, 0b00010000);  // switch 4 is latch
    setSwitchStates(i, 0b00010000); // all switches are low
    // set midi slots
    writeMidiSlotToMemory(i, 1, Action::PresetDown, Trigger::ShortPress, Switch::SW1, NA, NA, NA, NA);   // stego preset
    writeMidiSlotToMemory(i, 2, Action::PresetUp, Trigger::ShortPress, Switch::SW2, NA, NA, NA, NA);     // stego preset
    writeMidiSlotToMemory(i, 3, Action::ProgramChange, Trigger::EnterPreset, NA, 15, i, NA, NA);         // hx preset
    writeMidiSlotToMemory(i, 4, Action::ControlChange, Trigger::ShortPress, Switch::SW3, 15, 69, 8, NA); // hx snapshot down
    writeMidiSlotToMemory(i, 5, Action::ControlChange, Trigger::ShortPress, Switch::SW4, 15, 69, 9, 1);  // hx snapshot up
  }
  loadPreset(CURRENT_PRESET);
  printPreset(CURRENT_PRESET);
}
byte pres = 0;
void loop()
{
  // -- listen and read
  MIDICoreUSB.read();
  MIDICoreSerial.read();

  // -- any switch logic can go here --
  switch1.read();
  switch2.read();
  switch3.read();
  switch4.read();
}

// -- switch handlers --
void switch1Pressed()
{
  xprintf("[SW1] pressed\n");
  executeSwitchAction(Trigger::ShortPress, Switch::SW1);
}

void switch2Pressed()
{
  xprintf("[SW2] pressed\n");
  executeSwitchAction(Trigger::ShortPress, Switch::SW2);
}

void switch3Pressed()
{
  xprintf("[SW3] pressed\n");
  executeSwitchAction(Trigger::ShortPress, Switch::SW3);
}

void switch4Pressed()
{
  xprintf("[SW4] pressed\n");
  executeSwitchAction(Trigger::ShortPress, Switch::SW4);
}

/*
Using CURRENT_PRESET, execute the action for the switchNum
*/
void executeSwitchAction(Trigger trigger, byte switchNum)
{
  if (getSwitchType(CURRENT_PRESET, switchNum) == SwitchType::Latch)
  {
    bool currentState = getSwitchState(CURRENT_PRESET, switchNum);
    xprintf("Switch %d is %s\n", switchNum, currentState ? "HIGH" : "LOW");
    setSwitchState(CURRENT_PRESET, switchNum, !currentState);
    bool newState = getSwitchState(CURRENT_PRESET, switchNum);
    xprintf("Switch %d is now %s\n", switchNum, newState ? "HIGH" : "LOW");
  }
  // xprintf("Switch %d is %s\n", switchNum, getSwitchState(CURRENT_PRESET, switchNum) ? "HIGH" : "LOW");

  // iterate over slots
  for (int i = 0; i < SLOTS_PER_PRESET; i++)
  {
    // read the preset
    byte msgType, slotTrigger, slotSwitchNum, channel, data1, data2, data3;
    readSlotFromMemory(CURRENT_PRESET, i, &msgType, &slotTrigger, &slotSwitchNum, &channel, &data1, &data2, &data3);

    // check if type is latch
    if (slotTrigger == trigger && slotSwitchNum == switchNum)
      if (getSwitchType(CURRENT_PRESET, switchNum) == SwitchType::Latch)
      {
        // check switch state and execute
        if (getSwitchState(CURRENT_PRESET, switchNum) == 0)
          action(msgType, channel, data1, data2); // data2 is the value to latch to
        else
          action(msgType, channel, data1, data3); // data3 is the value to return to
      }
      // if momentary, just execute
      else
        action(msgType, channel, data1, data2);
  }
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
  xprintf("OUT: ControlChange: CH %d, CC %d, VAL %d\n", channel, number, value);
  MIDICoreUSB.sendControlChange(number, value, channel);
  MIDICoreSerial.sendControlChange(number, value, channel);
}

static void sendProgramChange(byte channel, byte number)
{
  xprintf("OUT: ProgramChange: CH %d, PC %d\n", channel, number);
  MIDICoreUSB.sendProgramChange(number, channel);
  MIDICoreSerial.sendProgramChange(number, channel);
}

// -- message creation
/*
data1, data2, data3 are optional
For program change, data1 is the program number.
For control change, data1 is the control number, data2 is the value when pressed.
Data3 is only used for msgType = ControlChange and switchType = Latch. Data2 is the value to latch to, data3 is the value to return to.
*/
void writeMidiSlotToMemory(byte preset, byte slot, byte msgType, byte trigger, byte switchNum, byte channel, byte data1, byte data2, byte data3)
{

  // xprintf("Writing to memory: %d %d %d %d %d %d %d %d %d\n", msgType, trigger, switchNum, switchType, channel, data1, data2, data3);
  // check validity of data
  if (preset >= NUM_PRESETS || slot >= SLOTS_PER_PRESET)
  {
    xprintf("Preset/slot OOB!\n");
    return;
  }
  if (slot == 0)
  {
    xprintf("Slot 0 is reserved for preset metadata!\n");
    return;
  }

  // write to memory
  MEMORY[preset][slot][0] = msgType;
  MEMORY[preset][slot][1] = trigger;
  MEMORY[preset][slot][2] = switchNum;
  MEMORY[preset][slot][3] = channel;
  MEMORY[preset][slot][4] = data1;
  MEMORY[preset][slot][5] = data2;
  MEMORY[preset][slot][6] = data3;
}

// write switch states (momentary or latch) to memory
// updateSwitchState (for momentary)
/*
Set the switch states for a preset
This is encoded as a byte, with each bit representing a switch type.
0 = momentary, 1 = latch.
This is enoded from LEFT to RIGHT, so SW1 is the leftmost bit.
For example, 0b00100000 would mean that SW3 is a latch switch, and the rest are momentary.
*/
void setSwitchTypes(byte preset, byte types)
{
  MEMORY[preset][0][0] = types; // first byte of first slot is switch types
}

/*
Update a single switch tyoe for a preset
Example: setSwitchType(0, Switch:SW1, SwitchType::Latch) would set SW1 to latch.
*/
void setSwitchType(byte preset, byte switchNum, byte type)
{
  setBit(MEMORY[preset][0][0], switchNum - SW_OFFSET, type);
}
/*
Set a single switch state for a preset.
Example: setSwitchState(0, Switch:SW1, 1) would set SW1 to HIGH.
*/
void setSwitchState(byte preset, byte switchNum, byte state)
{
  setBit(MEMORY[preset][0][1], switchNum - SW_OFFSET, state);
}

/*
Get the switch type for a specific switch in a preset
Example: getSwitchType(0, Switch::SW1) would return SwitchType::Latch if SW1 is a latch switch.
*/
byte getSwitchType(byte preset, byte switchNum)
{
  return isBitSet(MEMORY[preset][0][0], switchNum - SW_OFFSET);
}

/*
Get the switch state for a specific switch in a preset
Example: getSwitchState(0, Switch::SW1) would return 1 if SW1 is HIGH.
*/
byte getSwitchState(byte preset, byte switchNum)
{
  return isBitSet(MEMORY[preset][0][1], switchNum - SW_OFFSET);
}

/*
This is encoded as a byte, with each bit representing a switch state.
It's encoded from LEFT to RIGHT, so SW1 is the leftmost bit.
For example, 0b10000000 would mean that SW1 is HIGH.
This is only used for latch switches, momentary switches are always LOW in this byte.
*/
void setSwitchStates(byte preset, byte states)
{
  // check validity of data against switch types
  byte types = MEMORY[preset][0][0];
  // if a bit at position i is 1, then the switch at position i must be a latch switch
  // if that's not the case, then the data is invalid
  for (int i = 0; i < 8; i++)
    if (isBitSet(states, i) && !isBitSet(types, i))
    {
      xprintf("Switch state data is invalid!\n");
      return;
    }
  MEMORY[preset][0][1] = states; // second byte of first slot is switch states
}

/*
Reset all switch states to 0
*/
void resetSwitchStates(byte preset)
{
  MEMORY[preset][0][1] = 0; // second byte of first slot is switch states
}

/*
Read a slot from memory
*/
void readSlotFromMemory(byte preset, byte slot, byte *msgType, byte *trigger, byte *switchNum, byte *channel, byte *data1, byte *data2, byte *data3)
{
  *msgType = MEMORY[preset][slot][0];
  *trigger = MEMORY[preset][slot][1];
  *switchNum = MEMORY[preset][slot][2];
  *channel = MEMORY[preset][slot][3];
  *data1 = MEMORY[preset][slot][4];
  *data2 = MEMORY[preset][slot][5];
  *data3 = MEMORY[preset][slot][6];
}

/*
Print a slot from memory
*/
void printSlot(byte preset, byte slot)
{
  byte msgType, trigger, switchNum, channel, data1, data2, data3;
  readSlotFromMemory(preset, slot, &msgType, &trigger, &switchNum, &channel, &data1, &data2, &data3);
  xprintf("Preset [%d][%d]: %d %d %d %d %d %d %d %d\n", preset, slot, msgType, trigger, switchNum, channel, data1, data2, data3);
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
  case Action::PresetUp:
    loadPreset(CURRENT_PRESET + 1);
    break;
  case Action::PresetDown:
    loadPreset(CURRENT_PRESET - 1);
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
    for (int j = 0; j < BYTES_PER_SLOT; j++)
    {
      data[i * BYTES_PER_SLOT + j] = MEMORY[preset][i][j];
    }
  }
}

/*
Load preset from memory and apply required EnterPreset and ExitPreset actions
*/
void loadPreset(byte preset)
{
  // check bounds
  if (CURRENT_PRESET == 0 && preset >= NUM_PRESETS - 1)
    preset = NUM_PRESETS - 1;
  else if (preset >= NUM_PRESETS)
    preset = 0;

  // apply ExitPreset actions from CURRENT_PRESET
  for (int i = 0; i < SLOTS_PER_PRESET; i++)
  {
    byte msgType, trigger, switchNum, channel, data1, data2, data3;
    readSlotFromMemory(CURRENT_PRESET, i, &msgType, &trigger, &switchNum, &channel, &data1, &data2, &data3);
    if (trigger == Trigger::ExitPreset)
      action(msgType, channel, data1, data2);
  }

  // don't forget to reset latches and switch states

  CURRENT_PRESET = preset;
  xprintf("Load preset %d\n", preset);
  printPreset(preset);

  // apply EnterPreset actions from CURRENT_PRESET
  for (int i = 0; i < SLOTS_PER_PRESET; i++)
  {
    byte msgType, trigger, switchNum, channel, data1, data2, data3;
    readSlotFromMemory(CURRENT_PRESET, i, &msgType, &trigger, &switchNum, &channel, &data1, &data2, &data3);
    if (trigger == Trigger::EnterPreset)
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
  byte data[SLOTS_PER_PRESET * BYTES_PER_SLOT];
  readPresetFromMemory(preset, data);

  byte switches = MEMORY[preset][0][0];
  byte states = MEMORY[preset][0][1];

  // print meta slot (byte 0 switches, byte 1 states)
  xprintf("\n SW_TYPES:  ");
  // print M if momentary, L if latch
  for (int i = 0; i < 8; i++)
    xprintf("%c ", isBitSet(switches, i) ? 'L' : 'M');

  xprintf("\n SW_STATES: ");
  // print 1 if HIGH, 0 if LOW
  for (int i = 0; i < 8; i++)
    xprintf("%d ", isBitSet(states, i));

  // print all slots
  for (int i = 1; i < SLOTS_PER_PRESET; i++)
  {
    // check if empty
    byte empty[BYTES_PER_SLOT];
    memset(empty, 0, BYTES_PER_SLOT);

    if (memcmp(data + i * BYTES_PER_SLOT, empty, BYTES_PER_SLOT) == 0)
      continue;
    else
    {
      xprintf("\n [%02d]: ", i);
      printHexArray(data + i * BYTES_PER_SLOT, BYTES_PER_SLOT);
      printed++;
    }
  }
  if (printed == 0)
    xprintf(" (empty)");
  else
    xprintf("\n %d/%d slots used", printed, SLOTS_PER_PRESET);

  xprintf("\n");
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

void printBinaryByte(byte b)
{
  Serial.print("0b");
  for (int i = 7; i >= 0; i--)
  {
    Serial.print(bitRead(b, i));
  }
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
  xprintf("IN: SysEx (%d): ", length);
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
  else if (!checkVendor(data, length))
  {
    // do nothing
  }
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

// get bit at position pos from the left
bool isBitSet(byte b, byte position)
{
  // Check if position is within valid range
  if (position < 0 || position > 7)
    return false;

  // Calculate the bit position from the right
  int bitPositionFromRight = 7 - position;

  // Use bitwise operations to find the bit value
  return (b & (1 << bitPositionFromRight)) != 0;
}

// set bit to value at position pos from the left
void setBit(byte &b, byte position, byte value)
{
  // Check if position is within valid range
  if (position < 0 || position > 7)
    return;

  // Calculate the bit position from the right
  int bitPositionFromRight = 7 - position;

  // Use bitwise operations to set the bit value
  if (value == 1)
    b |= (1 << bitPositionFromRight);
  else
    b &= ~(1 << bitPositionFromRight);
}