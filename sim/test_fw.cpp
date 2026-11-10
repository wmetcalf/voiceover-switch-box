// Host-side simulator: runs the real firmware (.ino) against mocked hardware
// with virtual time, then checks the keystrokes and beeps it produces.
// Build: g++ -std=c++17 -Imock -DFW='"path/to/sketch.ino"' test_fw.cpp -o sim
// Run:   ./sim <scenario> <eeprom-file>
#include "Arduino.h"
#include "HIDMock.h"
#include "EEPROM.h"
#include "PicoBluetoothBLEHID.h"
#include <functional>
#include <algorithm>
#include <map>

uint32_t g_now = 0;
int g_pin[128];
void (*g_isr[128])() = {};
bool g_usb_mounted = false;
std::vector<ToneEvt> g_tones;
std::vector<KeyEvt2> g_keys;
int g_led = 0, g_adc = 0, g_violations = 0;
std::vector<uint8_t> g_down_usb, g_down_ble;
uint32_t g_down_t_usb = 0, g_down_t_ble = 0;
const char* g_eeprom_path = "eeprom.bin";
MockKbd Keyboard(true), KeyboardBLE(false);
MockEEPROM EEPROM;
extern "C" bool tud_mounted(void) { return g_usb_mounted; }
#include "USB.h"
MockUSB USB;
MockRP2040 rp2040;
bool g_usb_suspended = false, g_hid_ready = true;
int g_wake_calls = 0, g_resume_after = -1;   // ms until a woken host resumes; -1 = never
uint32_t g_wake_at = 0;
extern "C" bool tud_suspended(void) { return g_usb_suspended; }
extern "C" bool tud_remote_wakeup(void) { g_wake_calls++; g_wake_at = g_now; return true; }
uint32_t g_poll_ms = 0, g_last_report = 0;          // simulated host polling interval
extern "C" bool tud_hid_n_ready(uint8_t) { return g_hid_ready && (g_poll_ms == 0 || g_now - g_last_report >= g_poll_ms); }
MockSerial Serial;
int g_ctrl_presses = 0, g_ctrl_alone = 0;
bool g_ble_connected = false;
MockBLEHID PicoBluetoothBLEHID;

#include FW

// ── harness ──────────────────────────────────────────────────────────
static int failures = 0;
static std::string scen;
#define CHECK(cond, msg) do { if (!(cond)) { failures++; printf("  FAIL %s: %s\n", scen.c_str(), std::string(msg).c_str()); } } while (0)

void tick(int ms) { for (int i = 0; i < ms; i++) { loop(); loop(); g_now++;
  if (g_usb_suspended && g_wake_calls && g_resume_after >= 0 && g_now - g_wake_at >= (uint32_t)g_resume_after) g_usb_suspended = false; } }
void setPin(int p, int v) { if (g_pin[p] != v) { g_pin[p] = v; if (g_isr[p]) g_isr[p](); } }
void press(int p)   { setPin(p, LOW); }
void release(int p) { setPin(p, HIGH); }
void tap(int p, int holdMs = 150, int after = 150) { press(p); tick(holdMs); release(p); tick(after); }
// Mode switch gesture: hold BACK + CHOOSER together, then let go.
void comboHold(int ms = 3500, bool chooserFirst = false) {
  if (chooserFirst) { press(CHO_PIN); tick(200); press(BACK_PIN); } else { press(BACK_PIN); tick(200); press(CHO_PIN); }
  tick(ms); release(BACK_PIN); release(CHO_PIN); tick(1500); }

const int GRAY[4] = {0, 2, 3, 1};               // (A<<1)|B, forward order
void encEdge(int dir) {
  int cur = (g_pin[ENC_A] << 1) | g_pin[ENC_B], i = 0;
  while (GRAY[i] != cur) i++;
  int nxt = GRAY[(i + dir + 4) % 4];
  setPin(ENC_A, (nxt >> 1) & 1);
  setPin(ENC_B, nxt & 1);
}
void detent(int dir, int msPerEdge = 2) { for (int e = 0; e < 4; e++) { encEdge(dir); tick(msPerEdge); } }

void boot() {
  for (int i = 0; i < 128; i++) g_pin[i] = HIGH;
  setup();
  tick(1500);                                   // let boot beeps play out
}

std::string keyName(uint8_t k) {
  static std::map<uint8_t, std::string> n = {{0x80, "Ctrl"}, {0x81, "Shift"}, {0x82, "Opt"}, {0x83, "Cmd"}, {0xB0, "Enter"},
    {0xB1, "Esc"}, {0xB3, "Tab"}, {0xC6, "F5"}, {0xD7, "Right"}, {0xD8, "Left"}, {0xD9, "Down"}, {0xDA, "Up"}, {' ', "Space"}, {0xFE, "Pause"}};
  if (n.count(k)) return n[k];
  return std::string(1, (char)k);
}
std::string chord(const KeyEvt2& e) {           // modifiers in a fixed order, then the key
  std::vector<uint8_t> k = e.keys;
  std::stable_sort(k.begin(), k.end(), [](uint8_t a, uint8_t b) { return isMod(a) && isMod(b) ? a < b : isMod(a) && !isMod(b); });
  std::string s; for (auto x : k) s += (s.empty() ? "" : "+") + keyName(x); return s; }
std::vector<std::string> chords(size_t from = 0) { std::vector<std::string> v; for (size_t i = from; i < g_keys.size(); i++) v.push_back(chord(g_keys[i])); return v; }
std::string join(const std::vector<std::string>& v) { std::string s; for (auto& x : v) s += (s.empty() ? "" : ", ") + x; return "[" + s + "]"; }
void expectKeys(size_t from, const std::vector<std::string>& want, const char* what) {
  auto got = chords(from);
  CHECK(got == want, std::string(what) + ": want " + join(want) + " got " + join(got));
}
int beepsAt(uint16_t f, uint32_t since) { int n = 0; for (auto& t : g_tones) if (t.f == f && t.t >= since) n++; return n; }

void checkTiming() {
  for (size_t i = 0; i < g_keys.size(); i++) {
    const KeyEvt2& e = g_keys[i];
    CHECK(e.tUp - e.tMain >= KEY_HOLD_MS, "key held too briefly at event " + std::to_string(i));
    bool sameHold = i && g_keys[i - 1].tDown == e.tDown && e.keys.size() > 1;   // tapped under held modifiers
    if (e.keys.size() > 1) {
      if (!sameHold) CHECK(e.tMain - e.tDown >= MOD_LEAD_MS, "modifiers must go down before the key at event " + std::to_string(i));
      CHECK(e.tAllUp == 0 || e.tAllUp - e.tUp >= MOD_TAIL_MS || (i + 1 < g_keys.size() && g_keys[i + 1].tDown == e.tDown),
            "modifiers must stay down after the key at event " + std::to_string(i));
    }
    if (i) {
      if (sameHold) CHECK(e.tMain - g_keys[i - 1].tUp >= KEY_GAP_MS, "gap too short between taps before event " + std::to_string(i));
      else { uint32_t prevEnd = g_keys[i - 1].tAllUp ? g_keys[i - 1].tAllUp : g_keys[i - 1].tUp;
             CHECK(e.tDown - prevEnd >= KEY_GAP_MS, "gap too short before event " + std::to_string(i)); }
    }
  }
  CHECK(g_violations == 0, "keys held on USB and BLE at the same time");
}

const std::string NEXT = "Ctrl+Opt+Right", PREV = "Ctrl+Opt+Left", ACT = "Ctrl+Opt+Space";

// ── scenarios ────────────────────────────────────────────────────────
std::map<std::string, std::function<void()>> S = {
  {"boot_default_voiceover", [] {
    boot();
    CHECK(currentMode == MODE_VOICEOVER, "fresh device should boot in VoiceOver mode");
    CHECK(beepsAt(988, 0) == 1, "expected 1 boot beep, got " + std::to_string(beepsAt(988, 0)));
    CHECK(g_keys.empty(), "no keys at boot");
  }},
  {"wheel_one_click_one_step", [] {
    boot(); detent(+1); tick(300);
    expectKeys(0, {NEXT}, "one click forward");
    detent(-1); tick(300);
    expectKeys(0, {NEXT, PREV}, "then one click back");
    CHECK(!g_keys.empty() && !g_keys[0].usb, "no USB host -> should go over BLE");
  }},
  {"wheel_ten_slow", [] {
    boot(); for (int i = 0; i < 10; i++) { detent(+1); tick(100); } tick(500);
    expectKeys(0, std::vector<std::string>(10, NEXT), "10 slow clicks");
    checkTiming();
  }},
  {"wheel_jitter_no_step", [] {
    boot(); for (int i = 0; i < 50; i++) { encEdge(+1); tick(1); encEdge(-1); tick(1); } tick(300);
    CHECK(g_keys.empty(), "contact jitter at a detent must not step; got " + join(chords()));
  }},
  {"wheel_fast_spin", [] {
    boot(); for (int i = 0; i < 40; i++) detent(+1, 0); tick(1); // 40 clicks in ~0 ms
    tick(5000);
    printf("  info: 40 instant clicks -> %zu Next keystrokes\n", g_keys.size());
    CHECK(g_keys.size() >= 1 && g_keys.size() <= 40, "fast spin output out of range");
    checkTiming();
  }},
  {"wheel_fast_then_reverse", [] {
    boot(); for (int i = 0; i < 40; i++) detent(+1, 0); tick(100);
    for (int i = 0; i < 5; i++) detent(-1, 0);
    tick(5000);
    auto v = chords(); int lastNext = -1, firstPrev = -1;
    for (int i = 0; i < (int)v.size(); i++) { if (v[i] == NEXT) lastNext = i; if (v[i] == PREV && firstPrev < 0) firstPrev = i; }
    printf("  info: %zu keystrokes, first Prev at #%d, last Next at #%d\n", v.size(), firstPrev, lastNext);
    CHECK(firstPrev >= 0, "reversing after a fast spin must produce Prev steps");
    CHECK(firstPrev < 0 || lastNext < firstPrev, "no Next steps should fire after the user reverses");
  }},
  {"wheel_stops_when_hand_stops", [] {
    boot(); for (int i = 0; i < 40; i++) detent(+1, 0);
    uint32_t stop = g_now; tick(5000);
    uint32_t lastDown = g_keys.empty() ? stop : g_keys.back().tDown;
    printf("  info: 40 instant clicks -> %zu steps, last one %u ms after the wheel stopped\n", g_keys.size(), lastDown - stop);
    CHECK(lastDown - stop <= (ENC_MAX_PENDING + 1) * (MOD_LEAD_MS + KEY_HOLD_MS + MOD_TAIL_MS + KEY_GAP_MS) + 20, "steps keep firing long after the wheel stopped");
  }},
  {"wheel_reverse_is_honoured", [] {
    boot(); for (int i = 0; i < 40; i++) detent(+1, 0); tick(60);
    size_t s = g_keys.size(); for (int i = 0; i < 5; i++) detent(-1, 0); tick(3000);
    auto v = chords(s); int prevs = 0; for (auto& x : v) prevs += (x == PREV);
    printf("  info: after reversing 5 clicks: %s\n", join(v).c_str());
    CHECK(prevs >= 1 && v.back() == PREV, "reversal must end on Prev steps");
  }},
  {"tdsnap_back_and_forth_both_advance", [] {
    boot(); comboHold();
    size_t s = g_keys.size(); detent(+1, 0); tick(1); detent(-1, 0); tick(500);
    expectKeys(s, {"Space", "Space"}, "TD Snap: quick forward+back = two advances");
  }},
  {"select_activate_once", [] {
    boot(); press(SEL_PIN); tick(1000); release(SEL_PIN); tick(300);
    expectKeys(0, {ACT}, "holding Select fires once");
  }},
  {"select_bounce_debounced", [] {
    boot();
    for (int i = 0; i < 6; i++) { press(SEL_PIN); tick(1); release(SEL_PIN); tick(1); }
    press(SEL_PIN); tick(200); release(SEL_PIN);
    for (int i = 0; i < 6; i++) { press(SEL_PIN); tick(1); release(SEL_PIN); tick(1); }
    tick(300);
    expectKeys(0, {ACT}, "bouncy press is one activation");
  }},
  {"vo_button_map", [] {
    boot();
    uint32_t t0 = g_now;
    tap(RED_PIN); tap(GRN_PIN); tap(BLU_PIN); tap(YLW_PIN); tap(CHO_PIN); tap(BACK_PIN);
    tick(5000);
    expectKeys(0, {"Ctrl+Opt+a", "Ctrl+Opt+h", "Ctrl+Opt+s", "Ctrl+Opt+-", "Pause", "Cmd+w", "Cmd+h", "Esc"}, "VoiceOver map");
    CHECK(beepsAt(1319, t0) == 1, "Blue (mute) should chirp once");
    checkTiming();
  }},
  {"back_short_fires_on_release", [] {
    boot(); press(BACK_PIN); tick(400);
    CHECK(g_keys.empty(), "Back must not fire while still held");
    release(BACK_PIN); tick(300);
    expectKeys(0, {"Esc"}, "Back on release");
  }},
  {"back_long_hold_alone_is_just_back", [] {
    boot(); press(BACK_PIN); tick(6000);
    CHECK(g_keys.empty(), "Back must not fire while still held");
    release(BACK_PIN); tick(300);
    CHECK(currentMode == MODE_VOICEOVER, "holding Back alone must NOT switch mode any more");
    expectKeys(0, {"Esc"}, "a long Back press is still one Back");
  }},
  {"combo_hold_switches_mode", [] {
    boot(); uint32_t t0 = g_now;
    comboHold();
    CHECK(currentMode == MODE_TDSNAP, "Back + Chooser held 3.5 s should switch to TD Snap");
    CHECK(g_keys.empty(), "the mode-switch hold must not send Esc or VO+I; got " + join(chords()));
    CHECK(beepsAt(660, t0) == 1 && beepsAt(988, t0) == 3, "rising tones + 2 mode beeps expected");
  }},
  {"combo_chooser_first_also_works", [] {
    boot(); comboHold(3500, true);
    CHECK(currentMode == MODE_TDSNAP, "either press order should work");
    CHECK(g_keys.empty(), "no keys during the hold; got " + join(chords()));
    comboHold();
    CHECK(currentMode == MODE_VOICEOVER, "doing it again switches back");
  }},
  {"combo_timed_from_second_press", [] {
    boot(); press(BACK_PIN); tick(5000); press(CHO_PIN); tick(MODE_HOLD_MS - 300);
    CHECK(currentMode == MODE_VOICEOVER, "the 3 s starts when the second button goes down");
    tick(600); release(BACK_PIN); release(CHO_PIN); tick(1500);
    CHECK(currentMode == MODE_TDSNAP, "switches once both have been held 3 s");
    CHECK(g_keys.empty(), "no keys; got " + join(chords()));
  }},
  {"combo_let_go_early_does_nothing", [] {
    boot(); press(BACK_PIN); tick(100); press(CHO_PIN); tick(MODE_HOLD_MS - 500);
    release(CHO_PIN); tick(50); release(BACK_PIN); tick(500);
    CHECK(currentMode == MODE_VOICEOVER, "letting go early keeps the mode");
    CHECK(g_keys.empty(), "an abandoned two-button hold sends nothing; got " + join(chords()));
  }},
  {"combo_held_long_switches_once", [] {
    boot(); comboHold(10000);
    CHECK(currentMode == MODE_TDSNAP, "holding way past 3 s switches exactly once");
  }},
  {"chooser_acts_on_release", [] {
    boot(); press(CHO_PIN); tick(400);
    CHECK(g_keys.empty(), "Chooser must wait for release");
    release(CHO_PIN); tick(4000);
    expectKeys(0, {"Pause", "Cmd+w", "Cmd+h"}, "Chooser on release = close the app");
  }},
  {"chooser_in_tdsnap_is_still_tab", [] {
    boot(); comboHold(); size_t s = g_keys.size();
    tap(CHO_PIN, 150, 300); press(CHO_PIN); tick(1500); release(CHO_PIN); tick(400);
    expectKeys(s, {"Tab", "Tab"}, "TD Snap: Chooser is Tab, short or long");
  }},
  {"back_green_hold_toggles_quick_nav", [] {
    boot(); press(BACK_PIN); tick(100); press(GRN_PIN); tick(MODE_HOLD_MS + 300);
    release(GRN_PIN); release(BACK_PIN); tick(800);
    expectKeys(0, {"Ctrl+Opt+h", "Left+Right"}, "Green's Home on press, then Left+Right together (Quick Nav toggle), no Esc");
    CHECK(currentMode == MODE_VOICEOVER, "mode unchanged");
    checkTiming();
  }},
  {"back_green_let_go_early_no_toggle", [] {
    boot(); press(BACK_PIN); tick(100); press(GRN_PIN); tick(1000);
    release(GRN_PIN); release(BACK_PIN); tick(500);
    expectKeys(0, {"Ctrl+Opt+h"}, "only Green's Home; no toggle, no Esc");
  }},
  {"close_app_sequence_timing", [] {
    boot(); g_usb_mounted = true; tap(CHO_PIN, 150, 5000);
    expectKeys(0, {"Pause", "Cmd+w", "Cmd+h"}, "close-app sequence");
    if (g_keys.size() == 3) {
      const KeyEvt2 &pz = g_keys[0], &w = g_keys[1], &h = g_keys[2];
      CHECK(w.tDown - pz.tUp >= 400, "pause before Cmd+W");
      CHECK(h.tDown - w.tAllUp >= 900, "pause before the final Home");
      printf("  info: close app takes %u ms\n", h.tAllUp - pz.tDown);
    }
    CHECK(g_ctrl_alone == 0, "no bare Control");
    checkTiming();
  }},
  {"chooser_tdsnap_still_tab_after_change", [] {
    boot(); comboHold(); size_t s0 = g_keys.size(); tap(CHO_PIN, 150, 300);
    expectKeys(s0, {"Tab"}, "TD Snap Chooser = Tab");
  }},
  {"serial_test_port", [] {
    boot();
    Serial.in = "nps\nrgbyce1234"; tick(8000);   // 1-4 were the spare inputs: now ignored
    expectKeys(0, {NEXT, PREV, ACT, "Ctrl+Opt+a", "Ctrl+Opt+h", "Ctrl+Opt+s", "Ctrl+Opt+-", "Pause", "Cmd+w", "Cmd+h", "Esc"},
               "serial commands drive the same actions as the switches");
    CHECK(Serial.out.find("> wheel forward") != std::string::npos && Serial.out.find("> back") != std::string::npos && Serial.out.find("jack") == std::string::npos, "each command is echoed");
    Serial.out.clear(); g_usb_mounted = true; g_ble_connected = false; Serial.in = "?"; tick(10);
    CHECK(Serial.out.find("v10.11") != std::string::npos && Serial.out.find("mode: VoiceOver") != std::string::npos
          && Serial.out.find("host reading keys") != std::string::npos, "status line: " + Serial.out);
    g_hid_ready = false; Serial.out.clear(); Serial.in = "?"; tick(10);
    CHECK(Serial.out.find("NOT reading") != std::string::npos, "status should flag a host that isn't reading: " + Serial.out);
    g_hid_ready = true; Serial.out.clear(); Serial.in = "m?"; tick(1500);
    CHECK(currentMode == MODE_TDSNAP && Serial.out.find("mode now TD Snap") != std::string::npos, "m switches mode: " + Serial.out);
    size_t s = g_keys.size(); Serial.in = "nps"; tick(1000);
    expectKeys(s, {"Space", "Space", "Enter"}, "serial in TD Snap mode");
    checkTiming();
  }},
  {"mode_persists_reboot", [] {           // run after combo_hold_switches_mode with the same EEPROM file
    boot();
    CHECK(currentMode == MODE_TDSNAP, "mode should survive power-off");
    CHECK(beepsAt(988, 0) == 2, "TD Snap boot = 2 beeps, got " + std::to_string(beepsAt(988, 0)));
  }},
  {"tdsnap_map", [] {
    boot(); comboHold();
    size_t s = g_keys.size();
    detent(+1); tick(200); detent(-1); tick(200);
    tap(SEL_PIN); tap(RED_PIN); tap(GRN_PIN); tap(BLU_PIN); tap(YLW_PIN); tap(CHO_PIN); tap(BACK_PIN);
    tick(300);
    expectKeys(s, {"Space", "Space", "Enter", "Space", "Enter", "Esc", "Cmd+h", "Tab", "Esc"}, "TD Snap map");
    checkTiming();
  }},
  {"usb_only_when_host_present", [] {
    boot(); g_usb_mounted = true; detent(+1); tick(300);
    g_usb_mounted = false; detent(+1); tick(300);
    CHECK(g_keys.size() == 2 && g_keys[0].usb && !g_keys[1].usb, "USB when mounted, BLE otherwise");
    checkTiming();
  }},
  {"usb_not_read_falls_back_to_ble", [] {   // host enumerated us but never reads the keyboard
    boot(); g_usb_mounted = true; g_hid_ready = false;
    uint32_t t0 = g_now; detent(+1); tap(SEL_PIN, 150, 600);
    CHECK(g_keys.size() == 2 && !g_keys[0].usb && !g_keys[1].usb, "keys should go over BLE when USB isn't being read");
    CHECK(!g_keys.empty() && g_keys[0].tDown - t0 < USB_READY_WAIT_MS + 30, "fallback should take ~50 ms, not seconds");
    expectKeys(0, {NEXT, ACT}, "same keys, just over BLE");
  }},
  {"sleeping_host_is_woken_then_gets_key", [] {
    boot(); g_usb_mounted = true; g_usb_suspended = true; g_resume_after = 40;
    detent(+1); tick(500);
    CHECK(g_wake_calls == 1, "one wake-up request expected, got " + std::to_string(g_wake_calls));
    CHECK(g_keys.size() == 1 && g_keys[0].usb, "after waking, the key should go over USB");
  }},
  {"sleeping_host_that_never_wakes_uses_ble", [] {
    boot(); g_usb_mounted = true; g_usb_suspended = true; g_resume_after = -1;
    uint32_t t0 = g_now; detent(+1); tick(800);
    CHECK(g_keys.size() == 1 && !g_keys[0].usb, "should fall back to BLE");
    CHECK(g_keys.size() == 1 && g_keys[0].tDown - t0 >= USB_WAKE_WAIT_MS, "should wait for the host to wake first");
  }},
  {"chord_typed_like_a_person", [] {
    boot(); g_usb_mounted = true; detent(+1); tick(400); tap(RED_PIN); tick(400);
    CHECK(g_keys.size() == 2, "two chords expected");
    for (auto& e : g_keys) {
      CHECK(e.tMain - e.tDown >= MOD_LEAD_MS, "Control+Option must be down before the key");
      CHECK(e.tUp - e.tMain >= KEY_HOLD_MS, "key held long enough");
      CHECK(e.tAllUp - e.tUp >= MOD_TAIL_MS, "modifiers released after the key");
    }
    printf("  info: chord = mods down, +%u ms key down, +%u ms key up, +%u ms mods up\n",
           g_keys[0].tMain - g_keys[0].tDown, g_keys[0].tUp - g_keys[0].tMain, g_keys[0].tAllUp - g_keys[0].tUp);
    checkTiming();
  }},
  {"slow_polling_host_still_sees_full_hold", [] {   // iPad measured 1-3 ms holds on v10.4
    boot(); g_usb_mounted = true; g_poll_ms = 25;
    detent(+1); tick(800);
    CHECK(g_keys.size() == 1 && g_keys[0].usb, "one USB chord");
    if (g_keys.size() == 1) {
      const KeyEvt2& e = g_keys[0];
      printf("  info: 25 ms polling: key queued down->up %u ms (host sees >= %u ms)\n", e.tUp - e.tMain, e.tUp - e.tMain - g_poll_ms);
      CHECK(e.tUp - e.tMain >= KEY_HOLD_MS + g_poll_ms - 1, "hold must be timed from delivery, not from queueing");
      CHECK(e.tMain - e.tDown >= MOD_LEAD_MS + g_poll_ms - 1, "modifier lead must be timed from delivery");
    }
  }},
  {"host_stops_reading_mid_chord_does_not_hang", [] {
    boot(); g_usb_mounted = true; g_poll_ms = 1000000;   // host never takes another report
    g_last_report = g_now; detent(+1); tick(2000);
    CHECK(g_keys.size() <= 1 && kState == K_IDLE, "chord must finish (time out) rather than hang");
  }},
  {"control_never_pressed_alone", [] {      // two bare Control taps = iPad Dictation
    boot(); g_usb_mounted = true;
    for (int i = 0; i < 5; i++) { detent(+1); tick(400); }
    tap(RED_PIN); tap(GRN_PIN); tick(500);
    CHECK(g_ctrl_presses > 0, "Control should be used");
    CHECK(g_ctrl_alone == 0, "Control went down without Option " + std::to_string(g_ctrl_alone) + " times");
  }},
  {"fast_spin_holds_modifiers", [] {
    boot(); g_usb_mounted = true;
    for (int i = 0; i < 12; i++) { detent(+1); tick(70); }         // ~14 clicks/s
    tick(1500);
    auto v = chords(); int nexts = 0; for (auto& x : v) nexts += (x == NEXT);
    printf("  info: 12 clicks at 70 ms -> %d Next steps, Control pressed %d time(s)\n", nexts, g_ctrl_presses);
    CHECK(nexts >= 8, "a brisk spin should still deliver most steps");
    CHECK(g_ctrl_presses == 1, "Control+Option should stay down for the whole spin");
    CHECK(g_ctrl_alone == 0, "Control never alone");
    checkTiming();
  }},
  {"slow_clicks_release_modifiers", [] {
    boot(); g_usb_mounted = true;
    detent(+1); tick(1000); detent(+1); tick(1000);
    CHECK(g_ctrl_presses == 2 && g_down_usb.empty(), "separate clicks get separate holds, nothing left held");
  }},
  {"different_command_after_wheel_releases_first", [] {
    boot(); g_usb_mounted = true;
    detent(+1); tick(5); tap(BACK_PIN, 100, 400);
    expectKeys(0, {NEXT, "Esc"}, "wheel then Back");
    CHECK(g_keys.size() == 2 && g_keys[1].keys.size() == 1, "Esc must not be sent with Control+Option still held");
    checkTiming();
  }},
  {"watchdog_running_and_fed", [] {
    boot(); tick(1000);
    CHECK(rp2040.wdtBegun == 1 && rp2040.wdtMs == 8000, "watchdog should start once at 8 s");
    CHECK(rp2040.feeds > 1000, "loop should feed the watchdog continuously");
  }},
  {"two_buttons_same_instant", [] {
    boot(); press(RED_PIN); press(GRN_PIN); tick(200); release(RED_PIN); release(GRN_PIN); tick(300);
    expectKeys(0, {"Ctrl+Opt+a", "Ctrl+Opt+h"}, "simultaneous presses are two clean chords");
    checkTiming();
  }},
  {"mixed_burst_timing", [] {
    boot(); for (int i = 0; i < 6; i++) { detent(+1, 0); press(SEL_PIN); tick(30); release(SEL_PIN); tick(30); }
    tick(3000); checkTiming();
    printf("  info: burst produced %zu keystrokes\n", g_keys.size());
  }},
#ifdef VARIANT_HOLD
  {"hold_ms_filters_brush", [] {
    boot(); tap(RED_PIN, HOLD_MS / 2, 300);
    CHECK(g_keys.empty(), "brush shorter than HOLD_MS ignored");
    tap(RED_PIN, HOLD_MS + 50, 300);
    expectKeys(0, {"Ctrl+Opt+a"}, "press longer than HOLD_MS counts");
  }},
#endif
#ifdef VARIANT_REPEAT
  {"ignore_repeat", [] {
    boot(); tap(RED_PIN, 80, 80); tap(RED_PIN, 80, 80);       // second press ~160 ms later
    expectKeys(0, {"Ctrl+Opt+a"}, "double hit inside IGNORE_REPEAT_MS is one press");
    tick(IGNORE_REPEAT_MS); tap(RED_PIN, 80, 300);
    expectKeys(0, {"Ctrl+Opt+a", "Ctrl+Opt+a"}, "press after the window counts");
  }},
#endif
};

int main(int argc, char** argv) {
  if (argc < 2) { for (auto& kv : S) printf("%s\n", kv.first.c_str()); return 0; }
  scen = argv[1];
  if (argc > 2) g_eeprom_path = argv[2];
  if (!S.count(scen)) { printf("unknown scenario %s\n", scen.c_str()); return 2; }
  S[scen]();
  printf("%s %s\n", failures ? "FAIL" : "PASS", scen.c_str());
  return failures ? 1 : 0;
}
