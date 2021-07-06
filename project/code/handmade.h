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

#define Kilobytes(Value) ((Value) * 1024)
#define Megabytes(Value) (Kilobytes(Value) * 1024)
#define Gigabytes(Value) (Megabytes(Value) * 1024)
#define Terabytes(Value) (Gigabytes(Value) * 1024)
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
//TODO: Swap, min, max... macros???

/*
  TODO: Services that the platform layer provides to the game
*/

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
  bool IsAnalog;

  real32 StartX;
  real32 StartY;

  real32 MinX;
  real32 MinY;

  real32 MaxX;
  real32 MaxY;

  real32 EndX;
  real32 EndY;

  union{
    game_button_state Buttons[6];
    struct{
      game_button_state Up;//Buttons[0]
      game_button_state Down;//Buttons[1]
      game_button_state Right;//Buttons[2]
      game_button_state Left;//Buttons[3]
      game_button_state LeftShoulder;//Buttons[4]
      game_button_state RightShoulder;//Buttons[5]
    };
  };
};

struct game_input{
  game_controller_input Controllers[4];
};

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
