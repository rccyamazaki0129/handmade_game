@echo off
set CompilerFlags=-MT -nologo -EHsc -EHa- -GR- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4459 -wd4456 -wd4702 -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32=1 -Z7
set LinkerFlags=-incremental:no -opt:ref User32.lib Gdi32.lib Winmm.lib
IF NOT EXIST z:\handmade_game\project\build mkdir z:\handmade_game\project\build
pushd z:\handmade_game\project\build
del *.pdb > NUL 2> NUL
cl %CompilerFlags% z:\handmade_game\project\code\handmade.cpp -Fmhandmade.map /LD /link -incremental:no /PDB:handmade_%date:~0,4%%date:~5,2%%date:~8,2%%time:~3,2%%time:~6,2%.pdb /EXPORT:GameGetSoundSamples /EXPORT:GameUpdateAndRender
cl %CompilerFlags% z:\handmade_game\project\code\win32_handmade.cpp -Fmwin32_handmade.map /link %LinkerFlags%
popd
