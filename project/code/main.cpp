#include <windows.h>

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd){
  MessageBoxA(0, "This is your very first message box!", "Your Window", MB_YESNO|MB_ICONINFORMATION);
  return(0);
}
