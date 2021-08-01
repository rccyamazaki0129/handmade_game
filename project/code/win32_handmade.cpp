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
#include "handmade.h"

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

void CatStrings(size_t SourceACount, char *SourceA, size_t SourceBCount, char *SourceB, size_t DestCount, char *Dest)
{
  //TODO: Dest bounds checking
  for (int Index = 0; Index < SourceACount; Index++)
  {
    *Dest++ = *SourceA++;
  }

  for (int Index = 0; Index < SourceBCount; Index++)
  {
    *Dest++ = *SourceB++;
  }

  *Dest++ = 0;
}

internal void Win32GetEXEFileName(win32_state *State)
{
  //NOTE: Never use MAX_PATH in code that is user-facing, because it can be dangerous and lead to bad results.
  DWORD SizeOfFileName = GetModuleFileNameA(0, State->EXEFileName, sizeof(State->EXEFileName));
  State->OnePastLastEXEFileNameSlash = State->EXEFileName;
  for (char *Scan = State->EXEFileName; *Scan; ++Scan)
  {
    if (*Scan == '\\')
    {
      State->OnePastLastEXEFileNameSlash = Scan + 1;
    }
  }
}

internal int StringLength(char* String)
{
  int Count = 0;
  while (*String++)
  {
    ++Count;
  }
  return Count;
}

internal void Win32BuildEXEPathFileName(win32_state *State, char* FileName, int DestCount, char* Dest)
{
  CatStrings(State->OnePastLastEXEFileNameSlash - State->EXEFileName, State->EXEFileName, StringLength(FileName), FileName, DestCount, Dest);
}

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory){
  if (Memory){
    VirtualFree(Memory, 0, MEM_RELEASE);
  }
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile){

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
          DEBUGPlatformFreeFileMemory(Thread, Result.Contents);
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

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile){
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

inline FILETIME Win32GetLastWriteTime(char *FileName)
{
  FILETIME LastWriteTime = {};
  WIN32_FILE_ATTRIBUTE_DATA Data;
  if (GetFileAttributesExA(FileName, GetFileExInfoStandard, &Data))
  {
    LastWriteTime = Data.ftLastWriteTime;
  }
  return LastWriteTime;
}

internal win32_game_code Win32LoadGameCode(char* SourceDLLName, char* TempDLLName, char*LockFileName)
{
  win32_game_code Result = {};

  WIN32_FILE_ATTRIBUTE_DATA Ignored;
  if (!GetFileAttributesExA(LockFileName, GetFileExInfoStandard, &Ignored))
  {
    Result.DLLLastWriteTime = Win32GetLastWriteTime(SourceDLLName);
    CopyFile(SourceDLLName, TempDLLName, false);
    Result.GameCodeDLL = LoadLibraryA(TempDLLName);
    if (Result.GameCodeDLL){
      Result.UpdateAndRender = (game_update_and_render *)GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");
      Result.GetSoundSamples = (game_get_sound_samples *)GetProcAddress(Result.GameCodeDLL, "GameGetSoundSamples");
      Result.IsValid = (Result.UpdateAndRender && Result.GetSoundSamples);
    }
  }
  if (!Result.IsValid){
    Result.UpdateAndRender = 0;
    Result.GetSoundSamples = 0;
  }

  return Result;
}

internal void Win32UnloadGameCode(win32_game_code *GameCode){
  if (GameCode->GameCodeDLL){
    FreeLibrary(GameCode->GameCodeDLL);
    GameCode->GameCodeDLL = 0;
  }
  GameCode->IsValid = false;
  GameCode->UpdateAndRender = 0;
  GameCode->GetSoundSamples = 0;
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
      BufferDiscription.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
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
  Buffer->BytesPerPixel = BytesPerPixel;

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
  Buffer->Pitch = Buffer->Width * BytesPerPixel;
}

internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight){
  //TODO: Aspect ratio correction
  int OffsetX = 10;
  int OffsetY = 10;

  PatBlt(DeviceContext, 0, 0, WindowWidth, OffsetY, BLACKNESS);
  PatBlt(DeviceContext, 0, OffsetY + Buffer->Height, WindowWidth, WindowHeight, BLACKNESS);
  PatBlt(DeviceContext, 0, 0, OffsetX, WindowHeight, BLACKNESS);
  PatBlt(DeviceContext, OffsetX + Buffer->Width, 0, WindowWidth, WindowHeight, BLACKNESS);

  //NOTE: For prototyping purposes, we are going to always blit 1-to-1 pixels to make sure we don't introduce artifacts with stretching while we are learning to code the renderer
  StretchDIBits(DeviceContext,
                OffsetX, OffsetY, Buffer->Width, Buffer->Height,
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
#if 0
      if (WParam)
      {
        SetLayeredWindowAttributes(Window, RGB(0,0,0), 255, LWA_ALPHA);
      }
      else
      {
        SetLayeredWindowAttributes(Window, RGB(0,0,0), 64, LWA_ALPHA);
      }
#endif
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
  if (NewState->EndedDown != IsDown)
  {
    NewState->EndedDown = IsDown;
    ++NewState->HalfTransitionCount;
  }
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

internal void Win32GetInputFileLocation(win32_state *State, bool InputStream, int SlotIndex, int DestCount, char *Dest)
{
  char Temp[64];
  wsprintf(Temp, "loop_edit_%d_%s.rei", SlotIndex, InputStream ? "input" : "state");
  Win32BuildEXEPathFileName(State, Temp, DestCount, Dest);
}

internal win32_replay_buffer *Win32GetReplayBuffer(win32_state *State, unsigned int Index)
{
  Assert(Index > 0);
  Assert(Index < ArrayCount(State->ReplayBuffers));
  win32_replay_buffer *Result = &State->ReplayBuffers[Index];
  return Result;
}

internal void Win32BeginRecordingInput(win32_state *State, int InputRecordingIndex)
{
  win32_replay_buffer *ReplayBuffer = Win32GetReplayBuffer(State, InputRecordingIndex);
  if (ReplayBuffer->MemoryBlock)
  {
    State->InputRecordingIndex = InputRecordingIndex;
    char FileName[WIN32_STATE_FILE_NAME_COUNT];
    Win32GetInputFileLocation(State, true, InputRecordingIndex, sizeof(FileName), FileName);
    State->RecordingHandle = CreateFileA(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
#if 0
    LARGE_INTEGER FilePosition;
    FilePosition.QuadPart = State->TotalSize;
    SetFilePointerEx(State->RecordingHandle, FilePosition, 0, FILE_BEGIN);
#endif
    CopyMemory(ReplayBuffer->MemoryBlock, State->GameMemoryBlock, State->TotalSize);
  }
}

internal void Win32EndRecordingInput(win32_state *State)
{
  CloseHandle(State->RecordingHandle);
  State->InputRecordingIndex = 0;
}

internal void Win32BeginInputPlayBack(win32_state *State, int InputPlayingIndex)
{
  win32_replay_buffer *ReplayBuffer = Win32GetReplayBuffer(State, InputPlayingIndex);
  if (ReplayBuffer->MemoryBlock)
  {
    State->InputPlayingIndex = InputPlayingIndex;
    char FileName[WIN32_STATE_FILE_NAME_COUNT];
    Win32GetInputFileLocation(State, true, InputPlayingIndex, sizeof(FileName), FileName);
    State->PlayBackHandle = CreateFileA(FileName, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
#if 0
    LARGE_INTEGER FilePosition;
    FilePosition.QuadPart = State->TotalSize;
    SetFilePointerEx(State->PlayBackHandle, FilePosition, 0, FILE_BEGIN);
#endif
    CopyMemory(State->GameMemoryBlock, ReplayBuffer->MemoryBlock, State->TotalSize);
  }
}

internal void Win32EndInputPlayBack(win32_state *State)
{
  CloseHandle(State->PlayBackHandle);
  State->InputPlayingIndex = 0;
}

internal void Win32RecordInput(win32_state *State, game_input *NewInput)
{
  DWORD BytesWritten;
  WriteFile(State->RecordingHandle, NewInput, sizeof(*NewInput), &BytesWritten, 0);
}

internal void Win32PlayBackInput(win32_state *State, game_input *NewInput)
{
  DWORD BytesRead = 0;
  if (ReadFile(State->PlayBackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0))
  {
    if (BytesRead == 0)
    {
      //NOTE: We've hit the end of the stream
      int PlayingIndex = State->InputPlayingIndex;
      Win32EndInputPlayBack(State);
      Win32BeginInputPlayBack(State, PlayingIndex);
      ReadFile(State->PlayBackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0);
    }
  }
}

internal void Win32ProcessPendingMessages(win32_state *State, game_controller_input *KeyboardController){
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
        //NOTE: Since we are comparing WasDown to IsDown, we must use == and != to convert these bit tests to actual 0 or 1 value.
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
#if HANDMADE_INTERNAL
          else if (VKCode == 'L')
          {
            if (IsDown)
            {
              if (State->InputPlayingIndex == 0)
              {
                if (State->InputRecordingIndex == 0)
                {
                  Win32BeginRecordingInput(State, 1);
                }
                else
                {
                  Win32EndRecordingInput(State);
                  Win32BeginInputPlayBack(State, 1);
                }
              }
              else
              {
                Win32EndInputPlayBack(State);
              }
            }
          }
#endif
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

internal void Win32DebugDrawVertical(win32_offscreen_buffer *BackBuffer, int X, int Top, int Bottom, uint32_t Color){
  if (Top <= 0){
    Top = 0;
  }
  if (Bottom > BackBuffer->Height){
    Bottom = BackBuffer->Height;
  }

  if ((X >= 0) && (X < BackBuffer->Width)){
    uint8_t *Pixel = ((uint8_t *)(BackBuffer->Memory) + X*BackBuffer->BytesPerPixel + Top*BackBuffer->Pitch);
    for (int Y = Top; Y < Bottom; ++Y){
      *(uint32_t *)Pixel = Color;
      Pixel += BackBuffer->Pitch;
    }
  }
}

inline void Win32DrawSoundBufferMarker(win32_offscreen_buffer *BackBuffer, win32_sound_output *SoundOutput, real32 C, int PadX, int Top, int Bottom, DWORD Value, uint32_t Color){
  int X = PadX + (int)(C * (real32)Value);
  Win32DebugDrawVertical(BackBuffer, X, Top, Bottom, Color);

}

#if 0
internal void Win32DebugSyncDisplay(win32_offscreen_buffer *BackBuffer, int MarkerCount, win32_debug_time_marker *Markers, int CurrentMarkerIndex, win32_sound_output *SoundOutput, real32 TargetSecondsPerFrame){
  //TODO: Draw where we are writing out sound
  int PadX = 16;
  int PadY = 16;
  int LineHeight = 64;

  real32 C = (real32)(BackBuffer->Width - 2*PadX) / (real32)SoundOutput->SecondaryBufferSize;

  for (int MarkerIndex = 0; MarkerIndex < MarkerCount; ++MarkerIndex){
    win32_debug_time_marker *ThisMarker = &Markers[MarkerIndex];
    Assert(ThisMarker->OutputPlayCursor < SoundOutput->SecondaryBufferSize);
    Assert(ThisMarker->OutputWriteCursor < SoundOutput->SecondaryBufferSize);
    Assert(ThisMarker->OutputLocation < SoundOutput->SecondaryBufferSize);
    Assert(ThisMarker->OutputByteCount < SoundOutput->SecondaryBufferSize);
    Assert(ThisMarker->FlipWriteCursor < SoundOutput->SecondaryBufferSize);
    Assert(ThisMarker->FlipPlayCursor < SoundOutput->SecondaryBufferSize);

    DWORD PlayColor = 0xFFFFFFFF;
    DWORD WriteColor = 0xFFFF0000;
    DWORD ExpectedFlipColor = 0x0000FFFF;
    DWORD PlayWindowColor = 0xFF00FF00;
    int Top = PadY;
    int Bottom = PadY + LineHeight;

    if (MarkerIndex == CurrentMarkerIndex){
      Top += LineHeight + PadY;
      Bottom += LineHeight + PadY;

      int FirstTop = Top;

      Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputPlayCursor, PlayColor);
      Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputWriteCursor, WriteColor);

      Top += LineHeight + PadY;
      Bottom += LineHeight + PadY;
      Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputLocation, PlayColor);
      Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputLocation + ThisMarker->OutputByteCount, WriteColor);


      Top += LineHeight + PadY;
      Bottom += LineHeight + PadY;

      Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, FirstTop, Bottom, ThisMarker->ExpectedFlipPlayCursor, ExpectedFlipColor);
    }
    Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor, PlayColor);
    Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor + 480 * SoundOutput->BytesPerSample, PlayWindowColor);
    Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipWriteCursor, WriteColor);
  }
}
#endif

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
{
  win32_state Win32State = {};

  LARGE_INTEGER PerfCountFrequencyResult;
  QueryPerformanceFrequency(&PerfCountFrequencyResult);
  GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;

  Win32GetEXEFileName(&Win32State);
  char SourceGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
  Win32BuildEXEPathFileName(&Win32State, "handmade.dll", sizeof(SourceGameCodeDLLFullPath), SourceGameCodeDLLFullPath);

  char TempGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
  Win32BuildEXEPathFileName(&Win32State, "handmade_temp.dll", sizeof(TempGameCodeDLLFullPath), TempGameCodeDLLFullPath);

  char GameCodeLockFullPath[WIN32_STATE_FILE_NAME_COUNT];
  Win32BuildEXEPathFileName(&Win32State, "Lock.tmp", sizeof(GameCodeLockFullPath), GameCodeLockFullPath);

  //NOTE: Set the windows scheduler granularity to 1ms
  UINT DesiredSchedulerMS = 1;
  bool SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

  Win32LoadXInput();
  WNDCLASSA WindowClass = {};
  /*NOTE: 1080p display mode is 1920x1080 -> half of that is 960x540
  1920 -> 2048 = 2048 - 1920 -> 128pix
  1080 -> 2048 = 2048 - 1080 -> 968pix
  1024 + 128 = 1152pix
  */
  Win32ResizeDIBSection(&GlobalBackBuffer, 960, 540);

  //TODO: Check if CS_OWNDC, CS_HREDRAW, CS_VREDRAW still matter
  WindowClass.style = CS_HREDRAW|CS_VREDRAW;
  WindowClass.lpfnWndProc = Win32MainWindowCallback;
  WindowClass.hInstance = Instance;
  // WindowClass.hIcon;
  // LPCSTR    lpszMenuName;
  WindowClass.lpszClassName = "HandmadeWindowClass";

  if (RegisterClass(&WindowClass)){
    HWND Window = CreateWindowEx(
      0,//WS_EX_TOPMOST|WS_EX_LAYERED,
      WindowClass.lpszClassName,
      "HandmadeWindowClass",
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
          //NOTE: Graphics test
          int XOffset = 0;
          int YOffset = 0;

          win32_sound_output SoundOutput = {};
          //TODO: How do we reliably query on this on windows?
          int MonitorRefreshHz = 60;
          HDC RefreshDC = GetDC(Window);
          int Win32RefreshRate = GetDeviceCaps(RefreshDC, VREFRESH);
          ReleaseDC(Window, RefreshDC);
          if (Win32RefreshRate > 1)
          {
            MonitorRefreshHz = Win32RefreshRate;
          }
          real32 GameUpdateHz = (MonitorRefreshHz / 2.0f);
          real32 TargetSecondsPerFrame = 1.0f / (real32)GameUpdateHz;

          SoundOutput.SamplesPerSecond = 48000;
          SoundOutput.RunningSampleIndex = 0;
          SoundOutput.BytesPerSample = sizeof(int16_t)*2;
          SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample;
          //TODO: Actually compute this variance and see what the lowest reasonable value is.
          SoundOutput.SafetyBytes = (int)(((real32)SoundOutput.SamplesPerSecond * (real32)SoundOutput.BytesPerSample / GameUpdateHz) / 3);

          Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
          Win32ClearBuffer(&SoundOutput);
          GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

          GlobalRunning = true;
#if 0
          //NOTE: This tests the PlayCursor/WriteCursor update frequency
          //      on the handmade machine, it was 480 samples.
          while (GlobalRunning){
            DWORD PlayCursor;
            DWORD WriteCursor;
            GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);
            char CharBuffer[256];
            sprintf_s(CharBuffer, "PC:%u WC:%u\n", PlayCursor, WriteCursor);
            OutputDebugStringA(CharBuffer);
          }
#endif

          // TODO: Pool with bitmap VirtualAlloc
          int16_t *Samples = (int16_t*)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

#if HANDMADE_INTERNAL
          LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
          LPVOID BaseAddress = 0;
#endif
          game_memory GameMemory = {};
          GameMemory.PermanentStorageSize = Megabytes(64);
          GameMemory.TransientStorageSize = Megabytes(64);
          GameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
          GameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
          GameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;

          //TODO: Use MEM_LARGE_PAGES and call adjust token privileges
          //TODO: TransientStrage needs to be broken up into game transient and cache transient, and only the former need be saved for state playback.
          Win32State.TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
          Win32State.GameMemoryBlock = (int16_t*)VirtualAlloc(BaseAddress, (size_t)Win32State.TotalSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
          GameMemory.PermanentStorage = Win32State.GameMemoryBlock;
          GameMemory.TransientStorage = ((int8_t*)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);

          for (int ReplayIndex = 0; ReplayIndex < ArrayCount(Win32State.ReplayBuffers); ++ReplayIndex)
          {
            win32_replay_buffer *ReplayBuffer = &Win32State.ReplayBuffers[ReplayIndex];

            Win32GetInputFileLocation(&Win32State, false, ReplayIndex, sizeof(ReplayBuffer->FileName), ReplayBuffer->FileName);
            ReplayBuffer->FileHandle = CreateFileA(ReplayBuffer->FileName, GENERIC_WRITE|GENERIC_READ, 0, 0, CREATE_ALWAYS, 0, 0);

            DWORD MaxSizeHigh = (Win32State.TotalSize >> 32);
            DWORD MaxSizeLow = (Win32State.TotalSize & 0xFFFFFFFF);

            ReplayBuffer->MemoryMap = CreateFileMapping(ReplayBuffer->FileHandle, 0, PAGE_READWRITE, MaxSizeHigh, MaxSizeLow, 0);
            DWORD Error = GetLastError();
            ReplayBuffer->MemoryBlock = MapViewOfFile(ReplayBuffer->MemoryMap, FILE_MAP_ALL_ACCESS, 0, 0, Win32State.TotalSize);

            // ReplayBuffer->MemoryBlock = VirtualAlloc(0, (size_t)Win32State.TotalSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

            if (ReplayBuffer->MemoryBlock)
            {
            }
            else
            {
              //TODO: Diagnostic
            }
          }

          if (Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage){
            game_input Input[2] = {};
            game_input *NewInput = &Input[0];
            game_input *OldInput = &Input[1];

            LARGE_INTEGER LastCounter = Win32GetWallClock();
            LARGE_INTEGER FlipWallClock = Win32GetWallClock();

            int DebugTimeMarkerIndex = 0;
            win32_debug_time_marker DebugTimeMarkers[30] = {};

            DWORD AudioLatencyBytes = 0;
            real32 AudioLatencySeconds = 0;
            bool SoundIsValid = false;

            win32_game_code Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath, GameCodeLockFullPath);
            uint32_t LoadCounter = 0;

            uint64_t LastCycleCount = __rdtsc();

            while (GlobalRunning){

              NewInput->dtForFrame = TargetSecondsPerFrame;

              FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);

              // if (LoadCounter++ > 120)
              if (CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0)
              {
                Sleep(500);//IMPORTANT: Build another DLL takes a while, on the other hand, loading it is too quick
                Win32UnloadGameCode(&Game);
                Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath, GameCodeLockFullPath);
                LoadCounter = 0;
              }
              //TODO: Create zero macro
              //TODO: We can't zero everything because the up/down state will be wrong!!
              game_controller_input *OldKeyboardController = GetController(OldInput, 0);
              game_controller_input *NewKeyboardController = GetController(NewInput, 0);
              *NewKeyboardController = {};
              NewKeyboardController->IsConnected = true;
              for (int ButtonIndex = 0; ButtonIndex < ArrayCount(NewKeyboardController->Buttons); ++ButtonIndex){
                NewKeyboardController->Buttons[ButtonIndex].EndedDown = OldKeyboardController->Buttons[ButtonIndex].EndedDown;
              }
              Win32ProcessPendingMessages(&Win32State, NewKeyboardController);

              POINT MouseP;
              GetCursorPos(&MouseP);
              ScreenToClient(Window, &MouseP);
              NewInput->MouseX = MouseP.x;
              NewInput->MouseY = MouseP.y;
              NewInput->MouseZ = 0;//TODO: Support mousewheel?
              Win32ProcessKeyboardMessage(&NewInput->MouseButtons[0], GetKeyState(VK_LBUTTON) & (1 << 15));
              Win32ProcessKeyboardMessage(&NewInput->MouseButtons[1], GetKeyState(VK_MBUTTON) & (1 << 15));
              Win32ProcessKeyboardMessage(&NewInput->MouseButtons[2], GetKeyState(VK_RBUTTON) & (1 << 15));
              Win32ProcessKeyboardMessage(&NewInput->MouseButtons[3], GetKeyState(VK_XBUTTON1) & (1 << 15));
              Win32ProcessKeyboardMessage(&NewInput->MouseButtons[4], GetKeyState(VK_XBUTTON2) & (1 << 15));


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
                  NewController->IsAnalog = OldController->IsAnalog;
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
              WORD speed = 0;
              Vibration.wLeftMotorSpeed = speed;
              Vibration.wRightMotorSpeed = speed;

              XInputSetState(0, &Vibration);

              thread_context Thread = {};

              game_offscreen_buffer Buffer = {};
              Buffer.Memory = GlobalBackBuffer.Memory;
              Buffer.Width = GlobalBackBuffer.Width;
              Buffer.Height = GlobalBackBuffer.Height;
              Buffer.Pitch = GlobalBackBuffer.Pitch;
              Buffer.BytesPerPixel = GlobalBackBuffer.BytesPerPixel;

              if (Win32State.InputRecordingIndex)
              {
                Win32RecordInput(&Win32State, NewInput);
              }
              if (Win32State.InputPlayingIndex)
              {
                Win32PlayBackInput(&Win32State, NewInput);
              }
              if (Game.UpdateAndRender)
              {
                Game.UpdateAndRender(&Thread, &GameMemory, NewInput, &Buffer);
              }

              LARGE_INTEGER AudioWallClock = Win32GetWallClock();
              real32 FromBeginToAudioSeconds = Win32GetSecondsElapsed(FlipWallClock, AudioWallClock);

              DWORD PlayCursor;
              DWORD WriteCursor;
              if (GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK){
                /*
                NOTE:
                Here is how sound output computation works...

                We define a safety value that is the number of samples we think our game update
                may vary by (let's say up to 2ms)

                When we wake up to write audio, we will look and see what the play cursor position is
                and we will forecast ahead where we think the play cursor will be on the next frame boundary.

                We will then look to see if the write cursor is before that by at least our safety value.
                If it is, the target fill position is that frame boundary plus one frame,
                This gives us perfect audio sync in the case of a card
                that has low enough latency.

                If the write cursor is _after_ that safety margine, then we assume we can never sync the audio perfectly, so we will write one frame's worth of audio plus the safety margin's worth of guard sample.
                */


                  /* Data structure
                      [LEFT RIGHT] [LEFT RIGHT] [LEFT RIGHT] [LEFT RIGHT]
                      16bit 16bit  16bit 16bit ...
                  */
                if (!SoundIsValid){
                  SoundOutput.RunningSampleIndex = WriteCursor / SoundOutput.BytesPerSample;
                  SoundIsValid = true;
                }
                //NOTE: Compute how much sound to write and where
                DWORD BytesToLock = (SoundOutput.RunningSampleIndex*SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;

                DWORD ExpectedSoundBytesPerFrame = (int)((real32)(SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample) / GameUpdateHz);

                real32 SecondsLeftUntilFlip = TargetSecondsPerFrame - FromBeginToAudioSeconds;
                DWORD ExpectedBytesUntilFlip = (DWORD)((SecondsLeftUntilFlip / TargetSecondsPerFrame) * (real32)ExpectedSoundBytesPerFrame);

                DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedBytesUntilFlip;
                DWORD SafeWriteCursor = WriteCursor;
                if (SafeWriteCursor < PlayCursor){
                  SafeWriteCursor += SoundOutput.SecondaryBufferSize;
                }
                Assert(SafeWriteCursor >= PlayCursor);
                SafeWriteCursor += SoundOutput.SafetyBytes;
                bool AudioCardIsLowLatency = (SafeWriteCursor < ExpectedFrameBoundaryByte);

                DWORD TargetCursor = 0;
                if (AudioCardIsLowLatency){
                  TargetCursor = ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame;
                }
                else {
                  TargetCursor = WriteCursor + ExpectedSoundBytesPerFrame + SoundOutput.SafetyBytes;
                }
                TargetCursor = TargetCursor % SoundOutput.SecondaryBufferSize;

                DWORD BytesToWrite = 0;
                if (BytesToLock > TargetCursor){
                  BytesToWrite = SoundOutput.SecondaryBufferSize - BytesToLock;
                  BytesToWrite += TargetCursor;
                }
                else{
                  BytesToWrite = TargetCursor - BytesToLock;
                }

                game_sound_output_buffer SoundBuffer = {};
                SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
                SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
                SoundBuffer.Samples = Samples;
                if (Game.GetSoundSamples)
                {
                  Game.GetSoundSamples(&Thread, &GameMemory, &SoundBuffer);
                }

                Win32FillSoundBuffer(&SoundOutput, BytesToLock, BytesToWrite, &SoundBuffer);

#if HANDMADE_INTERNAL
                win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
                Marker->OutputPlayCursor = PlayCursor;
                Marker->OutputWriteCursor = WriteCursor;
                Marker->OutputLocation = BytesToLock;
                Marker->OutputByteCount = BytesToWrite;
                Marker->ExpectedFlipPlayCursor = ExpectedFrameBoundaryByte;

                DWORD UnwrappedWriteCursor = WriteCursor;
                if (UnwrappedWriteCursor < PlayCursor){
                  UnwrappedWriteCursor += SoundOutput.SecondaryBufferSize;
                }
                AudioLatencyBytes = UnwrappedWriteCursor - PlayCursor;
                AudioLatencySeconds = ((real32)AudioLatencyBytes / (real32)SoundOutput.BytesPerSample) / (real32)SoundOutput.SamplesPerSecond;
#if 0
                char CharBuffer[256];
                sprintf_s(CharBuffer, "BTL:%u TC:%u BTW:%u - PC:%u WC:%u DELTA:%u (%fs)\n", BytesToLock, TargetCursor, BytesToWrite, PlayCursor, WriteCursor, AudioLatencyBytes, AudioLatencySeconds);
                OutputDebugStringA(CharBuffer);
#endif
#endif
              }
              else {
                SoundIsValid = false;
              }

              LARGE_INTEGER WorkCounter = Win32GetWallClock();
              real32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);

              //TODO: NOT TESTED YET
              real32 SecondsElapsedForFrame = WorkSecondsElapsed;
              if (SecondsElapsedForFrame < TargetSecondsPerFrame){
                if (SleepIsGranular){
                  DWORD SleepMS =  DWORD(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));
                  if (SleepMS > 0){
                    Sleep(SleepMS);
                  }
                }

                //TODO: This Assertion is not working
                real32 TestSecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                if (TestSecondsElapsedForFrame < TargetSecondsPerFrame){

                }
                while (SecondsElapsedForFrame < TargetSecondsPerFrame){
                  SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                }
              }
              else {
                //TODO: Missed frame rate
                //TODO: Logging
              }
              LARGE_INTEGER EndCounter = Win32GetWallClock();
              real32 MSPerFrame = 1000.0f * Win32GetSecondsElapsed(LastCounter, EndCounter);
              LastCounter = EndCounter;

              win32_window_dimension Dimension = Win32GetWindowDimension(Window);

              HDC DeviceContext = GetDC(Window);
              Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
              ReleaseDC(Window, DeviceContext);

              FlipWallClock = Win32GetWallClock();

#if HANDMADE_INTERNAL
              //NOTE: This is debug code
              {
                DWORD PlayCursor;
                DWORD WriteCursor;
                if (GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK){
                  Assert(DebugTimeMarkerIndex < ArrayCount(DebugTimeMarkers));
                  win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
                  Marker->FlipPlayCursor = PlayCursor;
                  Marker->FlipWriteCursor = WriteCursor;
                }
              }
#endif
              game_input *Temp = NewInput;
              NewInput = OldInput;
              OldInput = Temp;

#if 1
              uint64_t EndCycleCount = __rdtsc();
              uint64_t CycleElapsed = EndCycleCount - LastCycleCount;
              LastCycleCount = EndCycleCount;
              real32 FPS = (real32)(1000.0f / (real32)MSPerFrame);
              real32 MCPF = (real32)(CycleElapsed/(real32)(1000*1000));

              char CharBuffer[256];
              sprintf_s(CharBuffer, "%.02fms/f, %.02ff/s, %.02fmc/f\n", MSPerFrame, FPS, MCPF);
              OutputDebugStringA(CharBuffer);
#endif
#if HANDMADE_INTERNAL
              ++DebugTimeMarkerIndex;
              if (DebugTimeMarkerIndex >= ArrayCount(DebugTimeMarkers)){
                DebugTimeMarkerIndex = 0;
              }
#endif
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
