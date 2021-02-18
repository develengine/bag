
cl /nologo /EHsc /Feprogram win32/bag_win32.c tests/test2.c glad/src/gl.c /I. /Iglad/include User32.lib Gdi32.lib Opengl32.lib

@echo off
