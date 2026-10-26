# How It Works

The library talks to Sharp's CE drivers and runs both on the plain CE desktop and from the dictionary's app menu.

## Limitations

- Greys and brightness need pwsh2-boot's sub LCD driver; Sharp's is black and white, backlight on or off. See [Display](#display) and [Brightness](#brightness).
- Holding the backlight on assumes the power manager's idle time is over 2 s. See [Backlight](#backlight).
- One beep pitch: only the length varies. See [Buzzer](#buzzer).
- In dictionary mode, the main screen is not repainted while an app runs; an app without a window leaves Sharp's "Please wait" message up. The examples open one with `window.h`. See [Dictionary mode](#dictionary-mode).

## Display

`SLD1:` is `EDNA2_slcd_ST7586.dll`, Sharp's driver for the ST7586 controller. Its `DeviceIoControl` codes are plain numbers:

| Code | Use |
| --- | --- |
| 1 | Panel on |
| 8 | Draw a rectangle from a full-screen 1-bit buffer |
| 12 | Reset the clip window to the whole screen |
| 14 | Contrast 0-100 |

Code 8 takes `x, y, width, height` and a pointer to a 240x120 buffer. The driver's native order puts the byte for pixel (x, y) is at `(x / 8) * 120 + y`, leftmost pixel in the top bit. This library keeps its buffer in that order so it can be passed straight through.

`sublcd_update()` compares the buffer with what was last sent and draws the band of rows that changed. The driver widens every draw to whole 24-pixel columns.

The panel has four grey levels, but code 8 is 1-bit and Sharp's driver puts the controller in monochrome mode (ST7586 command `0x39`).

pwsh2-boot replaces the driver with ce600's `drivers/sublcd-a7300`, which runs the controller in grey mode and adds code `0x101` for a 2-bit buffer. The library keeps a 2-bit buffer, row by row, and asks the driver with code `0x100` which it is; for Sharp's it reduces levels 2 and 3 to black and sends code 8.

## Touch

`TPD2:` is `EDNA2_TouchSub.dll`. A handle gets a sample queue (`0x80002007`), queues samples over Sharp's key zones too (`0x8000200B`, mode 1), and signals an event for each sample (`0x8000200F`). Each sample is a `DWORD`: x and y in sub LCD pixels, shifted left by 2, with the state (press, move, release) in the low 2 bits.

Only the newest handle gets samples, so the last app to open touch owns it.

## Backlight

Sharp's `backlight.dll` keeps the sub backlight state in the named event `Display_Event_Backlight_SubActive` and applies it when `Display_Event_Backlight_SubUpdate` and `Display_Event_Backlight_Update` are set. The library does the same as SharpLib's `EdSetSubBacklightState`.

`edna2_powermgr.dll` turns the sub backlight off after an idle time set by `IOCTL_PWR_SET_SUBLCDBLOFFTIME` (any number of milliseconds, 0 for never). Each update with `SubActive` set restarts that timer, so `sublcd_backlight_keep_on()` turns the backlight on again every 2 s while an app runs. Apps can't read the off time, so this assumes it is longer than 2 s.

### Brightness

The sub backlight is pad `PWM2`, which Sharp's `backlight.dll` drives as a GPIO. pwsh2-boot's copy hands that pad to its SLD1: driver, which dims it with PWM channel 2. `sublcd_set_backlight_level()` sends code `0x102`; the driver saves the level and uses it whenever the backlight comes on. With Sharp's drivers there is no brightness.

## Buzzer

`BZR1:` is `EDNA2_Buzzer.dll`, which clicks for keys and taps. Sounds are presets at a fixed pitch, differing only in length. Its registry events (`Drivers\BuiltIn\Buzzer\Event\<Name>`) and `DeviceIoControl` code `0x8000200F` both queue the same request: mode 1, alarm pattern and time. `sublcd_beep()` sends pattern 1, which plays for the time asked.

## Keyboard input

The keyboard example sends keys with `keybd_event`, which CE treats as hardware key presses, so they go to whichever window has focus. Sub LCD taps never move focus, so it can't type into itself by accident. Its window has a text box, which is the only target in dictionary mode; on the CE desktop, tap another app to type there. Snake needs arrow keys, which go only to the focused window, so its window takes focus.

## Main-screen window

Every interactive example opens a main-screen window (`window.h`) with a Quit button, so quitting is obvious on either screen. In dictionary mode it also covers the dictionary's "Please wait" message.

## Dictionary mode

The same executable is copied to `アプリ\SubLCD <Name>\AppMain.exe` with an empty `index.din`, which is all the dictionary's app menu needs.

The dictionary (`\Nand\wceprj.exe`) turns the sub LCD off, stops its own sub touch handling and blocks until the app exits, then reverses all three. While it waits it does not repaint the main screen, so an app without a window leaves its "Please wait" message up.

SHAM-7X0 once showed a white panel with a small Japanese button over the right third of the sub LCD in dictionary mode. None of the examples have reproduced it, and nothing in `wceprj.exe` draws it. SharpLib's `EdSetSubLcdCeAppCompatibleMode` only sets a flag read by SharpLib's pad editor in the calling process, so it is not a fix.
