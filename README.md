# PW-A7300 Sub LCD

A small C library and examples for the Sharp Brain PW-A7300's 240x120 sub LCD under Windows CE: drawing in four greys, text, touch, the backlight and the buzzer.

## Limitations

- Greys, backlight brightness and pitched tones need pwsh2-boot's sub LCD driver. With Sharp's: black and white, backlight on or off, one beep pitch.
- In dictionary mode, the main screen is not repainted while an app runs.

[How it works](docs/how-it-works.md) explains each.

## Examples

| Example | What it does |
| --- | --- |
| Hello | A greeting |
| Drawing | Lines, rectangles, circles and text |
| Backlight | Toggles the sub backlight, `on` / `off`, or a brightness 1-100 |
| Keyboard | Touch keyboard that types into the focused app |
| Snake | Arrow keys to steer, with beeps |
| Greys | The four grey levels, with - and + for brightness |

All but Backlight have a Quit button on both screens and keep the sub backlight on while running.

## Install

Copy the build's `card` folder to the root of the microSD card:

- `Apps\SubLCD\*.exe` for the CE desktop.
- `アプリ\SubLCD *\` for the dictionary's app menu.

CI builds the `card` folder on every push.

## Use

```c
#include "draw.h"

sublcd_open();
sublcd_clear();
sublcd_text(4, 4, "Hi", 2, SUBLCD_BLACK);
sublcd_update();
sublcd_close();
```

`touch.h`, `button.h`, `backlight.h`, `buzzer.h` and `window.h` cover the rest.

## Build

In a pinned container, as CI does:

```sh
sh build.sh out
```

Or with a local [jornada-72x-tools](https://github.com/Gadgetoid/jornada-72x-tools):

```sh
cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=$JORNADA/cmake/jornada-ce.cmake -DJORNADA_TOOLCHAIN=$JORNADA
cmake --build build
```
