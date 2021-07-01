@echo off
mkdir z:\handmade_game\project\build
pushd z:\handmade_game\project\build
cl z:\handmade_game\project\code\main.cpp -Zi
popd
