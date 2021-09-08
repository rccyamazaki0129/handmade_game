#ifndef HANDMADE_H
#define HANDMADE_H

#include "handmade_platform.h"

#define Pi32 3.14159265359f


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

#include "handmade_intrinsics.h"
#include "handmade_tile.h"
#include "handmade_math.h"

#define Minimum(A, B) ((A < B) ? (A) : (B))
#define Maximum(A, B) ((A > B) ? (A) : (B))

struct memory_arena
{
  memory_index Size;
  uint8_t *Base;
  memory_index Used;
};

internal void InitializeArena(memory_arena *Arena, memory_index Size, uint8_t *Base)
{
  Arena->Size = Size;
  Arena->Base = Base;
  Arena->Used = 0;
}

#define PushStruct(Arena, type) (type *)PushSize_(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type *)PushSize_(Arena, (Count)*sizeof(type))
void *PushSize_(memory_arena *Arena, memory_index Size)
{
  Assert((Arena->Used + Size) <= Arena->Size);
  void *Result = Arena->Base + Arena->Used;
  Arena->Used += Size;
  return Result;
}


struct world
{
  tile_map *TileMap;
};

struct loaded_bitmap
{
  int32_t Width;
  int32_t Height;
  uint32_t *Pixels;
};

struct hero_bitmaps
{
  int32_t AlignX;
  int32_t AlignY;
  loaded_bitmap Character;
};

struct entity
{
  bool Exists;
  tile_map_position P;
  v2 dP;
  uint32_t FacingDirection;
  real32 Width;
  real32 Height;
};

struct game_state
{
  memory_arena WorldArena;
  world *World;

  //TODO: Should we allow split-screen?
  uint32_t CameraFollowingEntityIndex;
  tile_map_position CameraP;

  uint32_t PlayerIndexForController[ArrayCount(((game_input *)0)->Controllers)];
  uint32_t EntityCount;
  entity Entities[256];

  loaded_bitmap Backdrop;
  hero_bitmaps HeroBitmaps[4];
};

#endif
