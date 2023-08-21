@echo off

if not exist ..\build mkdir ..\build

set CFLAGS=/nologo /Od /Zi /EHsc
set LIBS=User32.lib Xinput.lib Xaudio2.lib d3d11.lib d3dcompiler.lib
set INC_DIR=
set LNK_DIR=

pushd ..\build

    cl %CFLAGS% %INC_DIR% ..\src\main.cpp /Fe.\game /link %LNK_DIR% %LIBS% /SUBSYSTEM:CONSOLE

popd
