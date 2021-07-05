@echo off
IF NOT EXIST z:\handmade_game\project\build mkdir z:\handmade_game\project\build
pushd z:\handmade_game\project\build
cl -Zi z:\handmade_game\project\code\win32_handmade.cpp User32.lib Gdi32.lib
popd
