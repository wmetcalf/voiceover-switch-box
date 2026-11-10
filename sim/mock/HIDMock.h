#pragma once
#include "Arduino.h"
#define KEY_LEFT_CTRL 0x80
#define KEY_LEFT_SHIFT 0x81
#define KEY_LEFT_ALT 0x82
#define KEY_LEFT_GUI 0x83
#define KEY_RETURN 0xB0
#define KEY_ESC 0xB1
#define KEY_TAB 0xB3
#define KEY_F5 0xC6
#define KEY_RIGHT_ARROW 0xD7
#define KEY_LEFT_ARROW 0xD8
#define KEY_DOWN_ARROW 0xD9
#define KEY_UP_ARROW 0xDA

extern std::vector<uint8_t> g_down_usb, g_down_ble;
extern uint32_t g_down_t_usb, g_down_t_ble;
extern int g_violations;
extern uint32_t g_last_report;

extern int g_ctrl_presses, g_ctrl_alone;
inline bool isMod(uint8_t k) { return k >= 0x80 && k <= 0x87; }

struct MockKbd {
  bool usb; bool begun = false;
  explicit MockKbd(bool u) : usb(u) {}
  std::vector<uint8_t>& down() { return usb ? g_down_usb : g_down_ble; }
  std::vector<uint8_t>& other() { return usb ? g_down_ble : g_down_usb; }
  void begin(const char* = nullptr, const char* = nullptr) { begun = true; }
  bool recorded = false;   // at least one key of this modifier hold has been logged
  size_t groupStart = 0;   // first g_keys index logged during this modifier hold
  uint32_t tMain = 0;      // when the last (main) key went down
  size_t press(uint8_t k) {
    if (!other().empty()) g_violations++;            // keys held on both transports at once
    if (down().empty()) { (usb ? g_down_t_usb : g_down_t_ble) = g_now; recorded = false; groupStart = g_keys.size(); }
    if (k == 0x80) {                                  // Left Control
      g_ctrl_presses++;
      bool optDown = false; for (auto d : down()) if (d == 0x82) optDown = true;
      if (!optDown) g_ctrl_alone++;                   // iPad would see a bare Control press
    }
    if (!isMod(k)) tMain = g_now;
    if (usb) g_last_report = g_now;
    down().push_back(k); return 1;
  }
  size_t release(uint8_t k) {                         // main key up, modifiers stay down
    if (usb) g_last_report = g_now;
    if (!isMod(k)) {
      g_keys.push_back({usb ? g_down_t_usb : g_down_t_ble, g_now, usb, down(), tMain, 0});
      recorded = true;
    }
    for (size_t i = 0; i < down().size(); i++) if (down()[i] == k) { down().erase(down().begin() + i); break; }
    return 1;
  }
  void releaseAll() {
    if (down().empty()) return;
    if (usb) g_last_report = g_now;
    if (!recorded) g_keys.push_back({usb ? g_down_t_usb : g_down_t_ble, g_now, usb, down(), tMain, g_now});
    else for (size_t i = groupStart; i < g_keys.size(); i++) g_keys[i].tAllUp = g_now;
    down().clear(); recorded = false;
  }
  size_t write(uint8_t k) { press(k); releaseAll(); return 1; }
  uint32_t tCons = 0;
  size_t consumerPress(uint16_t) { tCons = g_now; if (usb) g_last_report = g_now; return 1; }
  size_t consumerRelease() { if (usb) g_last_report = g_now;
    g_keys.push_back({tCons, g_now, usb, std::vector<uint8_t>{0xFE}, tCons, g_now}); return 1; }
};
