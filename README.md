# xcursorview 1.0.1

xcursorview is an Xorg daemon that tracks an xinput cursor and draws a crosshair at its position.
Crosshair width and color are customizable.
Useful for debugging, minimal environments, or issues related to cursor rendering.

## Installation

### Dependencies

- xinput
- libX11
- libXfixes
- libXext

### Compilation

```bash
git clone https://github.com/jpcx/xcursorview.git
cd xcursorview
make
sudo make install
```

## Synopsis

```bash
xcursorview [-w width] [-c color] [-Fh] device
```

## Options

- `-w, --width width`: Set the total crosshair width (default: 11).
- `-c, --color color`: Set the crosshair color as a hex value (default: 0x800080).
- `-F, --foreground`: Do not daemonize.
- `-h, --help`: Display help information and exit.

## Arguments

- `device`: The xinput device ID.

## Examples

Draw a crosshair with default width and color for the xinput device ID 42:
```bash
xcursorview 42
```

Draw a crosshair with width 15 and color 0x123456 for the xinput device ID 42:
```bash
xcursorview -w 15 -c 0x123456 42
```

## Author

Written by Justin Collier.

## Copyright

Copyright (C) 2023 Justin Collier <m@jpcx.dev>
License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
