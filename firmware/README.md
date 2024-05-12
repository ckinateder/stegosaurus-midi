# stegosaurus

## Project Structure

THis project is organized as follows:

- `firmware/`: main firmware code
- `interface/`: code for the web interface

## Requirements

### Firmware

#### Raspberry Pi Pico

This is currently being switched over to work with the Raspberry Pi Pico.
To set it up in the Arduino IDE, follow [this guide](https://randomnerdtutorials.com/programming-raspberry-pi-pico-w-arduino-ide/). You will probably have to hold down the BOOTSEL button while plugging in the Pico to get it into bootloader mode for the first time. Pay attention to these upload settings:

![upload settings](../img/settings.png).

Make sure the selected port matches the port that the Pico is connected to.

Required libraries:

- [EasyButton](https://github.com/evert-arias/EasyButton) (@v2.0.3)
- [Adafruit TinyUSB](https://github.com/adafruit/Adafruit_TinyUSB_Arduino) (@v3.1.4)

https://electronics.stackexchange.com/questions/623026/problem-working-with-midi-on-hardware-serial-port-on-raspberry-pi-pico-2040


### Interface

This will have to use WebMIDI to communicate with the Arduino.

A good first step for this is to get a list of MIDI devices. I want to do this in React.

## Odds and Ends

See [this page](https://learn.sparkfun.com/tutorials/midi-tutorial/advanced-messages) for a good reference on MIDI messages.

The manufacturer ID for the USB MIDI SysEx messages is `0x00 0x53 0x4D`. The `0x00` indicates that the message has a vendor ID, and the `0x53 0x4D` is the ASCII representation of "SM". This is a unique identifier for the Stegosaurus MIDI controller (I think it is unique). So, if you see a message with this ID, it is likely from the Stegosaurus. The rest of the message is the actual data.

To program the controller behavior, the interface will send a SysEx message with the manufacturer ID, followed by the command byte, and then the data. Keep in mind that the SysEx message maximum length is 128 bytes. This is changable [here](https://github.com/FortySevenEffects/arduino_midi_library/wiki/Using-custom-Settings), but I don't think that any of the messages will be even close to that length.

## MIDI Messages

This SysEx message will be used to change the behavior of the controller. 

### Changing Preset Behavior
 
| Byte | Value                                                                 |
|------|----------------------------------------------------------------------|
| 0    | Left nybble denotes the operation. 0x0 for modify preset operation. Right nybble denotes whether or not to save to EEPROM or not. 0x0 for no, 0x1 for yes. For example, 0x01 for modify preset and write to EEPROM; 0x00 for modify preset but not write to EEPROM (probably for debugging). |
| 1    | Preset (allow up to 0-127)  |
| 2    | Only using the right nybble. Trigger on enter preset, exit preset, or switch press number (0x0 to 0x03 switch number, 0xE for on preset entry, 0xF for on preset exit). |
| 3    | Storage location for preset (1 byte would allow 256 different locations per byte, but that’s too many - I think up to 64 is the maximum that would make sense) |
| 4    | Message type (cc/pc), channel |
| 5    | Message number |
| 6    | Message value (if needed, for cc message) |


## TRASH

#### Teensy 4.0

[DEPRECATED]

This is currently built on a Teensy 4.0. It should also work with any ATMega32U4 board, such as the Arduino Micro, HOWEVER, the USB name on the Micro will show up as just "Arduino Micro". The firmware is written in C++ and uses the Arduino framework. 
Make sure you have the arduino IDE installed, and the Teensyduino add-on. See the [Teensyduino](https://www.pjrc.com/teensy/td_download.html) page for more information. The following libraries are required:

- [Arduino MIDI Library](https://github.com/FortySevenEffects/arduino_midi_library) (@v5.0.2)
- [Arduino USBMIDI](https://github.com/lathoub/Arduino-USBMIDI) (@v1.1.2)
- [EasyButton](https://github.com/evert-arias/EasyButton) (@v2.0.3)

You'll need to change the USB type to "Serial + MIDI" in the Arduino IDE. This is done by going to `Tools > USB Type` and selecting "Serial + MIDI".