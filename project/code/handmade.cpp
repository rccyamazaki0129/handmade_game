#include "handmade.h"
#include "handmade_intrinsics.h"
void GameOutputSound(game_state *GameState, game_sound_output_buffer *SoundBuffer, int ToneHz)
{
  int16_t ToneVolume = 6000;
  int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

  int16_t *SampleOut = SoundBuffer->Samples;

  for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
  {
#if 0
    real32 SineValue = sinf(GameState->tSine);
    int16_t SampleValue = (int16_t)(SineValue * ToneVolume);
#else
    int16_t SampleValue = 0;
#endif
    *SampleOut++ = SampleValue;
    *SampleOut++ = SampleValue;

#if 0
    GameState->tSine += 2.0f * Pi32 * 1.0f / (real32)WavePeriod;
    if (GameState->tSine > 2.0f*Pi32){
      GameState->tSine -= 2.0f*Pi32;
    }
#endif
  }
}

internal void DrawRectangle(game_offscreen_buffer *Buffer, real32 RealMinX, real32 RealMinY, real32 RealMaxX, real32 RealMaxY, real32 R, real32 G, real32 B)
{
  int32_t MinX = RoundReal32ToInt32(RealMinX);
  int32_t MinY = RoundReal32ToInt32(RealMinY);
  int32_t MaxX = RoundReal32ToInt32(RealMaxX);
  int32_t MaxY = RoundReal32ToInt32(RealMaxY);

  if (MinX < 0)
  {
    MinX = 0;
  }
  if (MinY < 0)
  {
    MinY = 0;
  }
  if (MaxX > Buffer->Width)
  {
    MaxX = Buffer->Width;
  }
  if (MaxY > Buffer->Height)
  {
    MaxY = Buffer->Height;
  }
  //BIT PATTERN: 0x AA RR GG BB
  uint32_t Color = (RoundReal32ToInt32(R * 255.0f) << 16) | (RoundReal32ToInt32(G * 255.0f) << 8) | (RoundReal32ToInt32(B * 255.0f));

  uint8_t *EndOfBuffer = (uint8_t*)Buffer->Memory + Buffer->Pitch*Buffer->Height;

  uint8_t *Row = ((uint8_t *)(Buffer->Memory) + MinX*Buffer->BytesPerPixel + MinY*Buffer->Pitch);

  for (int Y = MinY; Y < MaxY; ++Y)
  {
    uint32_t *Pixel = (uint32_t*)Row;
      for (int X = MinX; X < MaxX; ++X)
      {
          *Pixel++ = Color;
      }
      Row += Buffer->Pitch;
  }
}

inline tile_chunk* GetTileChunk(world *World, int32_t TileChunkX, int32_t TileChunkY)
{
  tile_chunk *TileChunk = 0;

  if ((TileChunkX >= 0) && (TileChunkX < World->TileChunkCountX) && (TileChunkY >= 0) && (TileChunkY < World->TileChunkCountY))
  {
    TileChunk = &World->TileChunks[TileChunkY * World->TileChunkCountX + TileChunkX];
  }
  return TileChunk;
}

inline uint32_t GetTileValueUnchecked(world *World, tile_chunk *TileChunk, uint32_t TileX, uint32_t TileY)
{
  Assert(TileChunk);
  Assert((TileX < World->ChunkDim) && (TileY < World->ChunkDim));
  uint32_t TileChunkValue = TileChunk->Tiles[TileY * World->ChunkDim + TileX];
  return TileChunkValue;
}

internal uint32_t GetTileValue(world *World, tile_chunk *TileChunk, uint32_t TestTileX, uint32_t TestTileY)
{
  uint32_t TileChunkValue = 0;
  if (TileChunk)
  {
    TileChunkValue = GetTileValueUnchecked(World, TileChunk, TestTileX, TestTileY);
  }
  return TileChunkValue;
}

inline void RecanonicalizeCoord(world *World, uint32_t *Tile, real32 *TileRel)
{
  //TODO: Need to do something that doesn't use the divide/multiply method for recanonicalizing because this can end up rounding back on to the tile you just came from.

  //NOTE:  World is assumed to be toroidal, if you step off one end you come back on the other
  int32_t Offset = FloorReal32ToInt32(*TileRel / World->TileSideInMeters);
  *Tile += Offset;
  *TileRel -= Offset * World->TileSideInMeters;

  //TODO: Fix floating point math so this can be <
  Assert(*TileRel >= 0);
  Assert(*TileRel <= World->TileSideInMeters);

}

inline world_position RecanonicalizePosition(world *World, world_position Pos)
{
  world_position Result = Pos;

  RecanonicalizeCoord(World, &Result.AbsTileX, &Result.TileRelX);
  RecanonicalizeCoord(World, &Result.AbsTileY, &Result.TileRelY);

  return Result;
}

inline tile_chunk_position GetChunkPositionFor(world *World, uint32_t AbsTileX, uint32_t AbsTileY)
{
  tile_chunk_position Result;
  Result.TileChunkX = AbsTileX >> World->ChunkShift;
  Result.TileChunkY = AbsTileY >> World->ChunkShift;
  Result.RelTileX = AbsTileX & World->ChunkMask;
  Result.RelTileY = AbsTileY & World->ChunkMask;
  return Result;
}

internal uint32_t GetTileValue(world *World, uint32_t AbsTileX, uint32_t AbsTileY)
{
  bool Empty = false;

  tile_chunk_position ChunkPos = GetChunkPositionFor(World, AbsTileX, AbsTileY);
  tile_chunk *TileChunk = GetTileChunk(World, ChunkPos.TileChunkX, ChunkPos.TileChunkY);
  uint32_t TileChunkValue = GetTileValue(World, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY);

  return TileChunkValue;
}

internal bool IsWorldPointEmpty(world *World, world_position CanPos)
{

  uint32_t TileChunkValue = GetTileValue(World, CanPos.AbsTileX, CanPos.AbsTileY);
  bool Empty = (TileChunkValue == 0);
  return Empty;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == (ArrayCount(Input->Controllers[0].Buttons)));
  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

#define TILE_MAP_COUNT_X 256
#define TILE_MAP_COUNT_Y 256
  uint32_t TempTiles[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
  {
    {1, 1, 1, 1,   1, 1, 1, 1,   1,   1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1,   1,   1, 1, 1, 1,   1, 1, 1, 1},
    {1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 1, 0, 1, 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 0, 0, 0,   0, 1, 0, 0,   0,   1, 0, 1, 1,   0, 0, 0, 1, 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 0, 0, 0,   0, 0, 1, 0,   0,   0, 0, 0, 0,   0, 0, 1, 1, 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 0, 0, 0,   0, 0, 1, 0,   1,   0, 1, 1, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 0, 1, 0,   1, 0, 0, 0,   0,   0, 0, 0, 0,   0, 1, 0, 1, 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 0, 1, 0,   0, 0, 0, 0,   0,   0, 1, 0, 0,   0, 0, 0, 1, 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 1, 0,   0, 0, 1, 1, 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 1, 1, 1,   1, 1, 1, 1,   0,   1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1,   0,   1, 1, 1, 1,   1, 1, 1, 1},
    {1, 1, 1, 1,   1, 1, 1, 1,   0,   1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1,   0,   1, 1, 1, 1,   1, 1, 1, 1},
    {1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1, 1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 1, 1, 1,   1, 1, 1, 1,   1,   1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1,   1,   1, 1, 1, 1,   1, 1, 1, 1},
  };

  world World;
  //NOTE: This is set to using 256x256 tile chunks.
  World.ChunkShift = 8;
  World.ChunkMask = 0x000000FF;
  World.ChunkDim = 256;

  World.TileChunkCountX = 1;
  World.TileChunkCountY = 1;

  tile_chunk TileChunk;
  TileChunk.Tiles = (uint32_t *)TempTiles;
  World.TileChunks = &TileChunk;

  //TODO: Begin using tile side in meters
  World.TileSideInMeters = 1.4f;
  World.TileSideInPixles = 60;
  World.MetersToPixels = (real32)World.TileSideInPixles / (real32)World.TileSideInMeters;

  real32 PlayerHeight = 1.4f;
  real32 PlayerWidth = 0.75f * PlayerHeight;

  real32 LowerLeftX = -(real32)World.TileSideInPixles / 2;
  real32 LowerLeftY = (real32)Buffer->Height;

  game_state *GameState = (game_state *)Memory->PermanentStorage;
  if (!Memory->IsInitialized)
  {
    GameState->PlayerP.AbsTileX = 2;
    GameState->PlayerP.AbsTileY = 2;
    GameState->PlayerP.TileRelX = 8.0f;
    GameState->PlayerP.TileRelY = 8.0f;

    Memory->IsInitialized = true;
  }

  //NOTE: Input0 is usually Keyboard
  for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex)
  {
    game_controller_input *Controller = GetController(Input, ControllerIndex);
    if (Controller->IsAnalog)
    {
      //NOTE: use analog movement tuning
      real32 dPlayerX = 0.0f; //pix/sec
      real32 dPlayerY = 0.0f; //pix/sec
      real32 PadAdjust = 1.0f;
      if (Controller->StickAverageX < 0)
      {
        dPlayerX += (real32)(4.0f*PadAdjust*(Controller->StickAverageX));
        PadAdjust = 1.5f;
      }
      else
      {
        dPlayerX += (real32)(3.0f*PadAdjust*(Controller->StickAverageX));
      }

      if (Controller->StickAverageY < 0)
      {
        dPlayerY += (real32)(-4.0f*PadAdjust*(Controller->StickAverageY));
      }
      else
      {
        PadAdjust = 1.0f;
        dPlayerY += (real32)(-3.0f*PadAdjust*(Controller->StickAverageY));
      }
      dPlayerX *= 30.0f;
      dPlayerY *= 30.0f;
      GameState->PlayerP.TileRelX += Input->dtForFrame*dPlayerX;
      GameState->PlayerP.TileRelY += Input->dtForFrame*dPlayerY;
    }
    else
    {
      //NOTE: use digital movement tuning
      real32 dPlayerX = 0.0f; //pix/sec
      real32 dPlayerY = 0.0f; //pix/sec

      if (Controller->MoveUp.EndedDown)
      {
        dPlayerY = 1.0f;
      }
      if (Controller->MoveDown.EndedDown)
      {
        dPlayerY = -1.0f;
      }
      if (Controller->MoveLeft.EndedDown)
      {
        dPlayerX = -1.0f;
      }
      if (Controller->MoveRight.EndedDown)
      {
        dPlayerX = 1.0f;
      }
      dPlayerX *= 4.0f;
      dPlayerY *= 4.0f;
      //TODO: Diagonal will be faster. Fix once we have vector.
      world_position NewPlayerP = GameState->PlayerP;
      NewPlayerP.TileRelX += Input->dtForFrame*dPlayerX;
      NewPlayerP.TileRelY += Input->dtForFrame*dPlayerY;
      NewPlayerP = RecanonicalizePosition(&World, NewPlayerP);
      //TODO: Delta function that auto-recanonicalizes

      world_position PlayerLeft = NewPlayerP;
      PlayerLeft.TileRelX -= 0.5f * PlayerWidth;
      PlayerLeft = RecanonicalizePosition(&World, PlayerLeft);

      world_position PlayerRight = NewPlayerP;
      PlayerRight.TileRelX += 0.5f * PlayerWidth;
      PlayerRight = RecanonicalizePosition(&World, PlayerRight);

      if (IsWorldPointEmpty(&World, NewPlayerP)
          && IsWorldPointEmpty(&World, PlayerLeft)
          && IsWorldPointEmpty(&World, PlayerRight))
      {
            GameState->PlayerP = NewPlayerP;
      }
    }
  }

  DrawRectangle(Buffer, 0.0f, 0.0f, (real32)Buffer->Width, (real32)Buffer->Height, 1.0f, 0.0f, 1.0f);

  real32 CenterX = 0.5f * (real32)Buffer->Width;
  real32 CenterY = 0.5f * (real32)Buffer->Height;
  for (int32_t RelRow = -10; RelRow < 10; ++RelRow)
  {
    for (int32_t RelColumn = -20; RelColumn < 20; ++RelColumn)
    {
      uint32_t Column = GameState->PlayerP.AbsTileX + RelColumn;
      uint32_t Row = GameState->PlayerP.AbsTileY + RelRow;
      uint32_t TileID = GetTileValue(&World, Column, Row);
      real32 Gray = 0.3f;
      if (TileID)
      {
        Gray = 0.8f;
      }

      if ((Column == GameState->PlayerP.AbsTileX) && (Row == GameState->PlayerP.AbsTileY))
      {
        Gray = 0.1f;
      }

      real32 MinX = CenterX + ((real32)RelColumn) * World.TileSideInPixles;
      real32 MinY = CenterY - ((real32)RelRow) * World.TileSideInPixles;
      real32 MaxX = MinX + World.TileSideInPixles;
      real32 MaxY = MinY - World.TileSideInPixles;
      DrawRectangle(Buffer, MinX, MaxY, MaxX, MinY, Gray, Gray, Gray);
    }
  }

  real32 PlayerR = 0.2f;
  real32 PlayerG = 0.8f;
  real32 PlayerB = 0.2f;

  real32 PlayerLeft = CenterX + World.MetersToPixels * GameState->PlayerP.TileRelX - 0.5f * World.MetersToPixels * PlayerWidth;
  real32 PlayerTop = CenterY - World.MetersToPixels * GameState->PlayerP.TileRelY - World.MetersToPixels * PlayerHeight;

  DrawRectangle(Buffer, PlayerLeft, PlayerTop, PlayerLeft + World.MetersToPixels * PlayerWidth, PlayerTop + World.MetersToPixels * PlayerHeight, PlayerR, PlayerG, PlayerB);

}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
  game_state *GameState = (game_state *)Memory->PermanentStorage;
  GameOutputSound(GameState, SoundBuffer, 400);
}

/*
internal void RenderWeirdGradient(game_offscreen_buffer *Buffer, int XOffset, int YOffset)
{
  //TODO: Let's see what the optimizer does
  uint8_t *Row = (uint8_t *)Buffer->Memory;

  for (int y = 0; y < Buffer->Height; y++){

    uint32_t *Pixel = (uint32_t *)Row;

    for (int x = 0; x < Buffer->Width; x++){

        // Pixel in memory: 00 00 00 00
                         // BB GG RR XX
        // 0x XXRRGGBB

      uint8_t Blue = (uint8_t)(x + XOffset);
      uint8_t Green = (uint8_t)(y + YOffset);
      uint8_t Red = uint8_t((x + y) / 16);

      *Pixel++ = ((Red << 16) | (Green << 8) | Blue);
    }
    Row += Buffer->Pitch;
  }
}
*/
