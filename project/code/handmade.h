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

struct tile_chunk_position
{
  uint32_t TileChunkX;
  uint32_t TileChunkY;

  uint32_t RelTileX;
  uint32_t RelTileY;
};

struct world_position
{
  /*
  TODO: Take the tile map x, y and the tile x, y and pack them into single 32 bit values for x, y where there is some low bits for the tile index, and the high bits are for the tile page
  */
  uint32_t AbsTileX;
  uint32_t AbsTileY;

  //TODO: Should these be from the center of a tile?
  real32 TileRelX;
  real32 TileRelY;
};

struct tile_chunk
{
  uint32_t *Tiles;
};

struct world
{
  uint32_t ChunkShift;
  uint32_t ChunkMask;
  uint32_t ChunkDim;

  real32 TileSideInMeters;
  int32_t TileSideInPixles;
  real32 MetersToPixels;

  int32_t TileChunkCountX;
  int32_t TileChunkCountY;

  tile_chunk *TileChunks;
};

struct game_state
{
  world_position PlayerP;
};

#endif
