#ifndef HANDMADE_PLATFORM_H
#define HANDMADE_PLATFORM_H

/*
  NOTE:
  HANDMADE_INTERNAL: 0 - Build for public release
                     1 - Build for developer only
  HANDMADE_SLOW:     0 - Not Slow code allowed!
                     1 - Slow code welcome.
*/

#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

typedef float real32;
typedef double real64;
typedef size_t memory_index;

struct thread_context
{
  int Placeholder;
};

/*
  TODO: Services that the platform layer provides to the game
*/
#if HANDMADE_INTERNAL
/*
  IMPORTANT:
  These are NOT for doing anything in the shipping version
  - they are blocking and the write doesn't protect against lost data
*/
struct debug_read_file_result
{
  uint32_t ContentsSize;
  void *Contents;
};

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context *Thread, void *Memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(thread_context *Thread, char *FileName)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool name(thread_context *Thread, char *FileName, uint64_t MemorySize, void *Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

#endif
/*
  TODO: Services that the game provides to the platform layer
  (This may expand in the future - sound on separate thread, etc...)
*/
// 4 things - timing, controller/keyboard input, bitmap buffer to use, sound buffer to use
// TODO: In the future, rendering _specifically_ will become a three-tiered abstraction!!
struct game_offscreen_buffer
{
  //NOTE: pixels are always 32-bit wide, memory order: BBGGRRXX
  void *Memory;
  int Width;
  int Height;
  int Pitch;
  int BytesPerPixel;
};

struct game_sound_output_buffer
{
  int SamplesPerSecond;
  int SampleCount;
  int16_t * Samples;
};

struct game_button_state
{
  int HalfTransitionCount;
  bool EndedDown;
};

struct game_controller_input
{
  bool IsConnected;
  bool IsAnalog;
  real32 StickAverageX;
  real32 StickAverageY;

  union{
    game_button_state Buttons[12];
    struct{
      game_button_state MoveUp;
      game_button_state MoveDown;
      game_button_state MoveRight;
      game_button_state MoveLeft;

      game_button_state ActionUp;
      game_button_state ActionDown;
      game_button_state ActionRight;
      game_button_state ActionLeft;

      game_button_state LeftShoulder;
      game_button_state RightShoulder;

      game_button_state Back;
      game_button_state Start;

      //NOTE: All buttons must be added above this line
      game_button_state Terminator;
    };
  };
};

struct game_input
{
  game_button_state MouseButtons[5];
  int32_t MouseX, MouseY, MouseZ;

  real32 dtForFrame;
  game_controller_input Controllers[5];
};

struct game_memory
{
  bool IsInitialized;
  uint64_t PermanentStorageSize;
  void *PermanentStorage;//NOTE: REQUIRED to be cleared to zero at startup
  uint64_t TransientStorageSize;
  void *TransientStorage;//NOTE: REQUIRED to be cleared to zero at startup

  debug_platform_free_file_memory *DEBUGPlatformFreeFileMemory;
  debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;
  debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile;
};

#pragma pack(push, 1)
struct bitmap_header
{
  uint16_t FileType;
  uint32_t FileSize;
  uint16_t Reserved1;
  uint16_t Reserved2;
  uint32_t BitmapOffset;
  uint32_t Size;
  int32_t Width;
  int32_t Height;
  uint16_t Planes;
  uint16_t BitsPerPixel;
  uint32_t Compression;
  uint32_t SizeofBitmap;
  int32_t HorzResolution;
  int32_t VertResolution;
  uint32_t ColorsUsed;
  uint32_t ColorsImportant;

  uint32_t RedMask;
  uint32_t GreenMask;
  uint32_t BlueMask;
};
#pragma pack(pop)

#define GAME_UPDATE_AND_RENDER(name) void name(thread_context *Thread, game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

//NOTE: At the moment, this has to be a very fast function, it cannot be more than a millisecond or so.
//TODO: Reduce the pressure on this function's performance by measuring it or asking about it, etc...
#define GAME_GET_SOUND_SAMPLES(name) void name(thread_context *Thread, game_memory *Memory, game_sound_output_buffer *SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

#endif
