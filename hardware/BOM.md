# Parts list

Adafruit product numbers are the parts the original box was built with. Equivalents are fine.

## Controls

| Qty | Part | Adafruit # | Notes |
|---|---|---|---|
| 1 | CNC Rotary Encoder, 100 pulses/rotation, 60 mm | [5734](https://www.adafruit.com/product/5734) | The navigation wheel. One click = one step. |
| 1 | Massive Arcade Button with LED, 100 mm | [1185](https://www.adafruit.com/product/1185) | The select button. Red in the original. |
| 1 | Arcade Button with LED, 30 mm, translucent red | [3489](https://www.adafruit.com/product/3489) | Read. Its LED is the power light. |
| 1 | Arcade Button with LED, 30 mm, translucent green | [3487](https://www.adafruit.com/product/3487) | Home |
| 1 | Arcade Button with LED, 30 mm, translucent blue | [3490](https://www.adafruit.com/product/3490) | Mute speech |
| 1 | Arcade Button with LED, 30 mm, translucent yellow | [3488](https://www.adafruit.com/product/3488) | Play/pause |
| 2 | Arcade Button with LED, 30 mm, translucent clear | [3491](https://www.adafruit.com/product/3491) | Chooser, Back |
| 1 pack | Quick-connect wire pairs, 0.11" (10-pack) | [1152](https://www.adafruit.com/product/1152) | One pair per 30 mm button's switch, plus one for the Red light: 7 pairs. Buy a second pack if you light every button. Each pair's two spade clips are joined in a 2-pin plug that takes jumper pins; cut it off for screw terminals. |
| 1 pack | Quick-connect wire pairs, 0.187" (10-pack) | [3835](https://www.adafruit.com/product/3835) | One pair for the 100 mm button's switch. Same 2-pin plug. |

## Electronics

| Qty | Part | Notes |
|---|---|---|
| 1 | Raspberry Pi Pico 2 W, with headers ([Amazon B0DPF9N1MN](https://www.amazon.com/dp/B0DPF9N1MN)) | The W (wireless) version is needed for Bluetooth. |
| 1 | Solderless breadboard (the original used a BOJACK kit with 830- and 400-point boards, [Amazon B08Y59P6D1](https://www.amazon.com/dp/B08Y59P6D1)), or Adafruit Perma-Proto half-size ([1609](https://www.adafruit.com/product/1609)), or Terminal Block Breakout for Pico ([5095](https://www.adafruit.com/product/5095)) | The terminal block board is the tidiest permanent build: no soldering, and the Pico plugs in. |
| 1 | PENGLIN USB-C female-to-female panel-mount coupler, D-type, 2-pack ([Amazon B0B4VWZD7X](https://www.amazon.com/dp/B0B4VWZD7X)) | The box's outside USB-C port. 24 mm hole plus two screws on opposite corners. One spare. **Passes power one way round only**; see below. |
| 1 | Cable Matters braided USB-C to micro-USB cable, 1 m ([Amazon B00UUBRX0Y](https://www.amazon.com/dp/B00UUBRX0Y)) | Inside the box, from the panel connector to the Pico. Coil the spare length. |
| 1 | USB-C to USB-C cable that carries data | Box to iPad. Many USB-C cables are charge-only. |
| 1 kit | Dupont jumper wires, male–male / male–female / female–female (ELEGOO 120-piece kit, [Amazon B01EV70C78](https://www.amazon.com/dp/B01EV70C78)) | Breadboard wiring |
| — | 22 AWG stranded hook-up wire | For screw terminals and longer runs |
| — | Wago 221-415 lever connectors (optional) | Ground and 5 V buses without soldering. |
| 3 | M3 screws, about 10 mm (plus nuts if the encoder's mounting holes aren't threaded) | Fix the encoder to the lid. |

## Enclosure

| Qty | Part | Notes |
|---|---|---|
| 1 | LeMotech IP65 ABS project box, pale gray, 263 × 182 × 95 mm (10.4 × 7.2 × 3.7 in) ([Amazon B0785ZBNKK](https://www.amazon.com/dp/B0785ZBNKK)) | Fits the 100 mm button and the 60 mm wheel. The build guide's drill templates are sized for this box. |

## Tools

| Part | Notes |
|---|---|
| HYCHIKA 19-piece hole saw kit, 19–152 mm ([Amazon B07Y1G2Y7K](https://www.amazon.com/dp/B07Y1G2Y7K)) | The original box used its 44 mm saw for the encoder, 32 mm for the six 30 mm buttons and 28 mm for the select button. Its 22 mm saw, filed out to 24 mm, suits the USB-C coupler. |
| Half-round file, or a step drill | Opens the 22 mm hole to the 24 mm the USB-C coupler needs. |
| 3.5 mm drill bit | Encoder and USB-C coupler screw holes (M3). |

## Button lights

The original box lights only the **Red** button, as a power light: the Pico's own LED is inside the box, so otherwise nothing outside shows the box is on.

The 30 mm buttons' LEDs have built-in resistors and need **5 V**; red and yellow won't light at 3.3 V. Power them from the Pico's **VBUS** pin (pin 40), with LED + to 5 V and LED − to GND. See [wiring.md](wiring.md).

## USB-C port orientation

The PENGLIN coupler only passes power when the iPad's cable is plugged into the box one way up. A pass-through coupler connects the cable's orientation-detect (CC) line to the internal cable's pull-down resistor in only one orientation, so the other way up the iPad never switches power on. The original box lives with this:

- Leave the cable plugged into the box (a cable tie for strain relief helps) and unplug it only at the iPad, whose port works either way. The Red power light shows when it's the right way up.
- If you want a port that works both ways up, use a single USB-C-socket-to-micro-USB panel-mount cable whose socket has 5.1 kΩ pull-downs on both CC pins, instead of the coupler and internal cable.
