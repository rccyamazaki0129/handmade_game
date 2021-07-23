#include "handmade.h"

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

inline int32_t RoundReal32ToInt32(real32 Real32)
{
  int32_t Result = (int32_t)(Real32 + 0.5f);
  //TODO: Intrinsic???
  return Result;
}

inline uint32_t RoundReal32ToUInt32(real32 Real32)
{
  uint32_t Result = (uint32_t)(Real32 + 0.5f);
  //TODO: Intrinsic???
  return Result;
}

#include "math.h"
inline uint32_t FloorReal32ToInt32(real32 Real32)
{
  uint32_t Result = (uint32_t)floorf(Real32);
  return Result;
}

inline uint32_t TruncateReal32ToInt32(real32 Real32)
{
  uint32_t Result = (uint32_t)(Real32);
  return Result;
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

inline tile_map* GetTileMap(world *World, int32_t TileMapX, int32_t TileMapY)
{
  tile_map *TileMap = 0;

  if ((TileMapX >= 0) && (TileMapX < World->TileMapCountX) && (TileMapY >= 0) && (TileMapY < World->TileMapCountY))
  {
    TileMap = &World->TileMaps[TileMapY * World->TileMapCountX + TileMapX];
  }
  return TileMap;
}

inline uint32_t GetTileValueUnchecked(world *World, tile_map *TileMap, int32_t TileX, int32_t TileY)
{
  Assert(TileMap);
  Assert((TileX >= 0) && (TileX < World->CountX) && (TileY >= 0) && (TileY < World->CountY));
  uint32_t TileMapValue = TileMap->Tiles[TileY * World->CountX + TileX];
  return TileMapValue;
}

internal bool IsTileMapPointEmpty(world *World, tile_map *TileMap, int32_t TestTileX, int32_t TestTileY)
{
  bool Empty = false;
  if (TileMap)
  {
    if ((TestTileX >= 0) && (TestTileX < World->CountX) && (TestTileY >= 0) && (TestTileY < World->CountY))
    {
      uint32_t TileMapValue = GetTileValueUnchecked(World, TileMap, TestTileX, TestTileY);
      Empty = (TileMapValue == 0);
    }
  }
  return Empty;
}


inline canonical_position GetCanonicalPosition(world *World, raw_position Pos)
{
  canonical_position Result;

  Result.TileMapX = Pos.TileMapX;
  Result.TileMapY = Pos.TileMapY;
  real32 X = Pos.X - World->UpperLeftX;
  real32 Y = Pos.Y - World->UpperLeftY;
  Result.TileX = FloorReal32ToInt32(X / World->TileWidth);
  Result.TileY = FloorReal32ToInt32(Y / World->TileHeight);
  Result.X = X - Result.TileX * World->TileWidth;
  Result.Y = Y - Result.TileY * World->TileHeight;

  Assert(Result.X >= 0);
  Assert(Result.Y >= 0);
  Assert(Result.X < World->TileWidth);
  Assert(Result.X < World->TileHeight);

  if (Result.TileX < 0)
  {
    Result.TileX = World->CountX + Result.TileX;
    --Result.TileMapX;
  }
  if (Result.TileY < 0)
  {
    Result.TileY = World->CountY + Result.TileY;
    --Result.TileMapY;
  }

  if (Result.TileX >= World->CountX)
  {
    Result.TileX = Result.TileX - World->CountX;
    ++Result.TileMapX;
  }
  if (Result.TileY >= World->CountY)
  {
    Result.TileY = Result.TileY - World->CountY;
    ++Result.TileMapY;
  }
  return Result;
}

internal bool IsWorldPointEmpty(world *World, raw_position TestPos)
{
  bool Empty = false;

  canonical_position CanPos = GetCanonicalPosition(World, TestPos);


  tile_map *TileMap = GetTileMap(World, CanPos.TileMapX, CanPos.TileMapY);
  Empty = IsTileMapPointEmpty(World, TileMap, CanPos.TileX, CanPos.TileY);

  return Empty;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == (ArrayCount(Input->Controllers[0].Buttons)));
  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

#define TILE_MAP_COUNT_X 17
#define TILE_MAP_COUNT_Y 9
  uint32_t Tiles00[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
  {
    {1, 1, 1, 1,   1, 1, 1, 1,   1,   1, 1, 1, 1,   1, 1, 1, 1},
    {1, 1, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 1, 0, 1},
    {1, 0, 0, 1,   0, 1, 0, 0,   0,   1, 0, 1, 1,   0, 0, 0, 1},
    {1, 0, 1, 0,   0, 0, 1, 0,   0,   0, 0, 0, 0,   0, 0, 1, 1},
    {1, 0, 0, 0,   0, 0, 1, 0,   1,   0, 1, 1, 0,   0, 0, 0, 0},
    {1, 0, 1, 0,   1, 0, 0, 0,   0,   0, 0, 0, 0,   0, 1, 0, 1},
    {1, 0, 1, 0,   0, 0, 0, 0,   0,   0, 1, 0, 0,   0, 0, 0, 1},
    {1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 1, 0,   0, 0, 1, 1},
    {1, 1, 1, 1,   1, 1, 1, 1,   0,   1, 1, 1, 1,   1, 1, 1, 1},
  };

  uint32_t Tiles01[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
  {
    {1, 1, 1, 1,   1, 1, 1, 1,   0,   1, 1, 1, 1,   1, 1, 1, 1},
    {1, 1, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 1, 1, 1,   1, 1, 1, 1,   1,   1, 1, 1, 1,   1, 1, 1, 1},
  };

  uint32_t Tiles10[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
  {
    {1, 1, 1, 1,   1, 1, 1, 1,   1,   1, 1, 1, 1,   1, 1, 1, 1},
    {1, 1, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {0, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 1, 1, 1,   1, 1, 1, 1,   0,   1, 1, 1, 1,   1, 1, 1, 1},
  };

  uint32_t Tiles11[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
  {
    {1, 1, 1, 1,   1, 1, 1, 1,   0,   1, 1, 1, 1,   1, 1, 1, 1},
    {1, 1, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {0, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 0, 0, 1},
    {1, 1, 1, 1,   1, 1, 1, 1,   1,   1, 1, 1, 1,   1, 1, 1, 1},
  };

  tile_map TileMaps[2][2];


  TileMaps[0][0].Tiles = (uint32_t *)Tiles00;
  TileMaps[0][1].Tiles = (uint32_t *)Tiles10;
  TileMaps[1][0].Tiles = (uint32_t *)Tiles01;
  TileMaps[1][1].Tiles = (uint32_t *)Tiles11;

  world World;
  World.TileMapCountX = 2;
  World.TileMapCountY = 2;
  World.TileMaps = (tile_map *)TileMaps;

  World.CountX = TILE_MAP_COUNT_X;
  World.CountY = TILE_MAP_COUNT_Y;
  World.UpperLeftX = -30;
  World.UpperLeftY = 0;
  World.TileWidth = 60;
  World.TileHeight = 60;

  real32 PlayerWidth = 0.75f * World.TileWidth;
  real32 PlayerHeight = World.TileHeight;


  game_state *GameState = (game_state *)Memory->PermanentStorage;
  if (!Memory->IsInitialized)
  {
    GameState->PlayerX = 400;
    GameState->PlayerY = 150;

    Memory->IsInitialized = true;
  }

  tile_map *TileMap = GetTileMap(&World, GameState->PlayerTileMapX, GameState->PlayerTileMapY);
  Assert(TileMap);

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
      GameState->PlayerX += Input->dtForFrame*dPlayerX;
      GameState->PlayerY += Input->dtForFrame*dPlayerY;
    }
    else
    {
      //NOTE: use digital movement tuning
      real32 dPlayerX = 0.0f; //pix/sec
      real32 dPlayerY = 0.0f; //pix/sec

      if (Controller->MoveUp.EndedDown)
      {
        dPlayerY = -1.0f;
      }
      if (Controller->MoveDown.EndedDown)
      {
        dPlayerY = 1.0f;
      }
      if (Controller->MoveLeft.EndedDown)
      {
        dPlayerX = -1.0f;
      }
      if (Controller->MoveRight.EndedDown)
      {
        dPlayerX = 1.0f;
      }
      dPlayerX *= 120.0f;
      dPlayerY *= 120.0f;
      //TODO: Diagonal will be faster. Fix once we have vector.
      real32 NewPlayerX = GameState->PlayerX + Input->dtForFrame*dPlayerX;
      real32 NewPlayerY = GameState->PlayerY + Input->dtForFrame*dPlayerY;

      raw_position PlayerPos = {GameState->PlayerTileMapX, GameState->PlayerTileMapY, NewPlayerX, NewPlayerY};
      raw_position PlayerLeft = PlayerPos;
      PlayerLeft.X -= 0.5f * PlayerWidth;
      raw_position PlayerRight = PlayerPos;
      PlayerRight.X += 0.5f * PlayerWidth;

      if (IsWorldPointEmpty(&World, PlayerPos) &&
          IsWorldPointEmpty(&World, PlayerLeft) &&
          IsWorldPointEmpty(&World, PlayerRight))
      {
        canonical_position CanPos = GetCanonicalPosition(&World, PlayerPos);
        GameState->PlayerTileMapX = CanPos.TileMapX;
        GameState->PlayerTileMapY = CanPos.TileMapY;
        GameState->PlayerX = World.UpperLeftX + World.TileWidth * CanPos.TileX + CanPos.X;
        GameState->PlayerY = World.UpperLeftY + World.TileHeight * CanPos.TileY + CanPos.Y;
      }
    }
  }

  DrawRectangle(Buffer, 0.0f, 0.0f, (real32)Buffer->Width, (real32)Buffer->Height, 1.0f, 0.0f, 1.0f);

  for (int Row = 0; Row < 9; ++Row)
  {
    for (int Column = 0; Column < 17; ++Column)
    {
      uint32_t TileID = GetTileValueUnchecked(&World, TileMap, Column, Row);
      real32 Gray = 0.3f;
      if (TileID)
      {
        Gray = 0.8f;
      }
      real32 MinX = World.UpperLeftX + ((real32)Column) * World.TileWidth;
      real32 MinY = World.UpperLeftY + ((real32)Row) * World.TileHeight;
      real32 MaxX = MinX + World.TileWidth;
      real32 MaxY = MinY + World.TileHeight;
      DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
    }
  }

  real32 PlayerR = 0.2f;
  real32 PlayerG = 0.8f;
  real32 PlayerB = 0.2f;

  real32 PlayerLeft = GameState->PlayerX - 0.5f * PlayerWidth;
  real32 PlayerTop = GameState->PlayerY - PlayerHeight;

  DrawRectangle(Buffer, PlayerLeft, PlayerTop, PlayerLeft + PlayerWidth, PlayerTop + PlayerHeight, PlayerR, PlayerG, PlayerB);

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
