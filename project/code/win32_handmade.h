#ifndef WIN32_HANDMADE_H
#define WIN32_HANDMADE_H

struct win32_offscreen_buffer
{
  //NOTE: pixels are always 32-bit wide, memory order: BBGGRRXX
  BITMAPINFO Info;
  void *Memory;
  int Width;
  int Height;
  int Pitch;
  int BytesPerPixel;
};

struct win32_window_dimension
{
  int Width;
  int Height;
};

struct win32_sound_output
{
  int SamplesPerSecond;
  uint32_t RunningSampleIndex;
  int BytesPerSample;
  DWORD SecondaryBufferSize;
  DWORD SafetyBytes;
  //TODO: Should Running sample index be in bytes as well
  //TODO: Math gets simpler if we add a "Bytes per second" field?
};

struct win32_debug_time_marker
{
  DWORD OutputPlayCursor;
  DWORD OutputWriteCursor;
  DWORD OutputLocation;
  DWORD OutputByteCount;
  DWORD ExpectedFlipPlayCursor;
  DWORD FlipPlayCursor;
  DWORD FlipWriteCursor;
};

struct win32_game_code{
  HMODULE GameCodeDLL;
  FILETIME DLLLastWriteTime;
  //IMPORTANT: Either of the callbacks can be 0. You must check before calling
  game_update_and_render *UpdateAndRender;
  game_get_sound_samples *GetSoundSamples;
  bool IsValid;
};

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH

struct win32_replay_buffer
{
  HANDLE FileHandle;
  HANDLE MemoryMap;
  char FileName[WIN32_STATE_FILE_NAME_COUNT];
  void *MemoryBlock;
};

struct win32_state
{
  int64_t TotalSize;
  void* GameMemoryBlock;
  win32_replay_buffer ReplayBuffers[4];

  HANDLE RecordingHandle;
  int InputRecordingIndex;

  HANDLE PlayBackHandle;
  int InputPlayingIndex;


  char EXEFileName[WIN32_STATE_FILE_NAME_COUNT];
  char* OnePastLastEXEFileNameSlash;
};

#endif
