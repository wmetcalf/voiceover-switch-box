# Changelog

Firmware versions. The sketch header has the same list.

## v10.11
- Clean-up with no change in behaviour: removed the four unused spare inputs on GP2–GP5 and the actions no button used any more.

## v10.10
- Chooser's close-app sends media **Pause** first, then Cmd+W, then Home, so a playing video isn't pushed into Picture in Picture.

## v10.9
- Chooser (VoiceOver mode) closes the current app (replaces v10.7's page turning).

## v10.8
- Back + Green held 3 s toggles VoiceOver Quick Nav.

## v10.7
- Chooser (VoiceOver mode) turned Home Screen pages (Option + arrows).

## v10.6
- Spinning the wheel fast no longer opens Dictation on the iPad. Option goes down before Control, and the modifiers stay held through a spin.

## v10.5
- Key combinations are typed like a person types them: modifiers 40 ms before the key, the key held 60 ms, modifiers released last. Timed from when the host actually reads each report. This made iPad VoiceOver accept the box's commands; the iPad had been seeing 1–3 ms key presses.

## v10.4
- Mode switch needs Back + Chooser held together 3 s; holding one button alone never switches.
- Back and Chooser act on release.
- USB serial test port (status and simulated button presses).

## v10.3
- USB can't freeze the firmware: if a host isn't reading the keyboard, keys go over Bluetooth after 50 ms.
- A sleeping host gets a USB wake-up request.
- 8 s watchdog.

## v10.2
- Wheel steps no longer pile up on fast spins; turning back cancels pending steps.

## v10.1
- Blue mutes and unmutes VoiceOver's speech instead of turning VoiceOver off.

## v10
- VoiceOver commands sent with Control+Option, so Quick Nav state doesn't matter.
- USB or Bluetooth, never both at once.
- Interrupt-driven wheel, one step per click.
- Debounce, plus optional hold time and ignore-repeat for tremor.
- Optional piezo feedback.
- Red no longer turns VoiceOver off (a blind user couldn't turn it back on).
- Optional battery monitoring with low-battery beeps (PCB with a LiPo only).
