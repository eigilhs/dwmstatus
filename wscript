#! /usr/bin/env python3

APPNAME = 'conkylite'
VERSION = '0.0'

top = '.'
out = 'build'

def options(opt):
    opt.load('compiler_c gnu_dirs')


def configure(ctx):
    from waflib.Tools.compiler_c import c_compiler
    c_compiler['linux'] = ['clang', 'gcc', 'icc']
    libs = ['iw', 'udev', 'pthread']
    ctx.env.CFLAGS = ['-Wall', '-Wunused', '-Ofast']
    ctx.load('compiler_c')
    ctx.check_cc(msg='Testing compiler', fragment="int main() { return 0; }\n", execute=True)
    for lib in libs:
        ctx.check_cc(lib=lib, cflags=ctx.env.CFLAGS, uselib_store='L', mandatory=True)

def build(ctx):
    ctx.program(source='src/conkylite.c',
                includes='.',
                target='conkylite',
                use='L')

