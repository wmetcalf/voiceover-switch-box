/*
 * ═══════════════════════════════════════════════════════════════════
 * VoiceOver + TD Snap Adaptive Device — firmware v10.10
 * Raspberry Pi Pico 2 W · Earle Philhower arduino-pico core
 *
 *   Board: Raspberry Pi Pico 2 W
 *   Tools > IP/Bluetooth Stack > IPv4 + Bluetooth      <- REQUIRED
 *
 * Changes from v9
 *   • VoiceOver mode sends VO-prefixed commands (Ctrl+Option+…), so it
 *     works whether Quick Nav is on or off. Quick Nav no longer matters.
 *   • Keys go to USB when a host is connected, otherwise to BLE —
 *     never both (v9 double-typed when USB and BLE hit the same iPad).
 *   • Encoder decoded by interrupt (x4); key output is non-blocking, so
 *     fast wheel turns no longer drop pulses.
 *   • One wheel click = one step (v9 stepped every 1.5 clicks).
 *   • Debounce on every input, plus optional hold time / ignore-repeat
 *     for users with tremor.
 *   • Audio feedback through an optional piezo (silent if not fitted).
 *   • Mode switch: hold BACK + CHOOSER together for 3 s (v10.4; was BACK
 *     alone, which was too easy to trigger by accident). No re-plugging.
 *   • Red no longer turns VoiceOver off (a stranding risk for a blind user).
 *   • v10.1: Blue mutes/unmutes VoiceOver speech (VO+S) so it stays quiet
 *     during videos and games; VoiceOver stays on, the same button brings it back.
 *   • v10.2: wheel steps no longer pile up when spun fast. At most
 *     ENC_MAX_PENDING steps are buffered, and turning back cancels pending
 *     forward steps (VoiceOver mode), so the focus stops when the hand stops.
 *   • v10.3: USB never freezes the firmware. If the host is connected but
 *     not reading the keyboard, keys go over Bluetooth after 50 ms instead
 *     of stalling ~2 s per press. A sleeping host gets a USB wake-up request.
 *     8 s watchdog: if anything ever hangs, the device reboots itself.
 *   • v10.4: mode switch needs BACK + CHOOSER held together for 3 s, so a long
 *     press on one button can't silently change modes. BACK and CHOOSER now act
 *     on release (so starting the two-button hold sends nothing). USB serial
 *     test port: a computer can press buttons and read status (see SERIAL TEST).
 *   • v10.5: key combinations are typed like a person types them: Control +
 *     Option go down 40 ms before the key, the key is held 60 ms, and the
 *     modifiers come up after the key (v10.4 sent them all within a few ms).
 *     Hold times now start when the host has taken each report, so an iPad
 *     that polls slowly still sees the full 60 ms (it measured 1-3 ms on v10.4).
 *   • v10.6: spinning the wheel fast no longer opens Dictation on the iPad.
 *     Option goes down before Control, so Control is never pressed alone, and
 *     Control+Option stay held through a fast spin instead of tapping per step.
 *   • v10.7: in VoiceOver mode CHOOSER changes Home Screen page instead of
 *     opening the item chooser: press = next page (Option+→), hold 1 s =
 *     previous page (Option+←, a tick sounds at 1 s). Needs VoiceOver Quick
 *     Nav on (the iPad default). TD Snap mode is unchanged (Tab).
 *   • v10.8: hold BACK + GREEN for 3 s to toggle VoiceOver Quick Nav (sends
 *     Left + Right arrow together; VoiceOver says "Quick Nav on/off"). Chooser's
 *     page turning only works with Quick Nav on.
 *   • v10.9: in VoiceOver mode CHOOSER closes the current app: Home, then
 *     Cmd+Tab / Cmd+Q in the app switcher, Cmd+Esc, Home (replaces the v10.7
 *     page turning). TD Snap mode is unchanged (Tab).
 *   • v10.10: CHOOSER close-app sends media Pause first, then Cmd+W, then Home,
 *     so a playing video isn't pushed into Picture in Picture (v10.9 went Home
 *     first, which started PiP, and its app-switcher quit didn't take).
 *   • Four spare switch inputs on GP2–GP5 (not wired on the original box).
 *   • Optional battery monitoring with low-battery beeps.
 *
 * Everything you are likely to tune is in the SETTINGS block below.
 * ═══════════════════════════════════════════════════════════════════
 */

#include <Keyboard.h>
#include <KeyboardBLE.h>
#include <EEPROM.h>
#include <PicoBluetoothBLEHID.h>    // .connected() for the status report

#include <USB.h>                      // USB.mutex (arduino-pico core)

extern "C" {                          // TinyUSB device API
  bool tud_mounted(void);             // a USB host has enumerated us
  bool tud_suspended(void);           // host has put the bus to sleep
  bool tud_remote_wakeup(void);       // ask a sleeping host to wake
  bool tud_hid_n_ready(uint8_t inst); // keyboard endpoint free (host is reading it)
}

// ═══════════════════════════ SETTINGS ═════════════════════════════

// BLE name shown in iPhone/iPad Bluetooth settings.
// iOS caches the name of a device it has seen before; after changing it,
// Forget the device, toggle Bluetooth off/on, and re-pair.
#define BLE_NAME            "VoiceOver Device"
#define FW_VERSION          "v10.10"

// Wheel: quadrature counts per navigation step.
// The 60 mm encoder has 100 detents/rev and gives 4 counts per detent.
// 4 = one step per click, 8 = one step per two clicks.
#define COUNTS_PER_STEP     4
#define ENC_REVERSE         false   // true if clockwise moves backwards
#define ENC_MAX_PENDING     3       // max wheel steps buffered; extra spin is dropped

// Input timing — tune these with the student's OT.
#define DEBOUNCE_MS         25      // contact bounce filter (leave alone)
#define HOLD_MS             0       // press must last this long to count (0 = instant)
#define IGNORE_REPEAT_MS    0       // ignore re-presses of the same input within this window
#define MODE_HOLD_MS        3000    // hold BACK + CHOOSER together this long to switch mode

// Audio feedback (needs piezo on GP22; harmless if absent)
#define AUDIO_ENABLED       true
#define CLICK_ON_ACTION     false   // short tick on every action

// Battery sense (PCB with LiPo + 100k/100k divider on GP26 only)
#define BATT_SENSE_ENABLED  false
#define BATT_LOW_V          3.50f

// ═══════════════════════════ PINS ═════════════════════════════════
// Breadboard prototype pins are unchanged from v9.
#define ENC_A      10   // pin 14
#define ENC_B      11   // pin 15
#define SEL_PIN    15   // pin 20  big select button
#define RED_PIN    16   // pin 21
#define GRN_PIN    17   // pin 22
#define BLU_PIN    18   // pin 24
#define YLW_PIN    19   // pin 25
#define CHO_PIN    20   // pin 26  "Chooser"
#define BACK_PIN   21   // pin 27  "Escape" — Back (hold with CHOOSER 3 s = switch mode)
#define JACK1_PIN   2   // pin 4   spare input 1 (not wired on the original box)
#define JACK2_PIN   3   // pin 5   spare input 2
#define JACK3_PIN   4   // pin 6   spare input 3
#define JACK4_PIN   5   // pin 7   spare input 4
#define PIEZO_PIN  22   // pin 29  (optional)
#define BATT_PIN   26   // pin 31  ADC0 (optional)

// ═══════════════════════════ ACTIONS ══════════════════════════════
enum Action : uint8_t {
  A_NONE, A_NEXT, A_PREV, A_SELECT, A_BACK, A_HOME,
  A_SPEECH, A_CHOOSER, A_READ, A_MAGIC, A_TAB, A_MUTE, A_PAGE_NEXT, A_PAGE_PREV, A_CLOSE_APP
};

enum Mode : uint8_t { MODE_VOICEOVER = 0, MODE_TDSNAP = 1 };

// Input order used by the tables below
enum InputIdx : uint8_t {
  IN_SEL, IN_RED, IN_GRN, IN_BLU, IN_YLW, IN_CHO, IN_BACK,
  IN_J1, IN_J2, IN_J3, IN_J4, NUM_INPUTS
};

const uint8_t INPUT_PINS[NUM_INPUTS] = {
  SEL_PIN, RED_PIN, GRN_PIN, BLU_PIN, YLW_PIN, CHO_PIN, BACK_PIN,
  JACK1_PIN, JACK2_PIN, JACK3_PIN, JACK4_PIN
};

// What each input does in each mode.
const Action INPUT_ACTION[2][NUM_INPUTS] = {
  // VoiceOver mode
  { A_SELECT,   // big select    VO+Space    activate item
    A_READ,     // red           VO+A        read from here
    A_HOME,     // green         VO+H        Home screen
    A_MUTE,     // blue          VO+S        mute/unmute VoiceOver speech (for videos)
    A_MAGIC,    // yellow        VO+Hyphen   Magic Tap (play/pause, answer call)
    A_CLOSE_APP,// chooser       close the current app: media Pause, Cmd+W, Cmd+H
    A_BACK,     // back          Escape
    A_NEXT,     // spare 1       next item
    A_SELECT,   // spare 2       activate
    A_PREV,     // spare 3       previous item
    A_BACK },   // spare 4       back
  // TD Snap mode (TD Snap: Scanning, 2 Switch Step Scan, Space = next, Enter = select)
  { A_SELECT,   // big select    Enter
    A_NEXT,     // red           Space (spare next)
    A_SELECT,   // green         Enter (spare select)
    A_BACK,     // blue          Escape
    A_HOME,     // yellow        Cmd+H  (iPad Home)
    A_TAB,      // chooser       Tab
    A_BACK,     // back          Escape
    A_NEXT,     // spare 1       Space
    A_SELECT,   // spare 2       Enter
    A_BACK,     // spare 3       Escape
    A_NONE }    // spare 4       unused
};

// ═══════════════════════════ STATE ════════════════════════════════
Mode currentMode = MODE_VOICEOVER;
#define EEPROM_MODE_ADDR  0
#define EEPROM_MAGIC_ADDR 1
#define EEPROM_MAGIC      0xA5

struct Btn {
  bool     raw, stable;
  uint32_t rawChangedAt, pressedAt, lastFireAt;
  bool     fired, suppressed;    // suppressed: used in the mode-switch hold, don't act on release
};
Btn btn[NUM_INPUTS];

// ═══════════════════════════ ENCODER (interrupt, x4) ══════════════
volatile int32_t encCount = 0;
int32_t encPending = 0;            // wheel steps waiting to be sent (+ next, - prev)
volatile uint8_t encState = 0;
static const int8_t QDEC[16] = { 0,-1, 1, 0,  1, 0, 0,-1,  -1, 0, 0, 1,  0, 1,-1, 0 };

void encISR() {
  uint8_t s = (digitalRead(ENC_A) << 1) | digitalRead(ENC_B);
  encState = ((encState << 2) | s) & 0x0F;
  encCount += QDEC[encState];
}

// ═══════════════════════════ KEY OUTPUT (non-blocking) ════════════
// Each event is a chord of up to 4 keys. A chord is typed the way a person
// types it: modifiers down first, then the key, key up, then modifiers up.
#define MOD_LEAD_MS  40     // modifiers held this long before the key goes down
#define KEY_HOLD_MS  60     // key held down
#define MOD_TAIL_MS  20     // modifiers held this long after the key comes up
#define MOD_BRIDGE_MS 150   // keep Control+Option down this long for a following wheel step
#define KEY_GAP_MS   20     // quiet time before the next chord
#define QLEN         16
#define USB_READY_WAIT_MS  50   // host connected but not reading keys: give up on USB after this
#define USB_WAKE_WAIT_MS   300  // host asleep: wait this long for it to wake before using BLE

struct KeyEvt { uint8_t k[4]; uint8_t n; uint16_t wait; bool keep; uint16_t cons; };  // wait: ms of quiet before it; keep: tap under the previous modifiers; cons: media key (consumer usage) instead of keys
KeyEvt   keyQ[QLEN];
uint8_t  qHead = 0, qTail = 0;
enum { K_IDLE, K_MODS, K_DOWN, K_HOLDMODS, K_GAP } kState = K_IDLE;
uint32_t kSince = 0;
bool     kOnUSB = false;
KeyEvt   kCur;                                  // chord being typed

bool queueKeysEx(uint16_t wait, bool keep, uint8_t a, uint8_t b = 0, uint8_t c = 0, uint8_t d = 0) {
  uint8_t next = (qTail + 1) % QLEN;
  if (next == qHead) return false;            // full: drop (e.g. wheel spun very fast)
  KeyEvt &e = keyQ[qTail];
  e.n = 0; e.wait = wait; e.keep = keep; e.cons = 0;
  if (a) e.k[e.n++] = a;
  if (b) e.k[e.n++] = b;
  if (c) e.k[e.n++] = c;
  if (d) e.k[e.n++] = d;
  qTail = next;
  return true;
}
bool queueKeys(uint8_t a, uint8_t b = 0, uint8_t c = 0, uint8_t d = 0) { return queueKeysEx(0, false, a, b, c, d); }
bool queueMedia(uint16_t wait, uint16_t usage) {     // consumer-page key, e.g. 0x00B1 Pause
  uint8_t next = (qTail + 1) % QLEN;
  if (next == qHead) return false;
  KeyEvt &e = keyQ[qTail];
  e.n = 0; e.wait = wait; e.keep = false; e.cons = usage;
  qTail = next;
  return true;
}

// Where should the next keystroke go?  USB if a host is connected AND reading
// the keyboard; otherwise Bluetooth.  Never blocks: returns 0 to "try again
// next loop" while waiting for the host to become ready or to wake up.
// (The core's own send waits up to 500 ms per report when the host isn't
// reading, which froze v10.2 for ~2 s per press.)
uint32_t usbWaitSince = 0;
bool     usbWakeSent  = false;

bool usbHidReady() {
  CoreMutex m(&USB.mutex, false);
  return m && tud_hid_n_ready(0);
}

void usbWakeup() {
  CoreMutex m(&USB.mutex, false);
  if (m) tud_remote_wakeup();
}

uint8_t pickTransport(uint32_t now) {
  if (!tud_mounted()) { usbWaitSince = 0; usbWakeSent = false; return 2; }
  if (usbWaitSince == 0) usbWaitSince = now ? now : 1;
  if (tud_suspended()) {                                 // host asleep (e.g. iPad screen off)
    if (!usbWakeSent) { usbWakeup(); usbWakeSent = true; }
    if (now - usbWaitSince < USB_WAKE_WAIT_MS) return 0;
  } else if (usbHidReady()) {
    usbWaitSince = 0; usbWakeSent = false; return 1;     // normal case
  } else if (now - usbWaitSince < USB_READY_WAIT_MS) {
    return 0;                                            // previous report still in flight
  }
  usbWaitSince = 0; usbWakeSent = false; return 2;       // USB not usable for this key
}

// Does the next queued chord use the same modifiers as the one being typed?
bool nextHasSameMods() {
  const KeyEvt &a = keyQ[qHead];
  if (a.n != kCur.n || a.n < 2) return false;
  for (uint8_t i = 0; i + 1 < a.n; i++) if (a.k[i] != kCur.k[i]) return false;
  return true;
}

void kPress(uint8_t k)   { if (kOnUSB) Keyboard.press(k);   else KeyboardBLE.press(k); }
void kRelease(uint8_t k) { if (kOnUSB) Keyboard.release(k); else KeyboardBLE.release(k); }
void kReleaseAll()       { if (kOnUSB) Keyboard.releaseAll(); else KeyboardBLE.releaseAll(); }

// Times are counted from when the host has actually TAKEN the last report,
// not from when we queued it. An iPad that polls the keyboard slowly saw
// v10.4's 30 ms key hold shrink to 1-3 ms. Give up waiting after
// USB_READY_WAIT_MS so a host that stops reading can't stall us.
uint32_t kQueuedAt = 0;
bool     kTiming   = false;
void kEnter(int st, uint32_t now) { kState = (decltype(kState))st; kQueuedAt = now; kTiming = false; }
bool kHeld(uint32_t now, uint32_t ms) {
  if (!kTiming) {
    if (kOnUSB && !usbHidReady() && now - kQueuedAt < USB_READY_WAIT_MS) return false;
    kTiming = true; kSince = now;
  }
  return now - kSince >= ms;
}

void serviceKeys(uint32_t now) {
  switch (kState) {
    case K_IDLE:
      if (qHead != qTail) {
        uint8_t t = pickTransport(now);       // 0 = wait, 1 = USB, 2 = BLE
        if (t == 0) break;
        kOnUSB = (t == 1);
        kCur = keyQ[qHead];
        qHead = (qHead + 1) % QLEN;
        digitalWrite(LED_BUILTIN, HIGH);
        if (kCur.cons) {                      // media key
          if (kOnUSB) Keyboard.consumerPress(kCur.cons); else KeyboardBLE.consumerPress(kCur.cons);
          kEnter(K_DOWN, now);
        } else if (kCur.n > 1) {              // modifiers first
          for (uint8_t i = 0; i + 1 < kCur.n; i++) kPress(kCur.k[i]);
          kEnter(K_MODS, now);
        } else {
          kPress(kCur.k[0]);
          kEnter(K_DOWN, now);
        }
      }
      break;
    case K_MODS:
      if (kHeld(now, MOD_LEAD_MS)) { kPress(kCur.k[kCur.n - 1]); kEnter(K_DOWN, now); }
      break;
    case K_DOWN:
      if (kHeld(now, KEY_HOLD_MS)) {
        if (kCur.cons) {
          if (kOnUSB) Keyboard.consumerRelease(); else KeyboardBLE.consumerRelease();
          kEnter(K_GAP, now);
        } else if (kCur.n > 1) { kRelease(kCur.k[kCur.n - 1]); kEnter(K_HOLDMODS, now); }
        else            { kReleaseAll();                kEnter(K_GAP, now);  }
      }
      break;
    case K_HOLDMODS:
      // Key is up, modifiers still down. If the next queued chord uses the same
      // modifiers (a fast wheel spin), just tap its key: Control+Option stay down
      // for the whole spin instead of being pressed and released for every step.
      if (!kHeld(now, MOD_TAIL_MS)) break;
      if (qHead != qTail && nextHasSameMods() && (keyQ[qHead].wait == 0 || keyQ[qHead].keep)) {
        if (now - kSince < (keyQ[qHead].wait > KEY_GAP_MS ? keyQ[qHead].wait : KEY_GAP_MS)) break;
        kCur = keyQ[qHead];
        qHead = (qHead + 1) % QLEN;
        kPress(kCur.k[kCur.n - 1]);
        kEnter(K_DOWN, now);
      } else if (qHead != qTail || now - kSince >= MOD_BRIDGE_MS) {
        kReleaseAll();
        kEnter(K_GAP, now);
      }
      break;
    case K_GAP:
      if (kHeld(now, (qHead != qTail && keyQ[qHead].wait > KEY_GAP_MS) ? keyQ[qHead].wait : KEY_GAP_MS)) kState = K_IDLE;
      break;
  }
}

// ═══════════════════════════ AUDIO (non-blocking) ═════════════════
struct Beep { uint16_t f, dur, gap; };
#define BQLEN 12
Beep     beepQ[BQLEN];
uint8_t  bHead = 0, bTail = 0;
uint32_t beepNextAt = 0;

void beep(uint16_t f, uint16_t dur, uint16_t gap = 60) {
  if (!AUDIO_ENABLED) return;
  uint8_t next = (bTail + 1) % BQLEN;
  if (next == bHead) return;
  beepQ[bTail] = { f, dur, gap };
  bTail = next;
}

void serviceAudio(uint32_t now) {
  if (bHead == bTail || (int32_t)(now - beepNextAt) < 0) return;
  Beep &b = beepQ[bHead];
  if (b.f) tone(PIEZO_PIN, b.f, b.dur);
  beepNextAt = now + b.dur + b.gap;
  bHead = (bHead + 1) % BQLEN;
}

void beepMode(Mode m) {                 // 1 beep = VoiceOver, 2 beeps = TD Snap
  int n = (m == MODE_TDSNAP) ? 2 : 1;
  for (int i = 0; i < n; i++) beep(988, 120, 120);
}

// ═══════════════════════════ ACTION → KEYS ════════════════════════
// Option goes down before Control (and both come up together), so the iPad never
// sees Control pressed on its own. Two bare Control taps open Dictation on iPadOS,
// and VoiceOver swallows the arrow key, which made each wheel step look like one.
#define VO KEY_LEFT_ALT, KEY_LEFT_CTRL

void doAction(Action a) {
  if (a == A_NONE) return;
  if (CLICK_ON_ACTION) beep(2000, 8, 0);

  if (currentMode == MODE_VOICEOVER) {
    switch (a) {
      case A_NEXT:    queueKeys(VO, KEY_RIGHT_ARROW); break;
      case A_PREV:    queueKeys(VO, KEY_LEFT_ARROW);  break;
      case A_SELECT:  queueKeys(VO, ' ');             break;
      case A_BACK:    queueKeys(KEY_ESC);             break;
      case A_HOME:    queueKeys(VO, 'h');             break;
      case A_SPEECH:  queueKeys(KEY_LEFT_CTRL);       break;
      case A_MUTE:    queueKeys(VO, 's'); beep(1319, 50, 0); break;  // chirp confirms the press
      case A_CHOOSER: queueKeys(VO, 'i');             break;
      case A_CLOSE_APP:
        // Pause first (media Pause, not the play/pause toggle), so going Home
        // can't push a playing video into Picture in Picture. Then Cmd+W, which
        // closes the app's window (quits it on current iPadOS), then Home as a
        // safety net. If Cmd+W isn't supported by an app, the app is still
        // left paused in the background.
        queueMedia(0, 0x00B1);
        queueKeysEx(400, false, KEY_LEFT_GUI, 'w');
        queueKeysEx(900, false, KEY_LEFT_GUI, 'h');
        break;
      case A_PAGE_NEXT: queueKeys(KEY_LEFT_ALT, KEY_RIGHT_ARROW); break;  // VoiceOver scroll one page
      case A_PAGE_PREV: queueKeys(KEY_LEFT_ALT, KEY_LEFT_ARROW);  break;
      case A_READ:    queueKeys(VO, 'a');             break;
      case A_MAGIC:   queueKeys(VO, '-');             break;
      case A_TAB:     queueKeys(KEY_TAB);             break;
      default: break;
    }
  } else { // MODE_TDSNAP
    switch (a) {
      case A_NEXT:
      case A_PREV:    queueKeys(' ');                 break;  // step scan only moves forward
      case A_SELECT:  queueKeys(KEY_RETURN);          break;
      case A_BACK:    queueKeys(KEY_ESC);             break;
      case A_HOME:    queueKeys(KEY_LEFT_GUI, 'h');   break;
      case A_TAB:     queueKeys(KEY_TAB);             break;
      default: break;                                         // speech/chooser/read/magic: no TD Snap equivalent
    }
  }
}

// ═══════════════════════════ MODE ═════════════════════════════════
void setMode(Mode m, bool announce) {
  currentMode = m;
  encPending = 0;
  EEPROM.write(EEPROM_MODE_ADDR, (uint8_t)m);
  EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC);
  EEPROM.commit();
  if (announce) {
    if (m == MODE_TDSNAP) { beep(660, 90, 30); beep(988, 140, 300); }   // rising
    else                  { beep(988, 90, 30); beep(660, 140, 300); }   // falling
    beepMode(m);
  }
}

// ═══════════════════════════ INPUTS ═══════════════════════════════
bool repeatOK(Btn &b, uint32_t now) {
  return IGNORE_REPEAT_MS == 0 || (now - b.lastFireAt) >= IGNORE_REPEAT_MS || b.lastFireAt == 0;
}

// BACK and CHOOSER act on release, so that starting the BACK + CHOOSER
// mode-switch hold sends nothing. Every other input acts on press.
bool actsOnRelease(uint8_t i) { return i == IN_BACK || i == IN_CHO; }

bool comboDone = false;
bool qnDone = false;

void serviceInputs(uint32_t now) {
  for (uint8_t i = 0; i < NUM_INPUTS; i++) {
    Btn &b = btn[i];
    bool r = (digitalRead(INPUT_PINS[i]) == LOW);
    if (r != b.raw) { b.raw = r; b.rawChangedAt = now; }

    // Debounced edge
    if (b.raw != b.stable && (now - b.rawChangedAt) >= DEBOUNCE_MS) {
      b.stable = b.raw;
      if (b.stable) {                       // press
        b.pressedAt = now; b.fired = false; b.suppressed = false;
      } else if (actsOnRelease(i)) {        // release
        if (!b.suppressed && (now - b.pressedAt) >= HOLD_MS && repeatOK(b, now)) {
          Action a = INPUT_ACTION[currentMode][i];
          doAction(a);
          b.lastFireAt = now;
        }
      }
    }

    if (!b.stable || actsOnRelease(i)) continue;

    if (!b.fired && (now - b.pressedAt) >= HOLD_MS) {
      b.fired = true;
      if (repeatOK(b, now)) {
        doAction(INPUT_ACTION[currentMode][i]);
        b.lastFireAt = now;
      }
    }
  }

  // Mode switch: BACK + CHOOSER both held for MODE_HOLD_MS (timed from the
  // second press). Once both are down together, neither acts on release,
  // even if they're let go early.
  Btn &bk = btn[IN_BACK], &ch = btn[IN_CHO];
  if (bk.stable && ch.stable) {
    bk.suppressed = ch.suppressed = true;
    uint32_t since = (bk.pressedAt > ch.pressedAt) ? bk.pressedAt : ch.pressedAt;
    if (!comboDone && (now - since) >= MODE_HOLD_MS) {
      comboDone = true;
      setMode(currentMode == MODE_VOICEOVER ? MODE_TDSNAP : MODE_VOICEOVER, true);
    }
  } else {
    comboDone = false;
  }

  // Quick Nav toggle: BACK + GREEN both held for MODE_HOLD_MS (VoiceOver mode).
  // Sends Left + Right arrow together, VoiceOver's keyboard command for Quick
  // Nav on/off; VoiceOver says which. Quick Nav can only be switched from a
  // keyboard. (Green still sends Home when it goes down, which is harmless.)
  Btn &gr = btn[IN_GRN];
  if (bk.stable && gr.stable) {
    bk.suppressed = true;
    uint32_t since = (bk.pressedAt > gr.pressedAt) ? bk.pressedAt : gr.pressedAt;
    if (!qnDone && (now - since) >= MODE_HOLD_MS) {
      qnDone = true;
      if (currentMode == MODE_VOICEOVER) { queueKeys(KEY_LEFT_ARROW, KEY_RIGHT_ARROW); beep(880, 60, 40); beep(1175, 60, 0); }
    }
  } else {
    qnDone = false;
  }
}

void serviceEncoder() {
  noInterrupts();
  int32_t c = encCount;
  int32_t steps = c / COUNTS_PER_STEP;          // truncates toward zero
  encCount = c - steps * COUNTS_PER_STEP;
  interrupts();
  if (ENC_REVERSE) steps = -steps;

  if (currentMode == MODE_TDSNAP) encPending += (steps < 0 ? -steps : steps);  // any direction = advance
  else                            encPending += steps;                        // turning back cancels forward
  if (encPending >  ENC_MAX_PENDING) encPending =  ENC_MAX_PENDING;
  if (encPending < -ENC_MAX_PENDING) encPending = -ENC_MAX_PENDING;

  // Release one step only when the keyboard is idle, so nothing piles up behind it.
  if (encPending != 0 && qHead == qTail && (kState == K_IDLE || kState == K_HOLDMODS)) {
    if (encPending > 0) { doAction(A_NEXT); encPending--; }
    else                { doAction(A_PREV); encPending++; }
  }
}

// ═══════════════════════════ SERIAL TEST ══════════════════════════
// For testing from a computer over the same USB cable (the box's USB serial
// port, 115200 baud). One character per command:
//   n / p      wheel one click forward / back
//   s          big select          r g b y c   red green blue yellow chooser
//   e          back (Escape)       1 2 3 4     spare inputs
//   m          switch mode         ?           print status
//   q          toggle VoiceOver Quick Nav (same as holding Back + Green 3 s)
// Buttons go through the same action table and keyboard path as the real
// switches. An iPad never opens this port, so it has no effect in normal use.
const char* modeName() { return currentMode == MODE_TDSNAP ? "TD Snap" : "VoiceOver"; }

void printStatus() {
  Serial.print("VoiceOver Device firmware " FW_VERSION " | mode: ");
  Serial.print(modeName());
  Serial.print(" | USB keyboard: ");
  if (!tud_mounted())        Serial.print("not connected");
  else if (tud_suspended())  Serial.print("host asleep");
  else if (usbHidReady())    Serial.print("connected, host reading keys");
  else                       Serial.print("connected, host NOT reading keys");
  Serial.print(" | Bluetooth: ");
  Serial.println(PicoBluetoothBLEHID.connected() ? "connected" : "not connected");
}

void serviceSerial() {
  // One command at a time, and only once the previous one has been typed,
  // so a burst like "nps" comes out in order (and n+p don't cancel out).
  while (qHead == qTail && (kState == K_IDLE || kState == K_HOLDMODS) && encPending == 0 && Serial.available() > 0) {
    int c = Serial.read();
    int idx = -1, wheel = 0;
    const char* what = nullptr;
    switch (c) {
      case 'n': wheel = +1; what = "wheel forward"; break;
      case 'p': wheel = -1; what = "wheel back";    break;
      case 's': idx = IN_SEL;  what = "select";  break;
      case 'r': idx = IN_RED;  what = "red";     break;
      case 'g': idx = IN_GRN;  what = "green";   break;
      case 'b': idx = IN_BLU;  what = "blue";    break;
      case 'y': idx = IN_YLW;  what = "yellow";  break;
      case 'c': idx = IN_CHO;  what = "chooser"; break;
      case 'e': idx = IN_BACK; what = "back";    break;
      case '1': idx = IN_J1;   what = "jack 1";  break;
      case '2': idx = IN_J2;   what = "jack 2";  break;
      case '3': idx = IN_J3;   what = "jack 3";  break;
      case '4': idx = IN_J4;   what = "jack 4";  break;
      case 'q':                              // Back + Green hold: toggle Quick Nav
        if (currentMode == MODE_VOICEOVER) queueKeys(KEY_LEFT_ARROW, KEY_RIGHT_ARROW);
        Serial.println("> quick nav toggle");
        return;
      case 'm':
        setMode(currentMode == MODE_VOICEOVER ? MODE_TDSNAP : MODE_VOICEOVER, true);
        Serial.print("> mode now "); Serial.println(modeName());
        continue;
      case '?': printStatus(); continue;
      default: continue;                   // ignore newlines and anything else
    }
    if (wheel) {                           // feed the real wheel decoder
      noInterrupts();
      encCount += (ENC_REVERSE ? -wheel : wheel) * COUNTS_PER_STEP;
      interrupts();
    } else {
      doAction(INPUT_ACTION[currentMode][idx]);
    }
    Serial.print("> "); Serial.println(what);
    break;                                 // next command on a later loop
  }
}

// ═══════════════════════════ LED / BATTERY ════════════════════════
uint32_t ledTimer = 0;
bool     ledOn    = false;

void serviceLED(uint32_t now) {
  if (kState != K_IDLE) return;                   // keypress flash in progress
  uint32_t interval = (currentMode == MODE_TDSNAP) ? 500 : 1000;  // toggles every 0.5 s (TD Snap) or 1 s (VoiceOver)
  if (now - ledTimer >= interval) {
    ledOn = !ledOn; ledTimer = now;
    digitalWrite(LED_BUILTIN, ledOn ? HIGH : LOW);
  }
}

uint32_t battCheckAt = 0, battWarnAt = 0;
void serviceBattery(uint32_t now) {
  if (!BATT_SENSE_ENABLED || now < battCheckAt) return;
  battCheckAt = now + 10000;
  float v = analogRead(BATT_PIN) * (3.3f / 4095.0f) * 2.0f;   // 100k/100k divider
  if (v > 2.5f && v < BATT_LOW_V && now >= battWarnAt) {       // >2.5 V: ignore if unpowered/absent
    beep(440, 200, 120); beep(440, 200, 120); beep(440, 200, 0);
    battWarnAt = now + 60000;
  }
}

// ═══════════════════════════ SETUP / LOOP ═════════════════════════
void setup() {
  for (uint8_t i = 0; i < NUM_INPUTS; i++) {
    pinMode(INPUT_PINS[i], INPUT_PULLUP);
    btn[i] = { false, false, 0, 0, 0, false, false };
  }
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIEZO_PIN, OUTPUT);
  if (BATT_SENSE_ENABLED) analogReadResolution(12);

  encState = ((digitalRead(ENC_A) << 1) | digitalRead(ENC_B));
  attachInterrupt(digitalPinToInterrupt(ENC_A), encISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), encISR, CHANGE);

  EEPROM.begin(256);
  if (EEPROM.read(EEPROM_MAGIC_ADDR) == EEPROM_MAGIC && EEPROM.read(EEPROM_MODE_ADDR) <= MODE_TDSNAP)
    currentMode = (Mode)EEPROM.read(EEPROM_MODE_ADDR);
  else
    setMode(MODE_VOICEOVER, false);

  Keyboard.begin();
  KeyboardBLE.begin(BLE_NAME);

  // Boot indication: LED flashes 3× (VoiceOver) or 6× (TD Snap) + mode beeps
  int flashes = (currentMode == MODE_TDSNAP) ? 6 : 3;
  for (int i = 0; i < flashes; i++) {
    digitalWrite(LED_BUILTIN, HIGH); delay(100);
    digitalWrite(LED_BUILTIN, LOW);  delay(100);
  }
  beepMode(currentMode);

  rp2040.wdt_begin(8000);   // reboot if loop() ever stops running for 8 s
}

void loop() {
  rp2040.wdt_reset();
  uint32_t now = millis();
  serviceInputs(now);
  serviceSerial();
  serviceEncoder();
  serviceKeys(now);
  serviceAudio(now);
  serviceLED(now);
  serviceBattery(now);
}
