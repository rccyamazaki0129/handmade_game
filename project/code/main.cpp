#include <windows.h>

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd){
  MessageBoxA(0, "This is your second message box!", "Your Window vol.2", MB_OK|MB_ICONEXCLAMATION);
  return(0);
}
