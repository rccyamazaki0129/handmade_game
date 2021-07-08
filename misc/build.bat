@echo off
IF NOT EXIST z:\handmade_game\project\build mkdir z:\handmade_game\project\build
pushd z:\handmade_game\project\build
cl -MT -nologo -EHsc -EHa- -GR- -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -Z7 -Fmwin32_handmade.map z:\handmade_game\project\code\win32_handmade.cpp /link -opt:ref User32.lib Gdi32.lib
popd
