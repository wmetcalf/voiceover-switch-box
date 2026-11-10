#pragma once
struct MockMutex {};
struct MockUSB { MockMutex mutex; };
extern MockUSB USB;
struct CoreMutex {
  CoreMutex(MockMutex*, bool = true) {}
  explicit operator bool() const { return true; }
};
