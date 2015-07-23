#!/usr/bin/env python3

APPNAME = 'dwmstatus'
VERSION = '0.0'

top = '.'
out = 'build'

def options(opt):
    opt.load('compiler_c gnu_dirs')
    opt.add_option('--debug', action='store', default=False,
                   help='Turn on debug mode (True/False) [default: False]')

def configure(ctx):
    from waflib.Tools.compiler_c import c_compiler
    c_compiler['linux'] = ['clang', 'gcc', 'icc']
    libs = ['iw', 'udev', 'pthread', 'X11']
    ctx.env.CFLAGS = ['-Wall', '-Wunused']

    if ctx.options.debug == 'True':
        print('=== DEBUG MODE ===')
        ctx.env.CFLAGS += ['-g', '-Wpadded']
    else:
        ctx.env.CFLAGS.append('-Ofast')

    ctx.start_msg('Counting CPUS')
    from multiprocessing import cpu_count
    cpus = cpu_count()
    ctx.env.DEFINES = ['DS_CPU_COUNT=%d' % cpus]
    ctx.end_msg(cpus)

    ctx.start_msg('Finding temperature sensors')
    from glob import glob
    sensors = glob('/sys/class/hwmon/hwmon*/temp*_input')
    if sensors:
        ctx.env.DEFINES += ['DS_TEMP_COUNT=%d' % len(sensors),
                    'DS_SENSORS=%s' % '\"' + '\", \"'.join(sensors) + '\"']
    ctx.end_msg(len(sensors))

    ctx.start_msg('Checking for battery')
    has_battery = len(glob('/sys/class/power_supply/*')) > 1
    if not has_battery:
        ctx.env.DEFINES.append('DS_NO_BATTERY')
    ctx.end_msg(has_battery)

    ctx.start_msg('Checking for wifi')
    import os
    has_wifi = os.path.islink('/sys/class/net/wlan0')
    if not has_wifi:
        ctx.env.DEFINES.append('DS_NO_WIFI')
    ctx.end_msg(has_wifi)

    ctx.load('compiler_c')
    ctx.check_cc(msg='Testing compiler', fragment="int main() { return 0; }\n", execute=True)
    for lib in libs:
        ctx.check_cc(lib=lib, cflags=ctx.env.CFLAGS, uselib_store='L', mandatory=True)

def build(bld):
    bld.program(source='src/dwmstatus.c',
                includes='.',
                target='dwmstatus',
                use='L')
