#ifndef HANDMADE_H
#define HANDMADE_H
/*
  NOTE:
  HANDMADE_INTERNAL: 0 - Build for public release
                     1 - Build for developer only
  HANDMADE_SLOW:     0 - Not Slow code allowed!
                     1 - Slow code welcome.
*/

#if HANDMADE_SLOW
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}//if this is true, program will crash
#else
#define Assert(Expression)
#endif

#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)
#define Terabytes(Value) (Gigabytes(Value) * 1024LL)
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
//TODO: Swap, min, max... macros???

inline uint32_t SafeTruncateSizeUInt64(uint64_t Value){
  //TODO: Defines for maximum values uint32Max
  Assert(Value <= 0xFFFFFFFF);
  uint32_t Result = (uint32_t)Value;
  return Result;
}

/*
  TODO: Services that the platform layer provides to the game
*/
#if HANDMADE_INTERNAL
/*
  IMPORTANT:
  These are NOT for doing anything in the shipping version
  - they are blocking and the write doesn't protect against lost data
*/
struct debug_read_file_result{
  uint32_t ContentsSize;
  void *Contents;
};
internal debug_read_file_result DEBUGPlatformReadEntireFile(char *FileName);
internal void DEBUGPlatformFreeFileMemory(void *BitmapMemory);
internal bool DEBUGPlatformWriteEntireFile(char *FileName, uint64_t MemorySize, void *Memory);
#endif
/*
  TODO: Services that the game provides to the platform layer
  (This may expand in the future - sound on separate thread, etc...)
*/
// 4 things - timing, controller/keyboard input, bitmap buffer to use, sound buffer to use
// TODO: In the future, rendering _specifically_ will become a three-tiered abstraction!!
struct game_offscreen_buffer{
  //NOTE: pixels are always 32-bit wide, memory order: BBGGRRXX
  void *Memory;
  int Width;
  int Height;
  int Pitch;
};

struct game_sound_output_buffer{
  int SamplesPerSecond;
  int SampleCount;
  int16_t * Samples;
};

struct game_button_state{
  int HalfTransitionCount;
  bool EndedDown;
};

struct game_controller_input{
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
    };
  };
};

struct game_input{
  //TODO: Insert clock value here.
  game_controller_input Controllers[5];
};

inline game_controller_input *GetController(game_input *Input, int ControllerIndex){
  Assert(ControllerIndex < ArrayCount(Input->Controllers));
  game_controller_input *Result = &Input->Controllers[ControllerIndex];
  return Result;
}

struct game_memory{
  bool IsInitialized;
  uint64_t PermanentStorageSize;
  void *PermanentStorage;//NOTE: REQUIRED to be cleared to zero at startup
  uint64_t TransientStorageSize;
  void *TransientStorage;//NOTE: REQUIRED to be cleared to zero at startup
};

internal void GameUpdateAndRender(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer, game_sound_output_buffer *SoundBuffer);

//
//
//
struct game_state{
  int ToneHz;
  int XOffset;
  int YOffset;
};

#endif
