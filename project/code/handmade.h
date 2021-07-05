#ifndef HANDMADE_H
#define HANDMADE_H

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

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer, int XOffset, int YOffset);

#endif
