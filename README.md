# Conkylite

Small daemon for updating the [dwm](http://dwm.suckless.org/)
status bar, inspired by [conky](https://github.com/brndnmtthws/conky)'s
"out to console" mode. Very untested.

## Dependencies
Depends on development files for `libiw`, `libudev`, `libx11`,
on `python >= 2.6` for `waf`.

## Installing
1. Change `config.h` to match your system
2. `$ ./waf configure`
3. `$ ./waf build`
4. `# ./waf install`

## Running
Put `conkylite &` in your `.xinitrc`.
