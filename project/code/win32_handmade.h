#ifndef WIN32_HANDMADE_H
#define WIN32_HANDMADE_H

struct win32_offscreen_buffer{
  //NOTE: pixels are always 32-bit wide, memory order: BBGGRRXX
  BITMAPINFO Info;
  void *Memory;
  int Width;
  int Height;
  int Pitch;
  int BytesPerPixel;
};

struct win32_window_dimension{
  int Width;
  int Height;
};

struct win32_sound_output{
  int SamplesPerSecond;
  uint32_t RunningSampleIndex;
  int BytesPerSample;
  int SecondaryBufferSize;
  real32 tSine;
  int LatencySampleCount;
};

struct win32_debug_time_marker{
  DWORD PlayCursor;
  DWORD WriteCursor;
};
#endif
