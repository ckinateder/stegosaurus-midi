# stegosaurus-firmware



## Project Structure

THis project is organized as follows:

- `firmware/`: main firmware code
- `interface/`: code for the web interface

## Requirements

### Firmware

This is currently built on a Teensy 4.0. It should also work with any ATMega32U4 board, such as the Arduino Micro. The firmware is written in C++ and uses the Arduino framework. 
Make sure you have the arduino IDE installed, and the Teensyduino add-on. See the [Teensyduino](https://www.pjrc.com/teensy/td_download.html) page for more information. The following libraries are required:

- [Arduino MIDI Library](https://github.com/FortySevenEffects/arduino_midi_library) (@v5.0.2)
- [Arduino USBMIDI](https://github.com/lathoub/Arduino-USBMIDI) (@v1.1.2)
- [EasyButton](https://github.com/evert-arias/EasyButton) (@v2.0.3)

### Interface

This will have to use WebMIDI to communicate with the Arduino.

A good first step for this is to get a list of MIDI devices. I want to do this in React.

## Odds and Ends

See [this page](https://learn.sparkfun.com/tutorials/midi-tutorial/advanced-messages) for a good reference on MIDI messages.

The manufacturer ID for the USB MIDI SysEx messages is `0x00 0x53 0x4D`. The `0x00` indicates that the message has a vendor ID, and the `0x53 0x4D` is the ASCII representation of "SM". This is a unique identifier for the Stegosaurus MIDI controller (I think it is unique). So, if you see a message with this ID, it is likely from the Stegosaurus. The rest of the message is the actual data.

To program the controller behavior, the interface will send a SysEx message with the manufacturer ID, followed by the command byte, and then the data. Keep in mind that the SysEx message maximum length is 128 bytes. This is changable [here](https://github.com/FortySevenEffects/arduino_midi_library/wiki/Using-custom-Settings), but I don't think that any of the messages will be even close to that length.
