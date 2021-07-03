#include <windows.h>
#include <stdint.h>
#include <xinput.h>

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

//NOTE: XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub){
  return(0);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

//NOTE: XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub){
  return(0);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void Win32LoadXInput(){
  HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");
  if (XInputLibrary){
    XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
    XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
  }
}

internal win32_window_dimension Win32GetWindowDimension(HWND Window){
  win32_window_dimension Result;
  RECT ClientRect;
  GetClientRect(Window, &ClientRect);
  Result.Width = ClientRect.right - ClientRect.left;
  Result.Height = ClientRect.bottom - ClientRect.top;
  return(Result);
}

internal void RenderWeirdGradient(win32_offscreen_buffer *Buffer, int XOffset, int YOffset){
  //TODO: Let's see what the optimizer does
  uint8_t *Row = (uint8_t *)Buffer->Memory;

  for (int y = 0; y < Buffer->Height; y++){

    uint32_t *Pixel = (uint32_t *)Row;

    for (int x = 0; x < Buffer->Width; x++){
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
    Row += Buffer->Pitch;
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

internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight){
  //TODO: Aspect ratio correction

  StretchDIBits(DeviceContext, /*X, Y, Width, Height, X, Y, Width, Height,*/
                0, 0, WindowWidth, WindowHeight,
                0, 0, Buffer->Width, Buffer->Height,
                Buffer->Memory, &Buffer->Info, DIB_RGB_COLORS, SRCCOPY);
}

internal LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam){

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
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:{
      uint32_t VKCode = WParam;
      bool WasDown = ((LParam & (1 << 30)) != 0);
      bool IsDown = ((LParam & (1 << 31)) == 0);
      if (IsDown != WasDown){
        if (VKCode == 'W'){
        }
        else if (VKCode == 'A'){
        }
        else if (VKCode == 'S'){
        }
        else if (VKCode == 'D'){
        }
        else if (VKCode == 'Q'){
        }
        else if (VKCode == 'E'){
        }
        else if (VKCode == VK_UP){
        }
        else if (VKCode == VK_DOWN){
        }
        else if (VKCode == VK_RIGHT){
        }
        else if (VKCode == VK_LEFT){
        }
        else if (VKCode == VK_ESCAPE){
          OutputDebugStringA("Escape: ");
          if (IsDown){
            OutputDebugStringA("IsDown");
          }
          if (WasDown){
            OutputDebugStringA("WasDown");
          }
          OutputDebugStringA("\n");
        }
        else if (VKCode == VK_SPACE){
        }
      }
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
      Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
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

  Win32LoadXInput();
  WNDCLASSA WindowClass = {};
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

            //TODO: should we pull this more frequently
            for (DWORD ControllerIntex = 0; ControllerIntex < XUSER_MAX_COUNT; ControllerIntex++ ){
              XINPUT_STATE ControllerState;
              if (XInputGetState(ControllerIntex, &ControllerState)){
                //NOTE: This controller is plugged in
                //TODO: See if ControllerState.dwPacketNumber increments too rapidly
                XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
                bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
                bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
                bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
                bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
                bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
                bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

                int16_t StickX = Pad->sThumbLX;
                int16_t StickY = Pad->sThumbLY;

                XOffset += StickX >> 14;
                YOffset += StickY >> 14;
              }
              else {
                //NOTE: The controller is not available
              }
            }

            XINPUT_VIBRATION Vibration;
            DWORD speed = 0;
            Vibration.wLeftMotorSpeed = speed;
            Vibration.wRightMotorSpeed = speed;

            XInputSetState(0, &Vibration);
            RenderWeirdGradient(&GlobalBackBuffer, XOffset, YOffset);

            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
            ReleaseDC(Window, DeviceContext);
            // XOffset += 2;
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
