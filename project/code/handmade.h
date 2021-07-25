#ifndef HANDMADE_H
#define HANDMADE_H

#include "handmade_platform.h"

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f


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

inline uint32_t SafeTruncateSizeUInt64(uint64_t Value)
{
  //TODO: Defines for maximum values uint32Max
  Assert(Value <= 0xFFFFFFFF);
  uint32_t Result = (uint32_t)Value;
  return Result;
}

inline game_controller_input *GetController(game_input *Input, int ControllerIndex)
{
  Assert(ControllerIndex < ArrayCount(Input->Controllers));
  game_controller_input *Result = &Input->Controllers[ControllerIndex];
  return Result;
}
//
//
//

struct canonical_position
{
  /*
  TODO: Take the tile map x, y and the tile x, y and pack them into single 32 bit values for x, y where there is some low bits for the tile index, and the high bits are for the tile page
  */
#if 1
  int32_t TileMapX;
  int32_t TileMapY;

  int32_t TileX;
  int32_t TileY;
#else
  uint32_t _TileX;
  uint32_t _TileY;
#endif
  //TODO: Should these be from the center of a tile?
  real32 TileRelX;
  real32 TileRelY;
};

struct tile_map
{
  uint32_t *Tiles;
};

struct world
{
  real32 TileSideInMeters;
  int32_t TileSideInPixles;
  real32 MetersToPixels;
  int32_t CountX;
  int32_t CountY;
  real32 UpperLeftX;
  real32 UpperLeftY;

  int32_t TileMapCountX;
  int32_t TileMapCountY;
  tile_map *TileMaps;
};

struct game_state
{
  canonical_position PlayerP;
};

#endif
