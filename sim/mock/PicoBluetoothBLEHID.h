#pragma once
extern bool g_ble_connected;
struct MockBLEHID { bool connected() { return g_ble_connected; } };
extern MockBLEHID PicoBluetoothBLEHID;
