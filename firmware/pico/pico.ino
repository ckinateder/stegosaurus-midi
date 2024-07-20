#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#include <EasyButton.h>

// product info
#define VSN "0.1.3"
#define PRODUCT "Stegosaurus"
#define MANUFACTURER "CJK Devices"

// sysex
#define MFID_1 0x53
#define MFID_2 0x4d

// settings
#define SAVE_TO_EEPROM 1
#define MIDICoreUSB_Cable 0 // this corresponds to the virtual "port" or "cable". stick to 0.

// switch press durations
#define SHORT_PRESS_DURATION 100
#define LONG_PRESS_DURATION 750

// button settings
#define PULLUP true
#define DEBOUNCE 40

// Switches
#define NUM_SWITCHES 4
#define SWITCH_1_PIN 10
#define SWITCH_2_PIN 11
#define SWITCH_3_PIN 12
#define SWITCH_4_PIN 13

// LED Builtin
#define LED_PIN 13

// memory settings
#define NUM_PRESETS 128
#define BYTES_PER_SLOT 7
#define SLOTS_PER_PRESET 17

// switch memory location
#define META_SLOT 0
#define SW_TYPE_BYTE 0
#define SW_STATE_BYTE 1

// switch values
#define MOMENTARY 0
#define LATCH 1

// hardcoded midi values
#define ALL_CHANNELS 16
#define NA 0 // switch to 0xFF?

/*
ALL BIT LOGIC IS INDEXED FROM LEFT TO RIGHT
*/

// main storage
// first slot may look like [32, 0, ...] - switch type (bit in order) latching, switch state (for latching),
// first slot holds metadata for preset - EXPLORE IN DOCS
byte MEMORY[NUM_PRESETS][SLOTS_PER_PRESET][BYTES_PER_SLOT]; // NUM_PRESETS presets, SLOTS_PER_PRESET slots, bytes each

// the main memory is never modified, only copied from and to
// the current preset memory is modified in real time
// if a preset is loaded, the current preset memory is copied from the main memory
// if a preset is saved, the current preset memory is copied to the main memory
byte CURRENT_PRESET_MEMORY[SLOTS_PER_PRESET][BYTES_PER_SLOT];
byte CURRENT_PRESET = 0;

// enums
// NONE of these can be 0

/*
  Action type
  8 bits
*/
enum Action
{
  ControlChange = 1,
  ProgramChange = 2,
  PresetUp = 3,   // on stegosaurus
  PresetDown = 4, // on stegosaurus
};

/*
  Trigger type
  4 bits
*/
enum Trigger
{
  EnterPreset = 1,
  ExitPreset = 2,
  ShortPress = 3,
  LongPress = 4,
  DoublePress = 5,
};

/*
  Switch number
  4 bits
*/
// DO NOT CHANGE THESE VALUES. They must climb and start from 0
enum Switch
{
  SWA = 0, // DON'T CHANGE
  SWB = 1, // DON'T CHANGE
  SWC = 2, // DON'T CHANGE
  SWD = 3, // DON'T CHANGE
};

enum MsgTypes
{
  SetSlot = 0,              // set a slot - this will call writeSlot
  GetSlot = 1,              // get a slot - this will call readSlot
  GetSlotReturn = 2,        // return value from get slot
  SetSystemParam = 3,       // set a system param
  GetSystemParam = 4,       // get a system param
  GetSystemParamReturn = 5, // return value from get system param
  GetPreset = 6,            // get a preset
  GetPresetReturn = 7,      // return value from get preset
};

enum SystemParams
{
  CurrentPreset = 0, // the current preset
};

// declare defaults
void writeSlot(byte slot, byte trigger, byte action, byte switchNum, byte channel, byte data1 = NA, byte data2 = NA, byte data3 = NA);

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
  USBDevice.setManufacturerDescriptor(MANUFACTURER);
  USBDevice.setProductDescriptor(PRODUCT);
  USB_MIDI_Transport.setStringDescriptor(PRODUCT);
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
    ; // wait for serial port to connect. Needed for native USB

  // ------------------- set up memory -------------------
  // set all memory to 0
  memset(MEMORY, NA, sizeof(MEMORY));

  // populate memory with some defaults
  for (int p = 0; p < NUM_PRESETS; p++)
  {
    // this enables the temporary memory to be used
    setCurrentPreset(p); // set current preset

    // set switch types
    setSwitchType(Switch::SWD, LATCH);
    resetSwitchStates(); // all switches are low

    setSwitchState(Switch::SWD, 1);

    // set midi slots
    writeSlot(1, Trigger::ShortPress, Action::PresetDown, Switch::SWA, NA, NA, NA, NA);    // stego preset
    writeSlot(2, Trigger::ShortPress, Action::PresetUp, Switch::SWB, NA, NA, NA, NA);      // stego preset
    writeSlot(3, Trigger::EnterPreset, Action::ProgramChange, NA, 15, p, NA, NA);          // hx preset
    writeSlot(4, Trigger::ShortPress, Action::ControlChange, Switch::SWC, 15, 69, 8, NA);  // hx snapshot down
    writeSlot(5, Trigger::ShortPress, Action::ControlChange, Switch::SWD, 15, 74, 127, 0); // latch test
    savePreset(p);
  }
  loadPreset(CURRENT_PRESET);
  printPreset(CURRENT_PRESET);
}

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
  xprintf("[SWA] pressed\n");
  executeSwitchAction(Trigger::ShortPress, Switch::SWA);
}

void switch2Pressed()
{
  xprintf("[SWB] pressed\n");
  executeSwitchAction(Trigger::ShortPress, Switch::SWB);
}

void switch3Pressed()
{
  xprintf("[SWC] pressed\n");
  executeSwitchAction(Trigger::ShortPress, Switch::SWC);
}

void switch4Pressed()
{
  xprintf("[SWD] pressed\n");
  executeSwitchAction(Trigger::ShortPress, Switch::SWD);
}

/**
 * @brief Execute a switch action given a trigger and switch number
 *
 * @param trigger
 * @param switchNum
 */
void executeSwitchAction(Trigger trigger, byte switchNum)
{
  if (getSwitchType(switchNum) == LATCH)
  {
    bool currentState = getSwitchState(switchNum);
    xprintf("Switch %d is %s\n", switchNum, currentState ? "HIGH" : "LOW");
    setSwitchState(switchNum, !currentState);
    bool newState = getSwitchState(switchNum);
    xprintf("Switch %d is now %s\n", switchNum, newState ? "HIGH" : "LOW");
  }
  // xprintf("Switch %d is %s\n", switchNum, getSwitchState( switchNum) ? "HIGH" : "LOW");

  // iterate over slots, skipping slot 0
  for (int i = 1; i < SLOTS_PER_PRESET; i++)
  {
    // read the preset
    byte slotTrigger, action, slotSwitchNum, channel, data1, data2, data3;
    readSlot(i, &slotTrigger, &action, &slotSwitchNum, &channel, &data1, &data2, &data3);

    // check if type is latch
    if (slotTrigger == trigger && slotSwitchNum == switchNum)
      if (getSwitchType(switchNum) == LATCH)
      {
        // check switch state and execute
        if (getSwitchState(switchNum) == 0)
          executeAction(action, channel, data1, data2); // data2 is the value to latch to
        else
          executeAction(action, channel, data1, data3); // data3 is the value to return to
      }
      // if momentary, just execute
      else
        executeAction(action, channel, data1, data2);
  }
}

/**
 * @brief On receiving control change message
 *
 * @param channel
 * @param number
 * @param value
 */
static void onControlChange(byte channel, byte number, byte value)
{
  xprintf("ControlChange from channel: %d, number: %d, value: %d\n", channel, number, value);
}

/**
 * @brief On receiving program change message
 *
 * @param channel
 * @param number
 */
static void onProgramChange(byte channel, byte number)
{
  xprintf("ProgramChange from channel: %d, number: %d\n", channel, number);
}

/**
 * @brief on receiving a message from USB
 *
 * @param message
 */
static void onMessageUSB(const MidiMessage &message)
{
  // xprintf("USB: ");
  // printHexArray(message, message.length);
  if (midiThru)
    MIDICoreSerial.send(message);
}

/**
 * @brief on receiving a message from serial
 *
 * @param message
 */
static void onMessageSerial(const MidiMessage &message)
{
  // xprintf("Serial: ");
  // printHexArray(message.data(), message.length());
  if (midiThru)
    MIDICoreSerial.send(message);
}

/**
 * @brief Semd a control change message
 *
 * @param channel
 * @param number
 * @param value
 */
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

void sendSystemExclusive(byte *data, unsigned int length)
{
  // prepend MFID_ARRAY to data
  unsigned int fullLength = length + 5;
  byte fullData[fullLength];
  fullData[0] = 0xF0;
  fullData[1] = 0x00;
  fullData[2] = MFID_1;
  fullData[3] = MFID_2;
  memcpy(fullData + 4, data, length);
  fullData[length + 4] = 0xF7;

  xprintf("OUT: SysEx (%d): ", fullLength);
  printHexArray(fullData, fullLength);
  xprintf("\n");

  // send the message
  // sendSysEx (int length, const byte *const array, bool ArrayContainsBoundaries=false)
  MIDICoreUSB.sendSysEx(fullLength, fullData, true);
  MIDICoreSerial.sendSysEx(fullLength, fullData, true);
}
// -- message creation

/**
 * @brief Write a slot to the current preset memory
 *  The first slot is reserved for metadata.
 * @param slot
 * @param trigger
 * @param action
 * @param switchNum
 * @param channel
 * @param data1
 * @param data2
 * @param data3
 */
void writeSlot(byte slot, byte trigger, byte action, byte switchNum, byte channel, byte data1, byte data2, byte data3)
{
  // validation
  if (slot == META_SLOT)
  {
    xprintf("Slot %d is reserved for preset metadata!\n", slot);
    return;
  }
  if (slot >= SLOTS_PER_PRESET)
  {
    xprintf("Slot %d is out of bounds!\n", slot);
    return;
  }
  if (channel > ALL_CHANNELS)
  {
    xprintf("Channel %d is out of bounds!\n", channel);
    return;
  }
  if (switchNum >= NUM_SWITCHES)
  {
    xprintf("Switch %d is out of bounds!\n", switchNum);
    return;
  }
  if (data1 > 127 || data2 > 127 || data3 > 127)
  {
    xprintf("Data is out of bounds!\n");
    return;
  }

  // write to memory
  CURRENT_PRESET_MEMORY[slot][0] = trigger;
  CURRENT_PRESET_MEMORY[slot][1] = action;
  CURRENT_PRESET_MEMORY[slot][2] = switchNum;
  CURRENT_PRESET_MEMORY[slot][3] = channel;
  CURRENT_PRESET_MEMORY[slot][4] = data1;
  CURRENT_PRESET_MEMORY[slot][5] = data2;
  CURRENT_PRESET_MEMORY[slot][6] = data3;
}

/**
 * @brief Set the Switch Types
 * Set the switch states for a preset
 * This is encoded as a byte, with each bit representing a switch type.
 * 0 = momentary, 1 = latch. This is enoded from LEFT to RIGHT, so SWA is the leftmost bit.
 * For example, 0b00100000 would mean that SWC is a latch switch, and the rest are momentary.
 * @param types
 */
void setSwitchTypes(byte types)
{
  CURRENT_PRESET_MEMORY[META_SLOT][SW_TYPE_BYTE] = types; // first byte of first slot is switch types
}

/**
 * @brief Get the Switch Types
 * See the setSwitchTypes function for more information
 * @return byte
 */
byte getSwitchTypes()
{
  return CURRENT_PRESET_MEMORY[META_SLOT][SW_TYPE_BYTE];
}

/*
This is encoded as a byte, with each bit representing a switch state.
It's encoded from LEFT to RIGHT, so SWA is the leftmost bit.
For example, 0b10000000 would mean that SWA is HIGH.
This is only used for latch switches, momentary switches are always LOW in this byte.
*/

/**
 * @brief Set the Switch States byte
 * This is encoded as a byte, with each bit representing a switch state.
 * It's encoded from LEFT to RIGHT, so SWA is the leftmost bit.
 * For example, 0b10000000 would mean that SWA is HIGH.
 * This is only used for latch switches, momentary switches are always LOW in this byte.
 * @param states
 */
void setSwitchStates(byte states)
{
  // check validity of data against switch types
  byte types = CURRENT_PRESET_MEMORY[META_SLOT][SW_TYPE_BYTE];
  // if a bit at position i is 1, then the switch at position i must be a latch switch
  // if that's not the case, then the data is invalid
  for (int i = 0; i < 8; i++)
    if (isBitSet(states, i) && !isBitSet(types, i))
    {
      xprintf("Switch state data is invalid!\n");
      return;
    }
  CURRENT_PRESET_MEMORY[META_SLOT][SW_STATE_BYTE] = states; // second byte of first slot is switch states
}

/**
 * @brief Get the Switch States byte
 * This is encoded as a byte, with each bit representing a switch state.
 * It's encoded from LEFT to RIGHT, so SWA is the leftmost bit.
 * For example, 0b10000000 would mean that SWA is HIGH.
 * This is only used for latch switches, momentary switches are always LOW in this byte.
 * @return byte
 */
byte getSwitchStates()
{
  return CURRENT_PRESET_MEMORY[META_SLOT][SW_STATE_BYTE];
}

/**
 * @brief Set the Switch Type object
 * Update a single switch tyoe for a preset
 * Example: setSwitchType(0, Switch:SWA, LATCH) would set SWA to latch.
 * @param switchNum
 * @param type
 */
void setSwitchType(byte switchNum, byte type)
{
  setBit(CURRENT_PRESET_MEMORY[META_SLOT][SW_TYPE_BYTE], switchNum, type);
}

/**
 * @brief Get the Switch Type for a specific switch in a preset
 * Example: getSwitchType(0, Switch::SWA) would return LATCH if SWA is a latch switch.
 * @param switchNum
 * @return byte
 */
byte getSwitchType(byte switchNum)
{
  return isBitSet(CURRENT_PRESET_MEMORY[META_SLOT][SW_TYPE_BYTE], switchNum);
}

/**
 * @brief Set the Switch State object
 * Example: setSwitchState(0, Switch:SWA, 1) would set SWA to HIGH.
 * @param switchNum
 * @param state
 */
void setSwitchState(byte switchNum, byte state)
{
  setBit(CURRENT_PRESET_MEMORY[META_SLOT][SW_STATE_BYTE], switchNum, state);
}

/**
 * @brief Get the Switch State object
 * Example: getSwitchState(0, Switch::SWA) would return 1 if SWA is HIGH.
 * @param switchNum
 * @return byte
 */
byte getSwitchState(byte switchNum)
{
  return isBitSet(CURRENT_PRESET_MEMORY[0][SW_STATE_BYTE], switchNum);
}

/**
 * @brief Reset all switch states to 0
 *
 */
void resetSwitchStates()
{
  CURRENT_PRESET_MEMORY[META_SLOT][SW_STATE_BYTE] = 0; // second byte of first slot is switch states
}

/**
 * @brief Read a slot from memory
 * This reads from the current preset memory, not the main memory.
 * @param slot
 * @param trigger
 * @param action
 * @param switchNum
 * @param channel
 * @param data1
 * @param data2
 * @param data3
 */
void readSlot(byte slot, byte *trigger, byte *action, byte *switchNum, byte *channel, byte *data1, byte *data2, byte *data3)
{
  *trigger = CURRENT_PRESET_MEMORY[slot][0];
  *action = CURRENT_PRESET_MEMORY[slot][1];
  *switchNum = CURRENT_PRESET_MEMORY[slot][2];
  *channel = CURRENT_PRESET_MEMORY[slot][3];
  *data1 = CURRENT_PRESET_MEMORY[slot][4];
  *data2 = CURRENT_PRESET_MEMORY[slot][5];
  *data3 = CURRENT_PRESET_MEMORY[slot][6];
}

/**
 * @brief Print a slot from memory
 * This reads from the main memory, not the current preset memory.
 * @param preset
 * @param slot
 */
void printSlot(byte preset, byte slot)
{
  byte trigger, action, switchNum, channel, data1, data2, data3;
  readSlotFromMemory(preset, slot, &trigger, &action, &switchNum, &channel, &data1, &data2, &data3);
  xprintf("Preset [%d][%d]: %d %d %d %d %d %d %d %d\n", preset, slot, action, trigger, switchNum, channel, data1, data2, data3);
}

/**
 * @brief Execute an action
 * This is a wrapper for sending midi messages based on the action type in the slot.
 * @param action
 * @param channel
 * @param data1
 * @param data2
 */
void executeAction(byte action, byte channel, byte data1, byte data2)
{
  // apply action
  switch (action)
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

/**
 * @brief DEPRECATED: Read a preset from memory
 * We don't use this anymore, as we have a current preset memory.
 * @param preset
 * @param data
 */
void readPresetFromMemory(byte preset, byte *data)
{
  for (int i = 0; i < SLOTS_PER_PRESET; i++)
    for (int j = 0; j < BYTES_PER_SLOT; j++)
      data[i * BYTES_PER_SLOT + j] = MEMORY[preset][i][j];
}

/**
 * @brief Copy a preset from main memory into a 2d array
 * This is used to copy a preset from main memory into current preset memory.
 * Data is a 2d array, with the first dimension being the slot, and the second dimension being the byte.
 * @param preset
 * @param data
 */
void copyPresetFromMemory(byte preset, byte data[SLOTS_PER_PRESET][BYTES_PER_SLOT])
{
  // validate
  if (preset > NUM_PRESETS - 1)
  {
    xprintf("Preset OOB!\n");
    return;
  }
  for (int i = 0; i < SLOTS_PER_PRESET; i++)
    for (int j = 0; j < BYTES_PER_SLOT; j++)
      data[i][j] = MEMORY[preset][i][j];
}

/**
 * @brief Set the CURRENT_PRESET state variable and copy the preset from main memory into current preset memory.
 *
 * @param preset
 */
void setCurrentPreset(byte preset)
{
  if (preset > NUM_PRESETS - 1)
  {
    xprintf("Preset OOB!\n");
    return;
  }
  CURRENT_PRESET = preset;
  copyPresetFromMemory(CURRENT_PRESET, CURRENT_PRESET_MEMORY);
}

/**
 * @brief Save the current preset memory to main memory
 * This is memory safe - it only copies over the changed slots to avoid unnecessary writes.
 * @param preset
 */
void savePreset(byte preset)
{
  // check bounds
  if (preset > NUM_PRESETS - 1)
  {
    xprintf("Preset OOB!\n");
    return;
  }

  // copy current preset memory to main memory, ignoring anything unchanged
  for (int i = 0; i < SLOTS_PER_PRESET; i++)
    for (int j = 0; j < BYTES_PER_SLOT; j++)
      if (MEMORY[preset][i][j] != CURRENT_PRESET_MEMORY[i][j])
        MEMORY[preset][i][j] = CURRENT_PRESET_MEMORY[i][j];
}

/**
 * @brief Save the current preset memory to main memory
 *
 */
void savePreset()
{
  savePreset(CURRENT_PRESET);
}

/**
 * @brief Read a slot from memory
 * This reads from the main memory, not the current preset memory.
 * @param preset
 * @param slot
 * @param trigger
 * @param action
 * @param switchNum
 * @param channel
 * @param data1
 * @param data2
 * @param data3
 */
void readSlotFromMemory(byte preset, byte slot, byte *trigger, byte *action, byte *switchNum, byte *channel, byte *data1, byte *data2, byte *data3)
{
  *trigger = MEMORY[preset][slot][0];
  *action = MEMORY[preset][slot][1];
  *switchNum = MEMORY[preset][slot][2];
  *channel = MEMORY[preset][slot][3];
  *data1 = MEMORY[preset][slot][4];
  *data2 = MEMORY[preset][slot][5];
  *data3 = MEMORY[preset][slot][6];
}

/**
 * @brief Load a preset from memory
 * Apply EnterPreset and ExitPreset actions
 * Reset latch switches
 * @param preset
 */
void loadPreset(byte preset)
{
  // savePreset(CURRENT_PRESET); // save current preset

  // check bounds
  if (CURRENT_PRESET == 0 && preset >= NUM_PRESETS - 1)
    preset = NUM_PRESETS - 1;
  else if (preset >= NUM_PRESETS)
    preset = 0;

  // apply ExitPreset actions from CURRENT_PRESET
  for (int i = 1; i < SLOTS_PER_PRESET; i++)
  {
    byte trigger, action, switchNum, channel, data1, data2, data3;
    readSlot(i, &trigger, &action, &switchNum, &channel, &data1, &data2, &data3);
    if (trigger == Trigger::ExitPreset)
      executeAction(action, channel, data1, data2);

    // apply latch off if switch is a latch and on
    if (getSwitchType(switchNum) == LATCH && getSwitchState(switchNum) == 1)
      executeAction(action, channel, data1, data3);
  }

  // copy preset to current preset memory
  setCurrentPreset(preset);

  // load preset and apply EnterPreset actions
  xprintf("Load preset %d\n", preset);
  printPreset(preset);

  // apply EnterPreset actions from CURRENT_PRESET
  for (int i = 1; i < SLOTS_PER_PRESET; i++)
  {
    byte trigger, action, switchNum, channel, data1, data2, data3;
    readSlot(i, &trigger, &action, &switchNum, &channel, &data1, &data2, &data3);
    if (trigger == Trigger::EnterPreset)
      executeAction(action, channel, data1, data2);

    // apply latch on if switch is a latch and ON at start
    if (getSwitchType(switchNum) && getSwitchState(switchNum) == 1)
      executeAction(action, channel, data1, data2);
  }
}

/**
 * @brief Print preset from main memory
 * Omits empty slots
 * @param preset
 */
void printPreset(byte preset)
{
  xprintf("Preset %03d:", preset);
  byte printed = 0;
  byte data[SLOTS_PER_PRESET][BYTES_PER_SLOT];
  copyPresetFromMemory(preset, data);

  // print meta slot (byte 0 switches, byte 1 states)
  xprintf("\n SW_TYPES:  ");
  // print M if momentary, L if latch
  for (int i = 7; i >= 0; i--)
    xprintf("%c ", getSwitchType(i) == LATCH ? 'L' : 'M');

  xprintf("\n SW_STATES: ");
  // print 1 if HIGH, 0 if LOW
  for (int i = 7; i >= 0; i--)
    xprintf("%d ", getSwitchState(i));

  for (int i = 1; i < SLOTS_PER_PRESET; i++)
  {
    byte copied[BYTES_PER_SLOT];
    // copy slot i from data into copied
    memcpy(copied, data[i], BYTES_PER_SLOT);
    // check if action is NA
    if (copied[0] == NA)
      continue;
    xprintf("\n [%02d]: ", i);
    printHexArray(copied, BYTES_PER_SLOT);
    printed++;
  }

  // don't count meta slot, it's not accessible by the user
  if (printed == 0)
    xprintf(" (empty)");
  else
    xprintf("\n %d/%d slots used", printed, SLOTS_PER_PRESET - 1);

  xprintf("\n");
}

// sysex
// -- message handlers --
/**
 * @brief Given a sysex byte array, check if it matches the vendor ID
 * Example: checkVendor([0xF0 0x00 0x53 0x4D], 4) would return true
 * @param data
 * @param size
 * @return true
 * @return false
 */
bool checkVendor(const byte *data, unsigned int size)
{
  if (size < 5) // if not long enough to contain the needed bytes, then it doesn't match
    return false;
  return (*(data + 1) == 0x00 && *(data + 2) == MFID_1 && *(data + 3) == MFID_2);
}

/**
 * @brief on receiving a system exclusive message
 *
 * @param data
 * @param length
 */
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
    Serial.println(F(" (end)"));
  else
    Serial.println(F(" (tbc)"));

  // check if the message is complete and process it
  if (last && checkVendor(data, length))
    parseSysExMessage(data, length);
  else if (!checkVendor(data, length)) // if it doesn't match the vendor ID
  {                                    // do nothing
  }
  else
    Serial.println(F("Unexplained issue! The SysEx message is not getting processed"));
}

/**
 * @brief Parse a sysex message
 * This directs the message to the correct function based on the message type.
 * @param data
 * @param length
 */
static void parseSysExMessage(byte *data, unsigned int length)
{
  // check message types
  byte msgType = *(data + 4); // 5th byte is the message type

  if (msgType == MsgTypes::SetSlot)
  {
    // check length
    if (length != 15) // including the 0xF0 and 0xF7
    {
      xprintf("Invalid message length for SetSlot: %d\n", length);
      return;
    }

    // get data
    byte preset = *(data + 5);
    byte slot = *(data + 6);
    byte trigger = *(data + 7);
    byte action = *(data + 8);
    byte switchNum = *(data + 9);
    byte channel = *(data + 10);
    byte data1 = *(data + 11);
    byte data2 = *(data + 12);
    byte data3 = *(data + 13);

    // write to memory
    xprintf("SetSlot: %d %d %d %d %d %d %d %d %d\n", preset, slot, trigger, action, switchNum, channel, data1, data2, data3);
    writeSlot(slot, trigger, action, switchNum, channel, data1, data2, data3);
    savePreset();
  }
  else if (msgType == MsgTypes::GetSlot)
  {
    byte preset = *(data + 5);
    byte slot = *(data + 6);
    xprintf("GetSlot: Preset %d, Slot %d\n", preset, slot);

    // read from memory
    byte trigger, action, switchNum, channel, data1, data2, data3;
    readSlot(slot, &trigger, &action, &switchNum, &channel, &data1, &data2, &data3);

    // send the message
    byte slotInfo[10] = {
        MsgTypes::GetSlotReturn,
        preset,
        slot,
        trigger,
        action,
        switchNum,
        channel,
        data1,
        data2,
        data3};

    sendSystemExclusive(slotInfo, 10);
  }
  else if (msgType == MsgTypes::SetSystemParam)
  {
    byte param = *(data + 5);
    byte value = *(data + 6);

    if (param == SystemParams::CurrentPreset)
    {
      xprintf("SetSystemParam: CurrentPreset: %d\n", value);
      setCurrentPreset(value);
    }
    else
    {
      xprintf("Unsupported system param: %d\n", param);
    }
  }
  else if (msgType == MsgTypes::GetSystemParam)
  {
    byte param = *(data + 5);
    xprintf("GetSystemParam: %d\n", param);

    if (param == SystemParams::CurrentPreset)
    {
      byte paramInfo[3] = {
          MsgTypes::GetSystemParamReturn,
          SystemParams::CurrentPreset,
          CURRENT_PRESET};

      sendSystemExclusive(paramInfo, 7);
    }
    else
    {
      xprintf("Unsupported system param: %d\n", param);
    }
  }
  else if (msgType == MsgTypes::GetPreset)
  {
    byte preset = *(data + 5);

    // create one byte array to hold the entire preset
    byte presetData[BYTES_PER_SLOT * SLOTS_PER_PRESET];
    for (int i = 0; i < SLOTS_PER_PRESET; i++)
    {
      byte trigger, action, switchNum, channel, data1, data2, data3;
      readSlotFromMemory(preset, i, &trigger, &action, &switchNum, &channel, &data1, &data2, &data3);
      presetData[i * BYTES_PER_SLOT] = trigger;
      presetData[i * BYTES_PER_SLOT + 1] = action;
      presetData[i * BYTES_PER_SLOT + 2] = switchNum;
      presetData[i * BYTES_PER_SLOT + 3] = channel;
      presetData[i * BYTES_PER_SLOT + 4] = data1;
      presetData[i * BYTES_PER_SLOT + 5] = data2;
      presetData[i * BYTES_PER_SLOT + 6] = data3;
    }
    // now create an array with the message type and the preset data
    byte presetInfo[BYTES_PER_SLOT * SLOTS_PER_PRESET + 2] = {MsgTypes::GetPresetReturn, preset};

    // copy the preset data into the array
    memcpy(presetInfo + 2, presetData, BYTES_PER_SLOT * SLOTS_PER_PRESET);

    // send the message
    sendSystemExclusive(presetInfo, BYTES_PER_SLOT * SLOTS_PER_PRESET + 2);
  }
  else
  {
    Serial.println(F("Unsupported message type"));
  }
}

// -- utility functions --

/**
 * @brief Print a byte array in hex
 *
 * @param data
 * @param size
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

/**
 * @brief Printf for Arduino Serial
 *
 * @param format
 * @param ...
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

/**
 * @brief Get a random byte between min and max
 *
 * @param min
 * @param max
 * @return byte
 */
byte randomByte(byte min, byte max)
{
  return random(min, max + 1);
}

// hash the current timestamp into 8 bytes
/**
 * @brief This function hashes the current timestamp into 8 bytes
 * For example, if the current decimal time (in milliseconds) is
 * 1721176304894, it would return 00 00 01 90 BE 1A 2C FE. This
 * helps to know which message is responding to which request.
 * @param data
 */
void hashTimestamp(byte *data)
{
  unsigned long timestamp = millis();
  for (int i = 0; i < 8; i++)
  {
    data[i] = timestamp & 0xFF;
    timestamp >>= 8;
  }
}

/**
 * @brief Print a byte in binary
 *
 * @param b
 */
void printBinaryByte(byte b)
{
  Serial.print("0b");
  for (int i = 7; i >= 0; i--)
  {
    Serial.print(bitRead(b, i));
  }
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
