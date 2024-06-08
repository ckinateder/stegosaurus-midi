# stegosaurus

Stegoaurus is an open-source MIDI controller. The controller is built around the Arduino framework and uses the MIDI protocol to communicate with a computer. It is designed to be easily configurable, with the ability to change the behavior of the controller using SysEx messages. 

## Project Structure

THis project is organized as follows:

- [`firmware/`](firmware/): main firmware code
- [`interface/`](interface/): code for the web interface

## Requirements

### Firmware

#### Raspberry Pi Pico

This is currently being switched over to work with the Raspberry Pi Pico.
To set it up in the Arduino IDE, follow [this guide](https://randomnerdtutorials.com/programming-raspberry-pi-pico-w-arduino-ide/). You will probably have to hold down the BOOTSEL button while plugging in the Pico to get it into bootloader mode for the first time. Pay attention to these upload settings:

![upload settings](img/settings.png).

Make sure the selected port matches the port that the Pico is connected to.

Required libraries:

- [EasyButton](https://github.com/evert-arias/EasyButton) (@v2.0.3)
- [Adafruit TinyUSB](https://github.com/adafruit/Adafruit_TinyUSB_Arduino) (@v3.1.4)

[This link](https://electronics.stackexchange.com/questions/623026/problem-working-with-midi-on-hardware-serial-port-on-raspberry-pi-pico-2040) should help with hardware midi on the Pico.

TODO:
- [ ] Add hardware MIDI support to the Pico
- [ ] Start using the linker instead of the Arduino IDE
- [ ] Add SysEx support to the Pico

### Interface

This will have to use WebMIDI to communicate with the Arduino.

A good first step for this is to get a list of MIDI devices. I want to do this in React.

## Interface for Programming

See [this page](https://learn.sparkfun.com/tutorials/midi-tutorial/advanced-messages) for a good reference on MIDI messages.

The Stegosaurus will use SysEx messages to change the behavior of the controller. 

The manufacturer ID for the USB MIDI SysEx messages is `0x00 0x53 0x4D`. The `0x00` indicates that the message has a vendor ID, and the `0x53 0x4D` is the ASCII representation of "SM". This is a unique identifier for the Stegosaurus MIDI controller (I think it is unique). So, if you see a message with this ID, it is likely from the Stegosaurus. The rest of the message is the actual data.

To program the controller behavior, the interface will send a SysEx message with the manufacturer ID, followed by the command byte, and then the data. Keep in mind that the SysEx message maximum length is 128 bytes. This is using this [here](https://github.com/FortySevenEffects/arduino_midi_library/wiki/Using-custom-Settings), but I don't think that any of the messages will be even close to that length.

## MIDI Messages

The Stegosaurus will use the following MIDI messages. Keep in mind that the first 4 bytes of the message are specific to the SysEx MIDI protocol. The rest of the message is the actual data.

### Message Structure

The message is broken up into 3 parts: the header, bookkeeping, and the actual data. The header is the first 4 bytes of the message, and is used to identify the message as a SysEx message from the Stegosaurus. The bookkeeping is the next 6 bytes, and is used to identify the type of message and the preset to modify. The actual data is the rest of the message, and is used to specify the behavior of the controller.

#### Header

| Byte | Value                                                            |
|------|----------------------------------------------------------------------|
| 0 | 0xF0 (SysEx start) |
| 1 | 0x00 (Three-byte Vendor ID) |
| 2 | 0x53 (Vendor ID) |
| 3 | 0x4D (Vendor ID) |

#### Bookkeeping

| Byte | Description    | Range |
|------|----------------|-------|
| 4    | Whether or not to save to persistent memory |  0x00 = no, 0x01 = yes |
| 5    | Trigger  |  0x00 for preset entry, 0x01 for switch short press, 0x02 for switch long press |
| 6    | Switch number (if trigger is 0x01 or 0x02) | [0, 3] |
| 7    | Switch type (if trigger is 0x01 or 0x02) | 0x0 for momentary, 0x1 for toggle |
| 8    | Preset to modify | [0, 127] |
| 9    | Operation | 0x00 for program change (PC), 0x01 for control change (CC), 0x0F for firmware parameter change |

#### Data

The data is the rest of the message, and is specific to the operation. 

**Program Change**

Program change messages are called when bit 9 of the bookkeeping byte is set to 0x00. 

| Byte | Description    | Range |
|------|----------------|-------|
| 10   | MIDI channel   | [0, 15] |
| 11   | Program number | [0, 127] |

**Control Change**

Control change messages are called when bit 9 of the bookkeeping byte is set to 0x01. 

| Byte | Description    | Range |
|------|----------------|-------|
| 10   | MIDI channel   | [0, 15] |
| 11   | Control number | [0, 127] |
| 12   | Control value  | [0, 127] |

**Firmware Parameter Change**


## TRASH

#### Teensy 4.0

[DEPRECATED]

This is currently built on a Teensy 4.0. It should also work with any ATMega32U4 board, such as the Arduino Micro, HOWEVER, the USB name on the Micro will show up as just "Arduino Micro". The firmware is written in C++ and uses the Arduino framework. 
Make sure you have the arduino IDE installed, and the Teensyduino add-on. See the [Teensyduino](https://www.pjrc.com/teensy/td_download.html) page for more information. The following libraries are required:

- [Arduino MIDI Library](https://github.com/FortySevenEffects/arduino_midi_library) (@v5.0.2)
- [Arduino USBMIDI](https://github.com/lathoub/Arduino-USBMIDI) (@v1.1.2)
- [EasyButton](https://github.com/evert-arias/EasyButton) (@v2.0.3)

You'll need to change the USB type to "Serial + MIDI" in the Arduino IDE. This is done by going to `Tools > USB Type` and selecting "Serial + MIDI".