# Wiring

Every button connects between a GPIO pin and **GND**. The firmware turns on the Pico's internal pull-ups, so no resistors are needed. A pressed button reads LOW.

| Pico pin | GPIO | Connects to |
|---|---|---|
| 14 | GP10 | Encoder A |
| 15 | GP11 | Encoder B |
| 20 | GP15 | Big select button (100 mm) |
| 21 | GP16 | Red |
| 22 | GP17 | Green |
| 24 | GP18 | Blue |
| 25 | GP19 | Yellow |
| 26 | GP20 | Chooser |
| 27 | GP21 | Back |
| 31 | GP26 | Battery sense (optional, off by default) |
| 40 | VBUS | 5 V for the button LEDs only. The original box lights the Red button as a power light. |
| 36 | 3V3(OUT) | Encoder power (the prototype runs the 60 mm encoder from 3.3 V) |
| 3, 8, 13, 18, 23, 28, 33, 38 | GND | Grounds |

## Notes

- **Encoder:** the 60 mm CNC encoder has screw terminals on the back, labelled VCC, GND, A and B. Push a male jumper pin into each and tighten. Its listing says 5–12 V, but the prototype runs it from 3V3 without trouble, which also keeps A and B at a level the Pico can take. If you ever power it from 5 V instead, divide A and B down to 3.3 V (the Pico's pins are not 5 V tolerant). If clockwise moves backwards, set `ENC_REVERSE true` in the sketch.
- **Button LEDs:** the original box wires only the Red button's LED, as a power light; it also shows whether the USB-C plug is the right way up (see [BOM.md](BOM.md#usb-c-port-orientation)). To light more, chain every LED + tab to VBUS and every LED − tab to GND. A light that stays dark is usually reversed; swap its two wires (a few seconds backwards does no harm). Never connect VBUS to the 3.3 V rail.
- **Buttons:** each Adafruit quick-connect pair has two spade clips joined in a 2-pin plug. Push the clips onto the button's two switch terminals, then push two male jumpers into the plug: one to the button's GPIO, one to GND.
- **Grounds:** daisy-chain one ground wire from button to button, or use a Wago lever connector, then run a single wire to a GND pin.
- **Checking the wiring:** flash `firmware/tools/keytest`, open a text editor on any computer, and press each control. It types its name: SELECT, RED, GREEN, and so on. The wheel types `>` and `<`.

The illustrated [build guide](https://wmetcalf.github.io/voiceover-switch-box/docs/build-guide.html) has breadboard diagrams, drill templates and step-by-step instructions.
