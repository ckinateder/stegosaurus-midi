![Stegosaurus Logo](img/logo96.png)

# stegosaurus


Stegoaurus is an open-source MIDI controller. The controller is built around the Arduino framework and uses the MIDI protocol to communicate with a computer. It is designed to be easily configurable, with the ability to change the behavior of the controller using SysEx messages. 

TODO:
- [x] Add hardware MIDI support to the Pico
- [x] Add SysEx support to the Pico
- [ ] Web interface for programming
    - [x] Basic interaction
    - [ ] Add SysEx messages
- [ ] EEPROM module
- [ ] Pinouts
## Project Structure

THis project is organized as follows:

- [`firmware/`](firmware/): main firmware code
- [`interface/`](interface/): code for the web interface

## Requirements

### Firmware

![schematic](img/schematic.png)

This project runs on the **Raspberry Pi Pico**.
To set it up in the Arduino IDE, follow [this guide](https://randomnerdtutorials.com/programming-raspberry-pi-pico-w-arduino-ide/). You will probably have to hold down the BOOTSEL button while plugging in the Pico to get it into bootloader mode for the first time. Pay attention to these upload settings:

![upload settings](img/settings.png).

Make sure the selected port matches the port that the Pico is connected to.

Required libraries:

- [EasyButton](https://github.com/evert-arias/EasyButton) (@v2.0.3)
- [Adafruit TinyUSB](https://github.com/adafruit/Adafruit_TinyUSB_Arduino) (@v3.1.4)

### Pinouts

### Interface

This will have to use WebMIDI to communicate with the Arduino.

A good first step for this is to get a list of MIDI devices. I want to do this in React.

## Interface for Programming

See [this page](https://learn.sparkfun.com/tutorials/midi-tutorial/advanced-messages) for a good reference on MIDI messages.

The Stegosaurus will use SysEx messages to change the behavior of the controller. 

The manufacturer ID for the USB MIDI SysEx messages is `0x00 0x53 0x4D`. The `0x00` indicates that the message has a vendor ID, and the `0x53 0x4D` is the ASCII representation of "SM". This is a unique identifier for the Stegosaurus MIDI controller (I think it is unique). So, if you see a message with this ID, it is likely from the Stegosaurus. The rest of the message is the actual data.

To program the controller behavior, the interface will send a SysEx message with the manufacturer ID, followed by the command byte, and then the data. Keep in mind that the SysEx message maximum length is 128 bytes. This is using this [here](https://github.com/FortySevenEffects/arduino_midi_library/wiki/Using-custom-Settings), but I don't think that any of the messages will be even close to that length.

## MIDI Messages

The Stegosaurus will use the following MIDI messages. Keep in mind that the first 4 bytes of the message are specific to the SysEx MIDI protocol. The rest of the message is the actual data. Every SysEx message is concluded with the SysEx end byte `0xF7`.

### Memory Layout
There are 128 presets, each with 17 slots, and 7 bytes per slot. The slots are numbered 1-16, with slot 0 reserved for metadata. The metadata for each preset is as follows:

| Byte | Description    |
|------|----------------|
| 0    | Switch type    |
| 1    | Switch values  |

The switch type is a 1-byte value that indicates the type of switch that the preset uses. Each bit from right to left indicates a switch. 0 is a momentary switch, and 1 is a latching switch. For example, 0b10000000 would mean that SWA is LATCHING and the rest are momentary.

The switch values are a 1-byte value that indicates the current state of the switches. Each bit from right to left indicates a switch. 0 is off, and 1 is on. For example, 0b10000000 would mean that SWA is on and the rest are off. This only applies to latching switches. This value is tracked fso that the user can set a switch to a specific state when they enter a preset.

#### Preset Layout

### Message Structure

The message is broken up into 2 parts: the header and the data. The header is the first 4 bytes of the message, and the data is the rest of the message.

Types of messages:
- Control change
- Program change
- Program get
- Variable set


#### Header

| Byte | Description | Value                                                            |
|------|------------|-----------------------|
| 0 | Status byte | 0xF0 (SysEx start) |
| 1 | Vendor ID | 0x00 (Three-byte Vendor ID) |
| 2 | Vendor ID | 0x53 (Vendor ID) |
| 3 | Vendor ID | 0x4D (Vendor ID) |
| 4-9 | Timestamp | 6 bytes indicating the current datetime |
| 10 | Response/Request* | 0 or 1|
| 11 | Message type** | 0, 1, or 2 |

**\*Response/Request is 0 if the message is a request and 1 if it is a response.**

**\*\*See following section for message types.**

Message types:

| Type | Description                |
|------|----------------------------|
| 0    | Write to a slot            |
| 1    | Read from a slot           |
| 2    | System parameter set       |
| 3    | System parameter get       |
|      | Get preset|


#### Message Data

The data is the rest of the message. There are 3 types of messages: write to a slot, read from a slot, and system parameter set. Each message has a different format.

#### Write to a Slot

| Byte | Description    |
|------|----------------|
| 5    | Preset to modify |
| 6    | Slot to modify |
| 7    | Trigger  |  
| 8    | Action |
| 9    | Switch number |  
| 10   | MIDI channel   |
| 11   | Data 1 |
| 12   | Data 2 |
| 13   | Data 3 |

```c++
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
```

Data 1, Data 2, Data 3 

This message will write the data to the specified slot in the specified preset. 

#### Read from a Slot

| Byte | Description    |
|------|----------------|
| 5    | Preset to read |
| 6    | Slot to read |

This returns the data from the specified slot in the specified preset in the same format as the write message.
Note that the preset and slot are returned in the message.
An example message would be:

```
F0 04 01 03 01 00 0F 45 08 00 F7
```

#### System Parameter Set

| Byte | Description    |
|------|----------------|
| 5    | Parameter to set |
| 6    | Value to set |

This sets a system parameter. The parameters are as follows:

```c++
enum SystemParams
{
  CurrentPreset = 0, // the current preset
};
```

