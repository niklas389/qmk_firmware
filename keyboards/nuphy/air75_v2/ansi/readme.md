# NuPhy Air75 V2

*NuPhy Air75 V2 is a standard 84 key keyboard.*
![NuPhy Air75 V2](https://i.imgur.com/R7jS2JC.jpeg)

* Keyboard Maintainer: [nuphy](https://github.com/nuphy-src), firmware customizations by [adi4086](https://github.com/adi4086)
* Hardware Supported: NuPhy Air75 V2 PCB
* Hardware Availability: Private

Make example for this keyboard (after setting up your build environment):

    make nuphy/air75_v2/ansi:default

Flashing example for this keyboard:

    make nuphy/air75_v2/ansi:default:flash

The `default` keymap has VIA enabled. The GitHub Actions workflow `.github/workflows/build_air75_v2.yml` builds the same target and attaches `nuphy_air75_v2_ansi_default.bin` to each run.

See the [build environment setup](https://docs.qmk.fm/#/getting_started_build_tools) and the [make instructions](https://docs.qmk.fm/#/getting_started_make_guide) for more information. Brand new to QMK? Start with our [Complete Newbs Guide](https://docs.qmk.fm/#/newbs).

## VIA

Load [keymaps/nuphy-air75-v2-via_custom_v4.json](keymaps/nuphy-air75-v2-via_custom_v4.json) in VIA (Settings → Show Design tab, then Design → Load Draft Definition). Its effect list follows the firmware's effect order, so it has to be updated whenever RGB Matrix effects are added or removed in `keyboard.json`.

## Bootloader

Enter the bootloader in 3 ways:

* **Bootmagic reset**: Set the mode switch to Cable (with no cable plugged in the keyboard is off), hold down Escape and plug in the keyboard
* **Keycode**: Hold Fn + M + Esc (`QK_REBOOT`) and keep Esc held while the keyboard restarts
* **Hardware reset**: Remove the capslock keycap, hold the little button beneath and plug in the keyboard.

Flashing new firmware resets the stored settings (keymap, lighting, sleep and debounce settings) whenever QMK's EEPROM layout changes, so export the VIA layout first.

## Customizations and Fixes

* [customizations.md](customizations.md)
