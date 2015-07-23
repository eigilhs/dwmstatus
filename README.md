# dwmstatus
[![Travis-CI](https://travis-ci.org/skjvlnd/dwmstatus.svg?branch=master)](https://travis-ci.org/skjvlnd/dwmstatus)

Small daemon for updating the [dwm](http://dwm.suckless.org/)
status bar. Very untested.

## Dependencies
Depends on development files for `libiw`, `libudev`, `libx11`,
and on `python >= 2.6` for `waf`.

## Installing
1. Change `config.h` to match your system
2. `$ ./waf configure build`
3. `# ./waf install`

## Running
Put `dwmstatus &` in your `.xinitrc`.
