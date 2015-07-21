#!/usr/bin/env python3

APPNAME = 'conkylite'
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
        ctx.env.CFLAGS.append('-g')
    else:
        ctx.env.CFLAGS.append('-Ofast')
    ctx.start_msg('Counting CPUS')
    from multiprocessing import cpu_count
    cpus = cpu_count()
    ctx.env.DEFINES = ['CL_CPU_COUNT=%d' % cpus]
    ctx.end_msg(cpus)
    ctx.load('compiler_c')
    ctx.check_cc(msg='Testing compiler', fragment="int main() { return 0; }\n", execute=True)
    for lib in libs:
        ctx.check_cc(lib=lib, cflags=ctx.env.CFLAGS, uselib_store='L', mandatory=True)

def build(bld):
    bld.program(source='src/conkylite.c',
                includes='.',
                target='conkylite',
                use='L')
