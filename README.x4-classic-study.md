# Running this fork on the Xteink X4 Classic

This is a quick-start for **this branch** (`feature/study-timer-pomodoro`, adding the
Study tab with countdown Timer and Pomodoro) built and flashed onto the
**Xteink X4 Classic (X4C)** — the button-only ESP32-S3 model with no touchscreen and
no frontlight. See [`freeink-sdk/docs/xteink-x4c-support.md`](freeink-sdk/docs/xteink-x4c-support.md)
for the full hardware reference; this doc only covers getting *this* build onto *that*
device.

> The main [README.md](README.md) covers CrossPoint generally. Use this file when you
> specifically want to build from source and flash the X4C — the web installer at
> crosspointreader.com only ever serves official upstream releases, not this branch.

## 1. Confirm you have the right device

The X4C shares its board and 800x480 panel with the X4 Pro, but is a different build
target — flashing the wrong environment will boot but the touch/frontlight-dependent
code paths won't apply, and the button mapping will be wrong. The X4C is:

- No touchscreen, no frontlight
- 8 buttons total: 2 side keys (page turn) + 4 bottom keys + power
- ESP32-S3, 16MB flash, 8MB PSRAM
- Native SDMMC SD card, USB-MSC capable

If your unit has a touchscreen, it's an X4 Pro, not an X4 Classic — use the `x4pro`
environment instead everywhere below.

## 2. Prerequisites

- [pioarduino PlatformIO Core](https://github.com/pioarduino/platformio-core) (or
  [VS Code + pioarduino IDE](https://github.com/pioarduino/pioarduino-vscode-ide))
- Python 3.8+
- `clang-format` 21 (only needed if you plan to edit code further)
- A USB-C cable that supports **data**, not just charging

## 3. Get the code (already done in this checkout)

This working copy is already cloned with submodules and on the right branch. For
reference, this is how it was set up:

```bash
git clone --recursive https://github.com/Jair0o/crosspoint-reader-study
cd crosspoint-reader-study
git checkout feature/study-timer-pomodoro

# if cloned without --recursive:
git submodule update --init --recursive
```

## 4. Build for the X4 Classic

The X4C has its own PlatformIO environment, `x4c`:

```bash
pio run -e x4c
```

This produces `.pio/build/x4c/firmware.bin`.

## 5. Flash it

### Option A — direct upload over USB-C (simplest)

With the X4C connected via USB-C and awake/unlocked:

```bash
pio run -e x4c -t upload
```

PlatformIO will find the port automatically in most cases. If it can't:

```bash
pio run -e x4c -t upload --upload-port /dev/ttyACM0
```

(Find the port with `dmesg` right after plugging in on Linux.)

### Option B — web installer with a custom .bin

If you'd rather use the browser flasher:

1. Go to https://crosspointreader.com/#flash-tools
2. Select the X4 Classic device
3. Choose "Custom .bin" and upload `.pio/build/x4c/firmware.bin`

### Option C — esptool by hand

```bash
pip install esptool
esptool.py --chip esp32s3 --port /dev/ttyACM0 --baud 921600 write_flash 0x10000 .pio/build/x4c/firmware.bin
```

### If the device doesn't show up when flashing

Some Xteink units bought from third-party resellers (e.g. AliExpress) ship
USB-locked. Try a different USB port/cable and the web flasher first; only reach for
the **Xteink Unlocker** (https://crosspointreader.com/#unlock-tool) if the device still
never appears as a serial port. Units bought directly from xteink.com are not locked.

> ⚠️ Only CrossPoint and CrossInk are officially supported by the unlock tool. Flashing
> anything else on a locked device can permanently brick it.

## 6. Verify it's running

- The device should boot straight to the CrossPoint home screen.
- Serial log level for the `x4c` env is set to debug (`LOG_LEVEL=2`), so you can watch
  boot output live:

```bash
python3 -m pip install pyserial colorama matplotlib
python3 scripts/debugging_monitor.py
```

(macOS: `python3 scripts/debugging_monitor.py /dev/cu.usbmodem2101`.)

## 7. Button layout on the X4C

Since there's no touchscreen, everything is button-driven:

- **Side keys:** page back / page forward
- **4 bottom keys:** Left / Right / Confirm / Back (remappable in Settings)
- **Power key:** short-press to sleep/wake

The Study tab (Timer/Pomodoro) added in this branch is navigated the same way as the
rest of the UI — no touch-specific code paths apply on this device.

## 8. Re-flashing after future code changes

Any time you pull in changes or edit code further:

```bash
pio run -e x4c -t upload
```

If a change touched cache/section file formats, clear the on-device cache once from
the SD card:

```bash
rm -rf /path/to/sd/.crosspoint/
```

## Reference

- [freeink-sdk/docs/xteink-x4c-support.md](freeink-sdk/docs/xteink-x4c-support.md) — full X4C hardware reference (pinout, sensors, partitions)
- [USER_GUIDE.md](USER_GUIDE.md) — using CrossPoint day-to-day
- [docs/contributing/README.md](docs/contributing/README.md) — if you want to keep developing
