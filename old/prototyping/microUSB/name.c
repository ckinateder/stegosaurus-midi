// if the board is teensy 4, then the usb name can be set here
#if defined(ARDUINO_TEENSY40) || defined(ARDUINO_TEENSY41)
  #include "usb_names.h"

  #define MIDI_NAME                         \
      {                                     \
          'S', 't', 'e', 'g', 'o', 's', 'a', 'u', 'r', 'u', 's' \
      }
  #define MIDI_NAME_LEN 11

  // Do not change this part.  This exact format is required by USB.
  struct usb_string_descriptor_struct usb_string_product_name = {
      2 + MIDI_NAME_LEN * 2,
      3,
      MIDI_NAME};
#elif defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega16U4__)
  // arduino micro
  // names have to be changed in boards.txt
#endif