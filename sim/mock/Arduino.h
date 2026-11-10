// Minimal Arduino/arduino-pico mock for host-side simulation of the firmware.
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
#include <set>

#define HIGH 1
#define LOW 0
#define INPUT_PULLUP 2
#define OUTPUT 1
#define CHANGE 3
#define LED_BUILTIN 99

struct ToneEvt { uint32_t t; uint16_t f, dur; };
struct KeyEvt2 { uint32_t tDown, tUp; bool usb; std::vector<uint8_t> keys; uint32_t tMain = 0, tAllUp = 0; };

extern uint32_t g_now;
extern int g_pin[128];
extern void (*g_isr[128])();
extern bool g_usb_mounted;
extern std::vector<ToneEvt> g_tones;
extern std::vector<KeyEvt2> g_keys;
extern int g_led;
extern int g_adc;

inline uint32_t millis() { return g_now; }
inline void delay(uint32_t ms) { g_now += ms; }
inline void pinMode(int, int) {}
inline int digitalRead(int p) { return g_pin[p]; }
inline void digitalWrite(int p, int v) { if (p == LED_BUILTIN) g_led = v; }
inline int digitalPinToInterrupt(int p) { return p; }
inline void attachInterrupt(int p, void (*fn)(), int) { g_isr[p] = fn; }
inline void noInterrupts() {}
inline void interrupts() {}
inline void tone(int, unsigned f, unsigned long dur) { g_tones.push_back({g_now, (uint16_t)f, (uint16_t)dur}); }
inline int analogRead(int) { return g_adc; }
inline void analogReadResolution(int) {}
struct MockRP2040 { int wdtBegun = 0; uint32_t wdtMs = 0; uint32_t feeds = 0;
  void wdt_begin(uint32_t ms) { wdtBegun++; wdtMs = ms; } void wdt_reset() { feeds++; } };
extern MockRP2040 rp2040;

// USB serial (test port) mock: tests push input, read back printed output.
struct MockSerial {
  std::string in, out;
  int available() { return (int)in.size(); }
  int read() { if (in.empty()) return -1; int c = (unsigned char)in[0]; in.erase(0, 1); return c; }
  void print(const char* s) { out += s; }
  void println(const char* s = "") { out += s; out += "\n"; }
};
extern MockSerial Serial;
