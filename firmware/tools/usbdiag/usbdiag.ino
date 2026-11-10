/*
 * VoiceOver Device — USB DIAGNOSTIC firmware (for finding why an iPad doesn't see the box)
 *
 * Plug the box into the iPad (unlocked) and watch the Pico's LED. It repeats a
 * count of flashes, then a pause. If the piezo is fitted it also beeps the
 * count whenever the result changes.
 *
 *   1 flash  = power only, NO USB data. The iPad never talked to the box:
 *              charge-only cable, or the USB-C plug/panel connector only
 *              carries data one way round (flip the plug at the box end).
 *   2 flashes= the iPad saw the box but did not set it up (refused it:
 *              accessory permission, power, or a USB problem).
 *   3 flashes= the iPad set the box up but is NOT reading the keyboard
 *              (accessory not allowed, or iPadOS rejected the keyboard).
 *   4 flashes= USB keyboard WORKING. The iPad is reading keystrokes.
 *              Then the problem is on the VoiceOver side, not the box.
 *   5 flashes= the iPad put the USB link to sleep (screen off / locked).
 *   Solid on/off = firmware not running.
 *
 * Buttons still type their names (SELECT, RED, ... ) and the wheel types > <,
 * over USB when it is working (4 flashes), otherwise over Bluetooth if paired.
 * The 4-flash test sends only empty "no key pressed" reports, so nothing is
 * typed unless you press something.
 *
 * Reflash voiceover_device_ble.uf2 afterwards to get the normal firmware back.
 */
#include <Keyboard.h>
#include <KeyboardBLE.h>
#include <PicoBluetoothBLEHID.h>
#include <USB.h>
extern "C" {
  bool tud_connected(void);
  bool tud_mounted(void);
  bool tud_suspended(void);
  bool tud_remote_wakeup(void);
  bool tud_hid_n_ready(uint8_t instance);
}

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

bool usbHidReady() { CoreMutex m(&USB.mutex, false); return m && tud_hid_n_ready(0); }
void usbWakeup()   { CoreMutex m(&USB.mutex, false); if (m) tud_remote_wakeup(); }

// ---- USB health probe -------------------------------------------------------
// Once a second, if the host has configured us and the HID endpoint is free,
// queue an empty keyboard report (types nothing). If the host is polling the
// keyboard, the endpoint is free again within a few ms. If it stays busy for
// over a second, the host is not reading the keyboard.
bool hidOK = false;
bool probePending = false;
uint32_t probeAt = 0, lastProbe = 0;

void serviceProbe(uint32_t now) {
  if (!tud_mounted() || tud_suspended()) { hidOK = false; probePending = false; return; }
  if (probePending) {
    if (usbHidReady()) { hidOK = true; probePending = false; }
    else if (now - probeAt > 1000) hidOK = false;   // stuck: host is not reading
  }
  if (!probePending && now - lastProbe >= 1000 && usbHidReady()) {
    Keyboard.releaseAll();                           // empty report, endpoint ready so no wait
    probePending = true; probeAt = lastProbe = now;
  }
}

uint8_t usbState() {
  if (!tud_connected()) return 1;
  if (tud_suspended())  return 5;
  if (!tud_mounted())   return 2;
  return hidOK ? 4 : 3;
}

// ---- typing -----------------------------------------------------------------
void out(const char* s) {
  if (usbState() == 4 && usbHidReady()) Keyboard.print(s);
  else if (PicoBluetoothBLEHID.connected()) KeyboardBLE.print(s);
  tone(PIEZO, 2000, 15);
}

// ---- LED / beep code --------------------------------------------------------
uint8_t shownState = 0, codeState = 0; uint32_t codeStart = 0, stateSince = 0, lastState = 0;
void serviceCode(uint32_t now) {
  uint8_t s = usbState();
  if (s != lastState) { lastState = s; stateSince = now; }
  // only announce a state once it has been steady for 1.5 s (avoid flicker while enumerating)
  if (s != shownState && now - stateSince >= 1500) {
    shownState = s;
    for (uint8_t i = 0; i < s; i++) tone(PIEZO, 1500, 60), delay(140);   // beep the count
  }
  uint8_t n = shownState ? shownState : 1;
  uint32_t cycle = n * 450UL + 1500UL;
  if (codeStart == 0 || now - codeStart >= cycle) { codeStart = now; codeState = n; }
  uint32_t t = now - codeStart;
  bool on = (t < codeState * 450UL) && ((t % 450UL) < 220UL);
  static int last = -1;
  if (on != last) { digitalWrite(LED_BUILTIN, on); last = on; }
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
  delay(800);
  rp2040.wdt_begin(8000);
}

void loop() {
  rp2040.wdt_reset();
  uint32_t now = millis();
  serviceProbe(now);
  serviceCode(now);
  for (int i = 0; i < N; i++) {
    bool r = digitalRead(PINS[i]) == LOW;
    if (r != raw[i]) { raw[i] = r; changedAt[i] = now; }
    if (raw[i] != stable[i] && now - changedAt[i] >= 25) {
      stable[i] = raw[i];
      if (stable[i]) {
        if (tud_suspended()) usbWakeup();
        out(NAMES[i]); out(" ");
      }
    }
  }
  noInterrupts(); int32_t c = encCount; int32_t steps = c / 4; encCount = c - steps * 4; interrupts();
  for (; steps > 0; steps--) out(">");
  for (; steps < 0; steps++) out("<");
}
