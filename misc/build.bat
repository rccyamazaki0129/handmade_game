@echo off
mkdir z:\handmade_game\project\build
pushd z:\handmade_game\project\build
cl -Zi z:\handmade_game\project\code\main.cpp User32.lib
popd
