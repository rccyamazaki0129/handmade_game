#include <windows.h>

LRESULT CALLBACK MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam){

  LRESULT Result = 0;

  switch(Message){
    case WM_SIZE:{
      OutputDebugStringA("WM_SIZE\n");
      break;
    }
    case WM_DESTROY:{
      OutputDebugStringA("WM_DESTROY\n");
      break;
    }
    case WM_CLOSE:{
      OutputDebugStringA("WM_CLOSE\n");
      break;
    }
    case WM_ACTIVATEAPP:{
      OutputDebugStringA("WM_ACTIVATEAPP\n");
      break;
    }
    case WM_PAINT:{
      PAINTSTRUCT Paint;
      HDC DeviceContext = BeginPaint(Window, &Paint);
      int X = Paint.rcPaint.left;
      int Y = Paint.rcPaint.top;
      int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
      int Width = Paint.rcPaint.right - Paint.rcPaint.left;
      PatBlt(DeviceContext, X, Y, Width, Height, WHITENESS);
      EndPaint(Window, &Paint);

      break;
    }
    default:
    {
      // OutputDebugStringA("defaut\n");
      Result = DefWindowProc(Window, Message, WParam, LParam);
      break;
    }
  }
  return(Result);
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode){
  WNDCLASS WindowClass = {};
  //TODO: Check if CS_OWNDC, CS_HREDRAW, CS_VREDRAW still matter
  WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
  WindowClass.lpfnWndProc = MainWindowCallback;
  WindowClass.hInstance = Instance;
  // WindowClass.hIcon;
  // LPCSTR    lpszMenuName;
  WindowClass.lpszClassName = "HandmadeGameWindowClass";
  if (RegisterClass(&WindowClass)){
    HWND WindowHandle = CreateWindowEx(
      0,
      WindowClass.lpszClassName,
      "HandmadeGameWindowClass",
      WS_OVERLAPPEDWINDOW|WS_VISIBLE,
      CW_USEDEFAULT,//x
      CW_USEDEFAULT,//y
      CW_USEDEFAULT,//width
      CW_USEDEFAULT,//height
      0,
      0,
      Instance,
      0);
      if (WindowHandle) {
          //succeeded creating window
          for (;;){
            MSG Message;
            BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
            if (MessageResult > 0){
              TranslateMessage(&Message);
              DispatchMessage(&Message);
            }
            else {
              break;
            }
          }

      }
      else {
        //TODO: Logging
      }
  }
  else {
    //TODO: Logging
  }

  return(0);
}
