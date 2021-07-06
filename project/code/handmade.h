#ifndef HANDMADE_H
#define HANDMADE_H

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

internal void GameUpdateAndRender(game_input *Input, game_offscreen_buffer *Buffer, game_sound_output_buffer *SoundBuffer);

#endif
