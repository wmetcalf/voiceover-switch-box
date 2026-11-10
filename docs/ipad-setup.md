# iPad setup checklist

Do this once on the student's iPad. Each item says why it matters.

## Connecting

- [ ] **Use a USB-C cable that carries data.** Many USB-C cables are charge-only: the Red button lights, but the iPad never sees a keyboard.
- [ ] **Plug it into the box the right way up.** The box's USB-C port only passes power one way round. The Red button lights when it's right; if it stays dark, flip the plug at the box. Then leave it plugged into the box and unplug only at the iPad.
- [ ] **Allow the accessory.** On iPadOS 26, open Settings → Privacy & Security → **Wired Accessories** and choose **Always Allow** or **Automatically Allow When Unlocked**. If the iPad was locked when the box was plugged in, unplug it, unlock the iPad, and plug it in again.
- [ ] **Keyboard layout: U.S.** Set this in Settings → General → Keyboard → Hardware Keyboard. Other layouts move the letter keys the box relies on.
- [ ] **Bluetooth (optional).** In Settings → Bluetooth, pair **VoiceOver Device**. The box uses Bluetooth whenever USB isn't available.

## VoiceOver

- [ ] **VoiceOver on.** Settings → Accessibility → VoiceOver.
- [ ] **Modifier Keys includes Control + Option.** Settings → Accessibility → VoiceOver → Typing → Modifier Keys. Every VoiceOver command from the box uses Control + Option.
- [ ] **Quick Nav: nothing to do.** The box's commands work whether it is on or off. To change it anyway, hold **Back + Green** for 3 seconds; VoiceOver says "Quick Nav on" or "off".
- [ ] **Full Keyboard Access off.** Settings → Accessibility → Keyboards & Typing → Full Keyboard Access. It competes with VoiceOver for keyboard control.

## Stop things that surprise the student

- [ ] **Picture in Picture: turn off Start PiP Automatically.** Settings → General → Picture in Picture. Otherwise, leaving Netflix or Apple TV during a video keeps it playing in a floating window.
- [ ] **Dictation shortcut off.** If iPadOS offers a "press Control twice" Dictation shortcut (under Settings → General → Keyboard), turn it off. Current firmware avoids triggering it, but there's no reason to leave it on.
- [ ] **Keep the student's apps on the first Home Screen page or in the dock.** Changing pages needs a gesture the box doesn't have a button for. Pressing Green (Home) again on the Home Screen goes back to the first page.
- [ ] **Focus / Do Not Disturb during games and videos,** so notifications don't interrupt.

## TD Snap

- [ ] Hold **Back + Chooser** for 3 seconds to put the box in TD Snap mode. The piezo (if fitted) plays rising tones, then two beeps.
- [ ] In TD Snap, open Edit, then User → Access Method → **Scanning**, then **2 Switch Step Scan**, with **Space = next** and **Enter = select**.
- [ ] Turn on TD Snap's audio feedback, with a different voice from the speaking voice.
- [ ] Don't use 1-Switch Auto Scan with Enter.
- [ ] Hold **Back + Chooser** for 3 seconds again to return to VoiceOver mode.

## When something seems broken

| Symptom | Likely cause |
|---|---|
| Wheel does nothing with VoiceOver on, only Chooser seems to work | The box is in TD Snap mode: unplug and replug it, and it beeps twice. Hold Back + Chooser for 3 s. |
| Red button dark when plugged in | No power: flip the plug at the box end, or try another cable. |
| Red button lit, but nothing happens on the iPad | Charge-only cable, Wired Accessories blocking it, or the iPad was locked when plugged in. |
| Not sure what the iPad is receiving | Open the [Box Key Check page](https://wmetcalf.github.io/voiceover-switch-box/tools/box-key-check.html) on the iPad with VoiceOver off and press the buttons. It lists every key received and which button it came from. |
| Video keeps playing after leaving the app | Turn off Start PiP Automatically (above). |
| VoiceOver gets lost inside an app | VoiceOver struggles with that app. Back closes pop-ups; Green goes Home. |
