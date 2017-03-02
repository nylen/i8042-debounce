## i8042 keyboard debounce

I wrote this simple kernel module as a software fix for the keyboard on my Dell
XPS 15 (9550) because it generates duplicate key presses sometimes, which is
pretty annoying.  It's not a perfect fix but it helps.

This module uses the filter feature of the `i8042` serio driver.  Much like
Highlander, there can be only one of these filters at a time.  There are a few
other platform-specific drivers that might use this filter:

- `drivers/input/misc/ideapad_slidebar.c`
- `drivers/platform/x86/asus-nb-wmi.c`
- `drivers/platform/x86/dell-laptop.c`
- `drivers/platform/x86/hp_accel.c`
- `drivers/platform/x86/msi-laptop.c`
- `drivers/platform/x86/toshiba_acpi.c`

Fortunately none of these are loaded on my machine.

## Credits

Basic structure and `Makefile` adapted from
[`bbswitch`](https://github.com/Bumblebee-Project/bbswitch).

`i8042` filter code and extended key handling adapted from the aforementioned
`drivers/platform/x86/dell-laptop.c`.

Everything else: copyright (c) 2017 James Nylen.

## License

[GNU General Public License v2 (or later)](./LICENSE.md).
