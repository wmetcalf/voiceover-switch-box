# Firmware simulator

Compiles the real sketch on a desktop (g++, C++17) against small mocks of the Arduino core, TinyUSB, the keyboard libraries, Bluetooth and EEPROM. Time is virtual, so a 3-second button hold takes microseconds.

```sh
./sim/run.sh firmware/voiceover_device_ble/voiceover_device_ble.ino
```

`run.sh` builds three variants: the default, one with a 150 ms minimum hold (`HOLD_MS`), and one with a 400 ms ignore-repeat window (`IGNORE_REPEAT_MS`). It then runs every scenario in `test_fw.cpp` against each one.

To add a scenario, add an entry to the `S` map (the scenario table) in `test_fw.cpp`:
- **Inputs:** `press()`, `release()`, `tap()`, `detent()`, `comboHold()`, or `Serial.in` for the test port.
- **Time:** `tick(ms)` advances virtual time.
- **Checks:** `expectKeys()`, `chords()`, `CHECK()` and `checkTiming()`.

The mocks record every keystroke with its press and release times, so tests can check what keys were sent and how they were held. Examples: modifiers going down before the key, Control never pressed without Option, and keys never sent over USB and Bluetooth at once.
