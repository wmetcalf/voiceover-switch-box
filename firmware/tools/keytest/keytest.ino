/*
 * VoiceOver Device — KEY TEST firmware (diagnostic only)
 * Each control TYPES its name as plain text, so you can test the wiring and the
 * USB/Bluetooth link in any text box (TextEdit, Notes) with VoiceOver OFF.
 *   Wheel forward = ">"   Wheel back = "<"
 *   Buttons type: SELECT RED GREEN BLUE YELLOW CHOOSER BACK
 *   (and JACK1..JACK4 for the spare inputs on GP2-GP5, not wired on the original box)
 * On power-up it types nothing; LED flashes 5x quickly, then shows the link:
 *   FAST blink (5/s)        = USB keyboard connected to a computer/iPad
 *   DOUBLE blink each sec   = Bluetooth connected
 *   SLOW blink (1/s)        = powered, but nothing connected
 *   SOLID on or off         = firmware not running (frozen)
 * USB when a host is connected, otherwise Bluetooth ("VoiceOver Device").
 * Reflash voiceover_device_ble.uf2 afterwards to get the normal firmware back.
 */
#include <Keyboard.h>
#include <KeyboardBLE.h>
#include <PicoBluetoothBLEHID.h>
extern "C" bool tud_mounted(void);

const uint8_t PINS[]  = { 15, 16, 17, 18, 19, 20, 21, 2, 3, 4, 5 };
const char*   NAMES[] = { "SELECT", "RED", "GREEN", "BLUE", "YELLOW", "CHOOSER", "BACK", "JACK1", "JACK2", "JACK3", "JACK4" };
const int N = sizeof(PINS);
#define ENC_A 10
#define ENC_B 11
#define PIEZO 22

bool stable[11], raw[11]; uint32_t changedAt[11];
volatile int32_t encCount = 0; volatile uint8_t encState = 0;
static const int8_t QDEC[16] = { 0,-1, 1, 0,  1, 0, 0,-1,  -1, 0, 0, 1,  0, 1,-1, 0 };
void encISR() { uint8_t s = (digitalRead(ENC_A) << 1) | digitalRead(ENC_B); encState = ((encState << 2) | s) & 0x0F; encCount += QDEC[encState]; }

void out(const char* s) {
  if (tud_mounted()) Keyboard.print(s); else KeyboardBLE.print(s);
  tone(PIEZO, 2000, 15);
}

void setup() {
  for (int i = 0; i < N; i++) { pinMode(PINS[i], INPUT_PULLUP); raw[i] = stable[i] = false; changedAt[i] = 0; }
  pinMode(ENC_A, INPUT_PULLUP); pinMode(ENC_B, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT); pinMode(PIEZO, OUTPUT);
  encState = (digitalRead(ENC_A) << 1) | digitalRead(ENC_B);
  attachInterrupt(digitalPinToInterrupt(ENC_A), encISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), encISR, CHANGE);
  Keyboard.begin();
  KeyboardBLE.begin("VoiceOver Device");
  for (int i = 0; i < 5; i++) { digitalWrite(LED_BUILTIN, HIGH); delay(60); digitalWrite(LED_BUILTIN, LOW); delay(60); }
}

void statusLED(uint32_t now) {
  bool on;
  if (tud_mounted())                         on = (now % 200) < 100;                 // fast: USB
  else if (PicoBluetoothBLEHID.connected()) { uint32_t t = now % 1000; on = t < 100 || (t >= 200 && t < 300); } // double: BLE
  else                                       on = (now % 1000) < 500;                // slow: nothing
  static int last = -1;
  if (on != last) { digitalWrite(LED_BUILTIN, on); last = on; }
}

void loop() {
  uint32_t now = millis();
  statusLED(now);
  for (int i = 0; i < N; i++) {
    bool r = digitalRead(PINS[i]) == LOW;
    if (r != raw[i]) { raw[i] = r; changedAt[i] = now; }
    if (raw[i] != stable[i] && now - changedAt[i] >= 25) {
      stable[i] = raw[i];
      if (stable[i]) { out(NAMES[i]); out(" "); }
    }
  }
  noInterrupts(); int32_t c = encCount; int32_t steps = c / 4; encCount = c - steps * 4; interrupts();
  for (; steps > 0; steps--) out(">");
  for (; steps < 0; steps++) out("<");
}
