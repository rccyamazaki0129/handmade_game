/*
  TODO: This is not a final platform layer!!

  - Saved game locations
  - Getting a handle to our own executable file
  - Asset loading path
  - Threading (launcha thread)
  - Raw input (support for multiple keyboards)
  - Sleep/time Begin period
  - ClipCursor() (for multimonitor support)
  - Fullscreen support
  - WM_SETCURSOR (control cursor visibility)
  - QueryCancelAutoplay
  - Blit speed improvements (BitBlt)
  - Hardware acceleration (openGL or Direct3D or BOTH?)
  - GetKeyboardLayout (for french keyboards, international WASD support)
  - etc...
*/
#include <math.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static
#define Pi32 3.14159265359f
typedef float real32;
typedef double real64;

#include "handmade.cpp"

#include <windows.h>
#include <stdio.h>
#include <xinput.h>
#include <dsound.h>
#include <malloc.h>

#include "win32_handmade.h"

//TODO: This is a global for now
global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable int64_t GlobalPerfCountFrequency;


//NOTE: XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub){
  return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

//NOTE: XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub){
  return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal debug_read_file_result DEBUGPlatformReadEntireFile(char *FileName){

  debug_read_file_result Result = {};

  HANDLE FileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
  if (FileHandle != INVALID_HANDLE_VALUE){
    LARGE_INTEGER FileSize;
    if (GetFileSizeEx(FileHandle, &FileSize)){
      uint32_t FileSize32 = SafeTruncateSizeUInt64(FileSize.QuadPart);
      Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
      if (Result.Contents){
        DWORD BytesRead;
        if (ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) && (FileSize32 == BytesRead)){
          //NOTE: File read successfully
          Result.ContentsSize = FileSize32;
        }
        else {
          //TODO: Logging
          DEBUGPlatformFreeFileMemory(Result.Contents);
          Result.Contents = 0;
        }
      }
      else {
        //TODO: Logging
      }
    }
    else {
      //TODO: Logging
    }

    CloseHandle(FileHandle);
  }
  else {
    //TODO: Logging
  }

  return Result;

}
internal void DEBUGPlatformFreeFileMemory(void *Memory){
  if (Memory){
    VirtualFree(Memory, 0, MEM_RELEASE);
  }
}
internal bool DEBUGPlatformWriteEntireFile(char *FileName, uint64_t MemorySize, void *Memory){
  bool Result = false;
  HANDLE FileHandle = CreateFileA(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
  if (FileHandle != INVALID_HANDLE_VALUE){
    DWORD BytesWritten;
    if (WriteFile(FileHandle, Memory, (uint32_t)MemorySize, &BytesWritten, 0)){
      //NOTE: File read successfully
      Result = (BytesWritten == MemorySize);
    }
    else {
      //TODO: Logging
    }

    CloseHandle(FileHandle);
  }
  else {
    //TODO: Logging
  }

  return Result;
}

internal void Win32LoadXInput(){
  HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
  if (!XInputLibrary){
    //TODO: Diagnostic
    XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
  }
  if (!XInputLibrary){
    //TODO: Diagnostic
    XInputLibrary = LoadLibraryA("xinput1_3.dll");
  }
  if (XInputLibrary){
    XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
    XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");

    //TODO: Diagnostic
  }
  else {
    //TODO: Diagnostic
  }
}

internal void Win32InitDSound(HWND Window, int32_t SamplesPerSecond, int32_t BufferSize){
  //NOTE: Load the library
  HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

  if (DSoundLibrary){
    //NOTE: Get a DirectSound Object
    direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");
    LPDIRECTSOUND DirectSound;
    if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0))){

      WAVEFORMATEX WaveFormat = {};
      WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
      WaveFormat.nChannels = 2;
      WaveFormat.nSamplesPerSec = SamplesPerSecond;
      WaveFormat.wBitsPerSample = 16;
      WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
      WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
      WaveFormat.cbSize = 0;

      if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY))){
        DSBUFFERDESC BufferDiscription = {};
        BufferDiscription.dwSize = sizeof(BufferDiscription);
        BufferDiscription.dwFlags = DSBCAPS_PRIMARYBUFFER;

        //NOTE: Create a primary buffer
        //TODO: DSBCAPS_GLOBALFOCUS?
        LPDIRECTSOUNDBUFFER PrimaryBuffer;
        if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDiscription, &PrimaryBuffer, 0))){
          HRESULT Error = PrimaryBuffer->SetFormat(&WaveFormat);
          if (SUCCEEDED(Error)){
            //NOTE: We have finally set the format
            OutputDebugStringA("Primary buffer format was set\n");
          }
          else {
            //TODO: Diagnostic
          }
        }
      }
      else {
        //TODO: Diagnostic
      }

      //NOTE: Create a secondary buffer
      //TODO: DSBCAPS_GETCURRENTPOSITION?
      DSBUFFERDESC BufferDiscription = {};
      BufferDiscription.dwSize = sizeof(BufferDiscription);
      BufferDiscription.dwFlags = 0;
      BufferDiscription.dwBufferBytes = BufferSize;
      BufferDiscription.lpwfxFormat = &WaveFormat;
      HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDiscription, &GlobalSecondaryBuffer, 0);
      if (SUCCEEDED(Error)){
        OutputDebugStringA("Secondary buffer created successfully\n");
      }
      //NOTE: Start it playing

    }
    else {
      //TODO: Diagnostic
    }
  }
  else {
    //TODO: Diagnostic
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
  Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

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
      Assert(!"keyboard input came in through a non-dispatch message!");
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
      Result = DefWindowProcA(Window, Message, WParam, LParam);
      break;
    }
  }
  return(Result);
}

internal void Win32ClearBuffer(win32_sound_output *SoundOutput){
  VOID *Region1;
  DWORD Region1Size;
  VOID *Region2;
  DWORD Region2Size;

  if (SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize, &Region1, &Region1Size, &Region2, &Region2Size, 0))){
    uint8_t *DestSample = (uint8_t *)Region1;
    for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex){
      *DestSample++ = 0;
    }
    DestSample = (uint8_t *)Region2;
    for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex){
      *DestSample++ = 0;
    }

    GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);

  }
}

internal void Win32FillSoundBuffer(win32_sound_output *SoundOutput, DWORD BytesToLock, DWORD BytesToWrite, game_sound_output_buffer *SourceBuffer){
  VOID *Region1;
  DWORD Region1Size;
  VOID *Region2;
  DWORD Region2Size;

  if (SUCCEEDED(GlobalSecondaryBuffer->Lock(BytesToLock, BytesToWrite, &Region1, &Region1Size, &Region2, &Region2Size, 0))){
    //TODO: assert that Region1Size/Region2Size is valid
    DWORD Region1SampleCount = Region1Size/SoundOutput->BytesPerSample;
    int16_t *DestSample = (int16_t *)Region1;
    int16_t *SourceSample = SourceBuffer->Samples;
    for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex){
      *DestSample++ = *SourceSample++;
      *DestSample++ = *SourceSample++;
      ++SoundOutput->RunningSampleIndex;
    }

    DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSample;
    DestSample = (int16_t *)Region2;
    for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex){
      *DestSample++ = *SourceSample++;
      *DestSample++ = *SourceSample++;
      ++SoundOutput->RunningSampleIndex;
    }
    GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
  }
}

internal void Win32ProcessXInputDigitalButton(DWORD XInputButtonState, game_button_state *OldState, DWORD ButtonBit, game_button_state *NewState){
  NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
  NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

internal void Win32ProcessKeyboardMessage(game_button_state *NewState, bool IsDown){
  Assert(NewState->EndedDown != IsDown);
  NewState->EndedDown = IsDown;
  ++NewState->HalfTransitionCount;
}

internal real32 Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshold){
  real32 Result = 0;
  if (Value < -DeadZoneThreshold){
    Result =  (real32)(Value + DeadZoneThreshold) / (32768.0f - DeadZoneThreshold);
  }
  else if (Value > DeadZoneThreshold) {
    Result =  (real32)(Value + DeadZoneThreshold) / (32767.0f - DeadZoneThreshold);
  }

  return Result;
}

internal void Win32ProcessPendingMessages(game_controller_input *KeyboardController){
  MSG Message;

  while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)){
    switch (Message.message){
      case WM_QUIT:{
        GlobalRunning = false;
        break;
      }
      case WM_SYSKEYDOWN:
      case WM_SYSKEYUP:
      case WM_KEYDOWN:
      case WM_KEYUP:{
        uint32_t VKCode = (uint32_t)Message.wParam;
        bool WasDown = ((Message.lParam & (1 << 30)) != 0);
        bool IsDown = ((Message.lParam & (1 << 31)) == 0);
        if (IsDown != WasDown){
          if (VKCode == 'W'){
            Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown);
          }
          else if (VKCode == 'A'){
            Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
          }
          else if (VKCode == 'S'){
            Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown);
          }
          else if (VKCode == 'D'){
            Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown);
          }
          else if (VKCode == 'Q'){
            Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder, IsDown);
          }
          else if (VKCode == 'E'){
            Win32ProcessKeyboardMessage(&KeyboardController->RightShoulder, IsDown);
          }
          else if (VKCode == VK_UP){
            Win32ProcessKeyboardMessage(&KeyboardController->ActionUp, IsDown);
          }
          else if (VKCode == VK_DOWN){
            Win32ProcessKeyboardMessage(&KeyboardController->ActionDown, IsDown);
          }
          else if (VKCode == VK_RIGHT){
            Win32ProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown);
          }
          else if (VKCode == VK_LEFT){
            Win32ProcessKeyboardMessage(&KeyboardController->ActionLeft, IsDown);
          }
          else if (VKCode == VK_ESCAPE){
            GlobalRunning = false;
            Win32ProcessKeyboardMessage(&KeyboardController->Start, IsDown);
          }
          else if (VKCode == VK_SPACE){
            Win32ProcessKeyboardMessage(&KeyboardController->Back, IsDown);
          }
        }
        bool AltKeyWasDown = ((Message.lParam & (1 << 29)) != 0);
        if ((VKCode == VK_F4) && AltKeyWasDown){
          GlobalRunning = false;
        }
        break;
      }
      default:{
        TranslateMessage(&Message);
        DispatchMessage(&Message);
        break;
      }
    }
  }
}

inline LARGE_INTEGER Win32GetWallClock(){
  LARGE_INTEGER Result;
  QueryPerformanceCounter(&Result);
  return Result;
}

inline real32 Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End){

  real32 Result = ((real32)(End.QuadPart - Start.QuadPart) / (real32)GlobalPerfCountFrequency);
  return Result;
}
int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode){

  LARGE_INTEGER PerfCountFrequencyResult;
  QueryPerformanceFrequency(&PerfCountFrequencyResult);
  GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;
  //NOTE: Set the windows scheduler granularity to 1ms
  UINT DesiredSchedulerMS = 1;
  bool SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

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

  //TODO: How do we reliably query on this on windows?
  int MonitorRefreshHz = 60;
  int GameUpdateHz = MonitorRefreshHz / 2;
  real32 TargetSecondsElapsedPerFrame = 1.0f / (real32)GameUpdateHz;

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

          //NOTE: Graphics test
          int XOffset = 0;
          int YOffset = 0;

          win32_sound_output SoundOutput = {};
          SoundOutput.SamplesPerSecond = 48000;
          SoundOutput.RunningSampleIndex = 0;
          SoundOutput.BytesPerSample = sizeof(int16_t)*2;
          SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample;
          SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;
          Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
          Win32ClearBuffer(&SoundOutput);
          GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

          GlobalRunning = true;

          // TODO: Pool with bitmap VirtualAlloc
          int16_t *Samples = (int16_t*)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

#if HANDMADE_INTERNAL
          LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
          LPVOID BaseAddress = 0;
#endif
          game_memory GameMemory = {};
          GameMemory.PermanentStorageSize = Megabytes(64);
          GameMemory.TransientStorageSize = Gigabytes(1);
          uint64_t TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
          GameMemory.PermanentStorage = (int16_t*)VirtualAlloc(BaseAddress, (size_t)TotalSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
          GameMemory.TransientStorage = ((int8_t*)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);
          if (Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage){
            game_input Input[2] = {};
            game_input *NewInput = &Input[0];
            game_input *OldInput = &Input[1];

            LARGE_INTEGER LastCounter = Win32GetWallClock();
            uint64_t LastCycleCount = __rdtsc();

            while (GlobalRunning){
              //TODO: Create zero macro
              //TODO: We can't zero everything because the up/down state will be wrong!!
              game_controller_input *OldKeyboardController = GetController(OldInput, 0);
              game_controller_input *NewKeyboardController = GetController(NewInput, 0);
              *NewKeyboardController = {};
              NewKeyboardController->IsConnected = true;
              for (int ButtonIndex = 0; ButtonIndex < ArrayCount(NewKeyboardController->Buttons); ++ButtonIndex){
                NewKeyboardController->Buttons[ButtonIndex].EndedDown = OldKeyboardController->Buttons[ButtonIndex].EndedDown;
              }
              Win32ProcessPendingMessages(NewKeyboardController);

              //TODO: Need to not pull disconnected controllers to avoid xinput frame rate hit on older libraries
              //TODO: should we pull this more frequently
              DWORD MaxControllerCount = XUSER_MAX_COUNT;
              if (MaxControllerCount > ArrayCount(OldInput->Controllers) - 1){
                MaxControllerCount = ArrayCount(NewInput->Controllers) - 1;
              }
              for (DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount; ControllerIndex++ ){
                DWORD OurControllerIndex = ControllerIndex + 1;
                game_controller_input *OldController = GetController(OldInput, OurControllerIndex);
                game_controller_input *NewController = GetController(NewInput, OurControllerIndex);

                XINPUT_STATE ControllerState;
                if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS){
                  NewController->IsConnected = true;

                  //NOTE: This controller is plugged in
                  //TODO: See if ControllerState.dwPacketNumber increments too rapidly
                  XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                  //TODO: This is a square deadzone, check XInput to verify that the deadzone is "round" and show how to do round deadzone processing
                  NewController->StickAverageX = Win32ProcessXInputStickValue(Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                  NewController->StickAverageY = Win32ProcessXInputStickValue(Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                  if (NewController->StickAverageX != 0.0f || NewController->StickAverageY != 0.0f){
                    NewController->IsAnalog = true;
                  }

                  if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP){
                    NewController->StickAverageY = 1.0f;
                    NewController->IsAnalog = false;
                  }
                  if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN){
                    NewController->StickAverageY = -1.0f;
                    NewController->IsAnalog = false;
                  }
                  if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT){
                    NewController->StickAverageX = -1.0f;
                    NewController->IsAnalog = false;
                  }
                  if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT){
                    NewController->StickAverageX = 1.0f;
                    NewController->IsAnalog = false;
                  }

                  real32 Threshold = 0.5f;
                  DWORD FakeMoveButtons = (NewController->StickAverageX < -Threshold) ? 1 : 0;
                  Win32ProcessXInputDigitalButton((NewController->StickAverageX < -Threshold) ? 1 : 0, &OldController->MoveLeft, 1, &NewController->MoveLeft);
                  Win32ProcessXInputDigitalButton((NewController->StickAverageX > Threshold) ? 1 : 0, &OldController->MoveRight, 1, &NewController->MoveRight);
                  Win32ProcessXInputDigitalButton((NewController->StickAverageY < -Threshold) ? 1 : 0, &OldController->MoveDown, 1, &NewController->MoveDown);
                  Win32ProcessXInputDigitalButton((NewController->StickAverageY > Threshold) ? 1 : 0, &OldController->MoveUp, 1, &NewController->MoveUp);


                  Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionDown, XINPUT_GAMEPAD_A, &NewController->ActionDown);
                  Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionRight, XINPUT_GAMEPAD_B, &NewController->ActionRight);
                  Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionLeft, XINPUT_GAMEPAD_X, &NewController->ActionLeft);
                  Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionUp, XINPUT_GAMEPAD_Y, &NewController->ActionUp);
                  Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER, &NewController->LeftShoulder);
                  Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER, &NewController->RightShoulder);
                  Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Back, XINPUT_GAMEPAD_BACK, &NewController->Back);
                  Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Start, XINPUT_GAMEPAD_START, &NewController->Start);

                }
                else {
                  //NOTE: The controller is not available
                  NewController->IsConnected = false;

                }
              }

              XINPUT_VIBRATION Vibration;
              //NOTE: Valid values are in the range 0 to 65,535
              WORD speed = 4000;
              Vibration.wLeftMotorSpeed = speed;
              Vibration.wRightMotorSpeed = speed;

              XInputSetState(0, &Vibration);

              DWORD BytesToLock = 0;
              DWORD PlayCursor = 0;
              DWORD WriteCursor = 0;
              DWORD TargetCursor = 0;
              DWORD BytesToWrite = 0;
              bool SoundIsValid = false;
              //TODO: Tighten up sound logic so that we know where we should be
              //      writing to and can anticipate the time spent in the game update.
              if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor))){
                BytesToLock = (SoundOutput.RunningSampleIndex*SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
                TargetCursor = (PlayCursor + (SoundOutput.LatencySampleCount*SoundOutput.BytesPerSample)) % SoundOutput.SecondaryBufferSize;

                if (BytesToLock > TargetCursor){
                  BytesToWrite = SoundOutput.SecondaryBufferSize - BytesToLock;
                  BytesToWrite += TargetCursor;
                }
                else{
                  BytesToWrite = TargetCursor - BytesToLock;
                }

                SoundIsValid = true;
              }

              game_sound_output_buffer SoundBuffer = {};
              SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
              SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
              SoundBuffer.Samples = Samples;

              game_offscreen_buffer Buffer = {};
              Buffer.Memory = GlobalBackBuffer.Memory;
              Buffer.Width = GlobalBackBuffer.Width;
              Buffer.Height = GlobalBackBuffer.Height;
              Buffer.Pitch = GlobalBackBuffer.Pitch;
              GameUpdateAndRender(&GameMemory, NewInput, &Buffer, &SoundBuffer);

              //NOTE: DirectSound output test
              if (SoundIsValid){
                /* Data structure
                    [LEFT RIGHT] [LEFT RIGHT] [LEFT RIGHT] [LEFT RIGHT]
                    16bit 16bit  16bit 16bit ...
                */
                Win32FillSoundBuffer(&SoundOutput, BytesToLock, BytesToWrite, &SoundBuffer);
              }

              LARGE_INTEGER WorkCounter = Win32GetWallClock();
              real32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);

              //TODO: NOT TESTED YET
              real32 SecondsElapsedForFrame = WorkSecondsElapsed;
              if (SecondsElapsedForFrame < TargetSecondsElapsedPerFrame){
                while (SecondsElapsedForFrame < TargetSecondsElapsedPerFrame){

                  if (SleepIsGranular){
                    DWORD SleepMS =  DWORD(1000.0f * (TargetSecondsElapsedPerFrame - SecondsElapsedForFrame));
                    Sleep(SleepMS);
                }
                  SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                }
              }
              else {
                //TODO: Missed frame rate
                //TODO: Logging
              }

              win32_window_dimension Dimension = Win32GetWindowDimension(Window);
              Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);

              LARGE_INTEGER EndCounter = Win32GetWallClock();
              real32 MSPerFrame = 1000.0f * Win32GetSecondsElapsed(LastCounter, EndCounter);
              LastCounter = EndCounter;

              uint64_t EndCycleCount = __rdtsc();
              uint64_t CycleElapsed = EndCycleCount - LastCycleCount;
              LastCycleCount = EndCycleCount;
              
#if 1
              real32 FPS = (real32)(1000 / (real32)MSPerFrame);
              real32 MCPF = (real32)(CycleElapsed/(real32)(1000*1000));

              char CharBuffer[256];
              sprintf_s(CharBuffer, "%fms/f, %fFPS, %fMc/f\n", MSPerFrame, FPS, MCPF);
              OutputDebugStringA(CharBuffer);
#endif

              game_input *Temp = NewInput;
              NewInput = OldInput;
              OldInput = Temp;


            }
          }
          else {
            //TODO: Logging
          }
      }
      else {
        //TODO: Logging
      }
  }
  else {
    //TODO: Logging
  }
  return 0;
}
