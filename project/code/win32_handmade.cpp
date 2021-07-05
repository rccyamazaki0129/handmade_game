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

//TODO: Implement sine ourselves

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
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

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

void *PlatformLoadFile(char *FileName){
  //NOTE: Implements the Win32 file loading
  return 0;
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
      bool AltKeyWasDown = ((LParam & (1 << 29)) != 0);
      if ((VKCode == VK_F4) && AltKeyWasDown){
        GlobalRunning = false;
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
      Result = DefWindowProcA(Window, Message, WParam, LParam);
      break;
    }
  }
  return(Result);
}

struct win32_sound_output{
  int SamplesPerSecond;
  int ToneHz;
  int16_t ToneVolume;
  uint32_t RunningSampleIndex;
  int WavePeriod;
  int BytesPerSample;
  int SecondaryBufferSize;
  real32 tSine;
  int LatencySampleCount;
};

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

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode){

  LARGE_INTEGER PerfCountFrequencyResult;
  QueryPerformanceFrequency(&PerfCountFrequencyResult);
  int64_t PerfCountFrequency = PerfCountFrequencyResult.QuadPart;

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

          //NOTE: Graphics test
          int XOffset = 0;
          int YOffset = 0;

          win32_sound_output SoundOutput = {};
          SoundOutput.SamplesPerSecond = 48000;
          SoundOutput.ToneHz = 256;
          SoundOutput.ToneVolume = 6000;
          SoundOutput.RunningSampleIndex = 0;
          SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond / SoundOutput.ToneHz;
          SoundOutput.BytesPerSample = sizeof(int16_t)*2;
          SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample;
          SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;
          Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
          Win32ClearBuffer(&SoundOutput);
          GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

          GlobalRunning = true;

          // TODO: Pool with bitmap VirtualAlloc
          int16_t *Samples = (int16_t*)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);


          LARGE_INTEGER LastCounter;
          QueryPerformanceCounter(&LastCounter);
          uint64_t LastCycleCount = __rdtsc();

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
              if (XInputGetState(ControllerIntex, &ControllerState) == ERROR_SUCCESS){
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

                //NOTE: FixControllerInput Function
                //      since my gamepad's LStick is working poorly,
                //      Negative Stick value must be rounded-up

                //TODO: We will do deadzone handling later using
                //#define XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE  7849
                //#define XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE 8689
                if (StickX > 0){
                  XOffset += StickX >> 13;
                }
                else {
                  XOffset += StickX >> 14;
                }

                if (StickY > 0){
                  YOffset += StickY >> 13;
                }
                else {
                  YOffset += StickY >> 14;
                }

                SoundOutput.ToneHz = (int)(((real32)StickY / 3000.0f) * 16.0f) + 512;
                SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond / SoundOutput.ToneHz;

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

            DWORD BytesToLock;
            DWORD PlayCursor;
            DWORD WriteCursor;
            DWORD TargetCursor;
            DWORD BytesToWrite;
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
            GameUpdateAndRender(&Buffer, XOffset, YOffset, &SoundBuffer, SoundOutput.ToneHz);

            //NOTE: DirectSound output test
            if (SoundIsValid){
              /* Data structure
                  [LEFT RIGHT] [LEFT RIGHT] [LEFT RIGHT] [LEFT RIGHT]
                  16bit 16bit  16bit 16bit ...
              */
              Win32FillSoundBuffer(&SoundOutput, BytesToLock, BytesToWrite, &SoundBuffer);
            }

            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);

            uint64_t EndCycleCount = __rdtsc();

            LARGE_INTEGER EndCounter;
            QueryPerformanceCounter(&EndCounter);

            uint64_t CycleElapsed = EndCycleCount - LastCycleCount;
            int64_t CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
            real32 MSPerFrame = (real32)((1000 * CounterElapsed) / (real32)PerfCountFrequency);
            real32 FPS = (real32)(1000 / (real32)MSPerFrame);
            real32 MCPF = (real32)(CycleElapsed/(real32)(1000*1000));

#if 1
            char CharBuffer[256];
            sprintf(CharBuffer, "%fms/f, %fFPS, %fMc/f\n", MSPerFrame, FPS, MCPF);
            OutputDebugStringA(CharBuffer);
#endif
            LastCycleCount = EndCycleCount;
            LastCounter = EndCounter;
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
