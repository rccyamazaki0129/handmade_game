#include <windows.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

struct win32_offscreen_buffer{
  //NOTE: pixels are always 32-bit wide, memory order: BBGGRRXX
  BITMAPINFO Info;
  void *Memory;
  int Width;
  int Height;
  int Pitch;
};

struct win32_window_dimension{
  int Width;
  int Height;
};

//TODO: This is a global for now
global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;


win32_window_dimension Win32GetWindowDimension(HWND Window){
  win32_window_dimension Result;

  RECT ClientRect;
  GetClientRect(Window, &ClientRect);
  Result.Width = ClientRect.right - ClientRect.left;
  Result.Height = ClientRect.bottom - ClientRect.top;
  return(Result);
}

internal void RenderWeirdGradient(win32_offscreen_buffer Buffer, int XOffset, int YOffset){
  //TODO: Let's see what the optimizer does
  uint8_t *Row = (uint8_t *)Buffer.Memory;

  for (int y = 0; y < Buffer.Height; y++){

    uint32_t *Pixel = (uint32_t *)Row;

    for (int x = 0; x < Buffer.Width; x++){
      /*
        Pixel in memory: 00 00 00 00
                         BB GG RR XX
        0x XXRRGGBB
      */
      uint8_t Blue = (x + XOffset);
      uint8_t Green = (y + YOffset);
      uint8_t Red = uint8_t((x + y) / 16);

      *Pixel++ = ((Red << 16) | (Green << 8) | Blue);
    }
    Row += Buffer.Pitch;
  }
}

internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height){
  //TODO: Bulletproof this
  // maybe dont free first, free after, then free first if that fails
  if (Buffer->Memory){
    VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
  }

  Buffer->Width = Width;
  Buffer->Height = Height;
  int BytesPerPixel = 4;

  //NOTE: When the biHeight field is negative, this is the clue to
  //      windows to treat this bitmap as top-down, not bottom-up
  Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
  Buffer->Info.bmiHeader.biWidth = Buffer->Width;
  Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
  Buffer->Info.bmiHeader.biPlanes = 1;
  Buffer->Info.bmiHeader.biBitCount = 32;
  Buffer->Info.bmiHeader.biCompression = BI_RGB;
  Buffer->Info.bmiHeader.biSizeImage = 0;
  Buffer->Info.bmiHeader.biXPelsPerMeter = 0;
  Buffer->Info.bmiHeader.biYPelsPerMeter = 0;
  Buffer->Info.bmiHeader.biClrUsed = 0;
  Buffer->Info.bmiHeader.biClrImportant = 0;

  int BitmapMemorySize = BytesPerPixel * Buffer->Width * Buffer->Height;
  Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

  //TODO: Probably clear to this black

  Buffer->Pitch = Buffer->Width * BytesPerPixel;
}

internal void Win32DisplayBufferInWindow(HDC DeviceContext, int WindowWidth, int WindowHeight,
                                         win32_offscreen_buffer Buffer){
  //TODO: Aspect ratio correction

  StretchDIBits(DeviceContext, /*X, Y, Width, Height, X, Y, Width, Height,*/
                0, 0, WindowWidth, WindowHeight,
                0, 0, Buffer.Width, Buffer.Height,
                Buffer.Memory, &Buffer.Info, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam){

  LRESULT Result = 0;

  switch(Message){
    case WM_SIZE:{
      break;
    }
    case WM_DESTROY:{
      //TODO: Handle this as an error - recreate window?
      GlobalRunning = false;
      break;
    }
    case WM_CLOSE:{
      //TODO: Handle this with a message to the user?
      GlobalRunning = false;
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
      win32_window_dimension Dimension = Win32GetWindowDimension(Window);
      Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackBuffer);
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
  Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

  //TODO: Check if CS_OWNDC, CS_HREDRAW, CS_VREDRAW still matter
  WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
  WindowClass.lpfnWndProc = Win32MainWindowCallback;
  WindowClass.hInstance = Instance;
  // WindowClass.hIcon;
  // LPCSTR    lpszMenuName;
  WindowClass.lpszClassName = "HandmadeGameWindowClass";
  if (RegisterClass(&WindowClass)){
    HWND Window = CreateWindowEx(
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
      if (Window) {
          //succeeded creating window
          //NOTE: Since we specified CS_OWNDC, we can just get one DC and
          //      use it forever because we are not sharing it with anyone
          HDC DeviceContext = GetDC(Window);

          int XOffset = 0;
          int YOffset = 0;
          GlobalRunning = true;

          while (GlobalRunning){

            MSG Message;

            while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)){
              if (Message.message == WM_QUIT){
                GlobalRunning = false;
              }
              TranslateMessage(&Message);
              DispatchMessage(&Message);
            }
            RenderWeirdGradient(GlobalBackBuffer, XOffset, YOffset);


            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackBuffer);
            ReleaseDC(Window, DeviceContext);

            XOffset++;
            YOffset++;
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
