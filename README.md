# Conkylite

Small daemon made for updating the [dwm](http://dwm.suckless.org/)
status bar, inspired by [conky](https://github.com/brndnmtthws/conky)'s
"out to console" mode. Beware of absolute paths to stuff in `/proc`
and `/sys`, assumptions on CPU core count, and use of the `MemAvailable`
stat which is only present on Linux >= 3.14. Probably only works on my
machine.

## Dependencies
Depends on development files for `libiw`, `libudev`, `libx11`,
and on `python >= 2.6` for `waf`.

## Installing
1. `$ ./waf configure`
2. `$ ./waf build`
3. `# ./waf install`

## Running
Put `conkylite &` in your `.xinitrc`.