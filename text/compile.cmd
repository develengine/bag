
cl /nologo /EHsc /Feprogram ../engine/win32/bag_win32.c test.c bag_text.c ../engine/glad/src/gl.c /I. /I../engine /I../engine/glad/include User32.lib Gdi32.lib Opengl32.lib

@echo off
