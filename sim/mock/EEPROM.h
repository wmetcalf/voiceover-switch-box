#pragma once
#include "Arduino.h"
extern const char* g_eeprom_path;
struct MockEEPROM {
  uint8_t d[256];
  void begin(int) {
    memset(d, 0xFF, sizeof d);
    if (FILE* f = fopen(g_eeprom_path, "rb")) { size_t n = fread(d, 1, sizeof d, f); (void)n; fclose(f); }
  }
  uint8_t read(int a) { return d[a]; }
  void write(int a, uint8_t v) { d[a] = v; }
  bool commit() { FILE* f = fopen(g_eeprom_path, "wb"); fwrite(d, 1, sizeof d, f); fclose(f); return true; }
};
extern MockEEPROM EEPROM;
