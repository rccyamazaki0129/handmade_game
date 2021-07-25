#include "handmade.h"

#include "handmade_tile.cpp"

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


extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == (ArrayCount(Input->Controllers[0].Buttons)));
  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

  real32 PlayerHeight = 1.4f;
  real32 PlayerWidth = 0.75f * PlayerHeight;

  game_state *GameState = (game_state *)Memory->PermanentStorage;
  if (!Memory->IsInitialized)
  {
    GameState->PlayerP.AbsTileX = 1;
    GameState->PlayerP.AbsTileY = 3;
    GameState->PlayerP.TileRelX = 5.0f;
    GameState->PlayerP.TileRelY = 5.0f;
    InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state), (uint8_t *)Memory->PermanentStorage + sizeof(game_state));

    GameState->World = PushStruct(&GameState->WorldArena, world);
    world *World = GameState->World;
    World->TileMap = PushStruct(&GameState->WorldArena, tile_map);

    tile_map *TileMap = World->TileMap;

    TileMap->ChunkShift = 8;
    TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1;
    TileMap->ChunkDim = 256;
    TileMap->TileChunkCountX = 4;
    TileMap->TileChunkCountY = 4;

    TileMap->TileChunks = PushArray(&GameState->WorldArena, TileMap->TileChunkCountX*TileMap->TileChunkCountY, tile_chunk);

    for (uint32_t Y = 0; Y < TileMap->TileChunkCountY; ++Y)
    {
      for (uint32_t X = 0; X < TileMap->TileChunkCountX; ++X)
      {
        TileMap->TileChunks[Y*TileMap->TileChunkCountX + X].Tiles = PushArray(&GameState->WorldArena, TileMap->ChunkDim*TileMap->ChunkDim, uint32_t);
      }
    }

    TileMap->TileSideInMeters = 1.4f;
    TileMap->TileSideInPixles = 60;
    TileMap->MetersToPixels = (real32)TileMap->TileSideInPixles / (real32)TileMap->TileSideInMeters;

    real32 LowerLeftX = -(real32)TileMap->TileSideInPixles / 2;
    real32 LowerLeftY = (real32)Buffer->Height;

    uint32_t TilesPerWidth = 17;
    uint32_t TilesPerHeight = 9;
    for (uint32_t ScreenY = 0; ScreenY < 32; ++ScreenY)
    {
      for (uint32_t ScreenX = 0; ScreenX < 32; ++ScreenX)
      {
        for (uint32_t TileY = 0; TileY < TilesPerHeight; ++TileY)
        {
          for (uint32_t TileX = 0; TileX < TilesPerWidth; ++TileX)
          {
            uint32_t AbsTileX = ScreenX * TilesPerWidth + TileX;
            uint32_t AbsTileY = ScreenY * TilesPerHeight + TileY;
            SetTileValue(&GameState->WorldArena, World->TileMap, AbsTileX, AbsTileY, ((TileX == TileY) && (TileY % 2)) ? 1: 0);
          }
        }
      }
    }
    Memory->IsInitialized = true;
  }

  world *World = GameState->World;
  tile_map *TileMap = World->TileMap;

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
      real32 PlayerSpeed = 4.0f;
      if (Controller->ActionUp.EndedDown)
      {
        PlayerSpeed = 12.0f;
      }
      dPlayerX *= PlayerSpeed;
      dPlayerY *= PlayerSpeed;
      //TODO: Diagonal will be faster. Fix once we have vector.
      tile_map_position NewPlayerP = GameState->PlayerP;
      NewPlayerP.TileRelX += Input->dtForFrame*dPlayerX;
      NewPlayerP.TileRelY += Input->dtForFrame*dPlayerY;
      NewPlayerP = RecanonicalizePosition(TileMap, NewPlayerP);
      //TODO: Delta function that auto-recanonicalizes

      tile_map_position PlayerLeft = NewPlayerP;
      PlayerLeft.TileRelX -= 0.5f * PlayerWidth;
      PlayerLeft = RecanonicalizePosition(TileMap, PlayerLeft);

      tile_map_position PlayerRight = NewPlayerP;
      PlayerRight.TileRelX += 0.5f * PlayerWidth;
      PlayerRight = RecanonicalizePosition(TileMap, PlayerRight);

      if (IsTileMapPointEmpty(TileMap, NewPlayerP)
          && IsTileMapPointEmpty(TileMap, PlayerLeft)
          && IsTileMapPointEmpty(TileMap, PlayerRight))
      {
            GameState->PlayerP = NewPlayerP;
      }
    }
  }

  DrawRectangle(Buffer, 0.0f, 0.0f, (real32)Buffer->Width, (real32)Buffer->Height, 1.0f, 0.0f, 1.0f);

  real32 ScreenCenterX = 0.5f * (real32)Buffer->Width;
  real32 ScreenCenterY = 0.5f * (real32)Buffer->Height;
  for (int32_t RelRow = -10; RelRow < 10; ++RelRow)
  {
    for (int32_t RelColumn = -20; RelColumn < 20; ++RelColumn)
    {
      uint32_t Column = GameState->PlayerP.AbsTileX + RelColumn;
      uint32_t Row = GameState->PlayerP.AbsTileY + RelRow;
      uint32_t TileID = GetTileValue(TileMap, Column, Row);
      real32 Gray = 0.3f;
      if (TileID)
      {
        Gray = 0.8f;
      }

      if ((Column == GameState->PlayerP.AbsTileX) && (Row == GameState->PlayerP.AbsTileY))
      {
        Gray = 0.1f;
      }

      real32 CenX = ScreenCenterX - TileMap->MetersToPixels * GameState->PlayerP.TileRelX + ((real32)RelColumn) * TileMap->TileSideInPixles;
      real32 CenY = ScreenCenterY + TileMap->MetersToPixels * GameState->PlayerP.TileRelY - ((real32)RelRow) * TileMap->TileSideInPixles;
      real32 MinX = CenX - 0.5f * TileMap->TileSideInPixles;
      real32 MinY = CenY - 0.5f * TileMap->TileSideInPixles;
      real32 MaxX = CenX + 0.5f * TileMap->TileSideInPixles;
      real32 MaxY = CenY + 0.5f * TileMap->TileSideInPixles;
      DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
    }
  }

  real32 PlayerR = 0.7f;
  real32 PlayerG = 0.8f;
  real32 PlayerB = 0.2f;

  real32 PlayerLeft = ScreenCenterX - 0.5f * TileMap->MetersToPixels * PlayerWidth;
  real32 PlayerTop = ScreenCenterY - TileMap->MetersToPixels * PlayerHeight;

  DrawRectangle(Buffer, PlayerLeft, PlayerTop, PlayerLeft + TileMap->MetersToPixels * PlayerWidth, PlayerTop + TileMap->MetersToPixels * PlayerHeight, PlayerR, PlayerG, PlayerB);

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
