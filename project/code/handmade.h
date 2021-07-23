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
  int32_t TileMapX;
  int32_t TileMapY;

  int32_t TileX;
  int32_t TileY;
  //NOTE: This is tile-relative X and Y
  real32 X;
  real32 Y;
};

struct raw_position
{
  int32_t TileMapX;
  int32_t TileMapY;
  //NOTE:  This is tile-map relative X and Y
  real32 X;
  real32 Y;
};

struct tile_map
{
  uint32_t *Tiles;
};

struct world
{
  int32_t CountX;
  int32_t CountY;
  real32 UpperLeftX;
  real32 UpperLeftY;
  real32 TileWidth;
  real32 TileHeight;

  int32_t TileMapCountX;
  int32_t TileMapCountY;
  tile_map *TileMaps;
};

struct game_state
{
  int32_t PlayerTileMapX;
  int32_t PlayerTileMapY;

  real32 PlayerX;
  real32 PlayerY;
};

#endif
