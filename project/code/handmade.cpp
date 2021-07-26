#include "handmade.h"

#include "handmade_tile.cpp"
#include "handmade_random.h"

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

    TileMap->ChunkShift = 4;
    TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1;
    TileMap->ChunkDim = 256;
    TileMap->TileChunkCountX = 128;
    TileMap->TileChunkCountY = 128;
    TileMap->TileChunkCountZ = 2;

    TileMap->TileChunks = PushArray(&GameState->WorldArena, TileMap->TileChunkCountX*TileMap->TileChunkCountY*TileMap->TileChunkCountZ, tile_chunk);

    TileMap->TileSideInMeters = 1.4f;

    uint32_t RandomNumberIndex = 0;

    uint32_t TilesPerWidth = 17;
    uint32_t TilesPerHeight = 9;
    uint32_t ScreenY = 0;
    uint32_t ScreenX = 0;
    uint32_t AbsTileZ = 0;

    bool DoorLeft = false;
    bool DoorRight = false;
    bool DoorTop = false;
    bool DoorBottom = false;
    bool DoorUp = false;
    bool DoorDown = false;

    for (uint32_t ScreenIndex = 0; ScreenIndex < 100; ++ScreenIndex)
    {
      Assert(RandomNumberIndex < ArrayCount(RandomNumberTable));
      uint32_t RandomChoice;
      if (DoorUp || DoorDown)
      {
        RandomChoice = RandomNumberTable[RandomNumberIndex++] % 2;
      }
      else
      {
        RandomChoice = RandomNumberTable[RandomNumberIndex++] % 3;
      }

      if (RandomChoice == 2)
      {
        if (AbsTileZ == 0)
        {
          DoorUp = true;
        }
        else
        {
          DoorDown = true;
        }
      }
      else if (RandomChoice == 1)
      {
        DoorRight = true;
      }
      else
      {
        DoorTop = true;
      }

      for (uint32_t TileY = 0; TileY < TilesPerHeight; ++TileY)
      {
        for (uint32_t TileX = 0; TileX < TilesPerWidth; ++TileX)
        {
          uint32_t AbsTileX = ScreenX * TilesPerWidth + TileX;
          uint32_t AbsTileY = ScreenY * TilesPerHeight + TileY;

          uint32_t TileValue = 1;
          if ((TileX == 0) && (!DoorLeft || (TileY != (TilesPerHeight / 2))))
          {
            TileValue = 2;
          }

          if (TileX == (TilesPerWidth - 1) && (!DoorRight || (TileY != (TilesPerHeight / 2))))
          {
            TileValue = 2;
          }

          if ((TileY == 0) && (!DoorBottom || (TileX != (TilesPerWidth / 2))))
          {
            TileValue = 2;
          }
          if ((TileY == (TilesPerHeight - 1)) && (!DoorTop || (TileX != (TilesPerWidth / 2))))
          {
            TileValue = 2;
          }
          if ((TileX == 10) && (TileY == 6))
          {
            if (DoorUp)
            {
              TileValue = 3;
            }
            if (DoorDown)
            {
              TileValue = 4;
            }
          }
          SetTileValue(&GameState->WorldArena, World->TileMap, AbsTileX, AbsTileY, AbsTileZ, TileValue);
        }
      }

      DoorLeft = DoorRight;
      DoorBottom = DoorTop;
      if (DoorUp)
      {
        DoorDown = true;
        DoorUp = false;
      }
      else if (DoorDown)
      {
        DoorDown = false;
        DoorUp = true;
      }
      else
      {
        DoorDown = false;
        DoorUp = false;
      }
      DoorRight = false;
      DoorTop = false;
      //TODO: Random number generator
      if (RandomChoice == 2)
      {
        if (AbsTileZ == 0)
        {
          AbsTileZ = 1;
        }
        else
        {
          AbsTileZ = 0;
        }
      }
      else if (RandomChoice == 1)
      {
        ScreenX += 1;
      }
      else
      {
        ScreenY += 1;
      }
    }

    Memory->IsInitialized = true;
  }

  world *World = GameState->World;
  tile_map *TileMap = World->TileMap;

  int32_t TileSideInPixles = 60;
  real32 MetersToPixels = (real32)TileSideInPixles / (real32)TileMap->TileSideInMeters;
  real32 LowerLeftX = -(real32)TileSideInPixles / 2;
  real32 LowerLeftY = (real32)Buffer->Height;


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
        dPlayerY += (real32)(4.0f*PadAdjust*(Controller->StickAverageY));
      }
      else
      {
        PadAdjust = 1.0f;
        dPlayerY += (real32)(3.0f*PadAdjust*(Controller->StickAverageY));
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
      uint32_t TileID = GetTileValue(TileMap, Column, Row, GameState->PlayerP.AbsTileZ);
      if (TileID > 0)
      {
        real32 Gray = 0.3f;
        if (TileID == 2)
        {
          Gray = 0.8f;
        }

        if (TileID > 2)
        {
          Gray = 0.23f;
        }

        if ((Column == GameState->PlayerP.AbsTileX) && (Row == GameState->PlayerP.AbsTileY))
        {
          Gray = 0.1f;
        }

        real32 CenX = ScreenCenterX - MetersToPixels * GameState->PlayerP.TileRelX + ((real32)RelColumn) * TileSideInPixles;
        real32 CenY = ScreenCenterY + MetersToPixels * GameState->PlayerP.TileRelY - ((real32)RelRow) * TileSideInPixles;
        real32 MinX = CenX - 0.5f * TileSideInPixles;
        real32 MinY = CenY - 0.5f * TileSideInPixles;
        real32 MaxX = CenX + 0.5f * TileSideInPixles;
        real32 MaxY = CenY + 0.5f * TileSideInPixles;
        DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
      }
    }
  }

  real32 PlayerR = 0.7f;
  real32 PlayerG = 0.8f;
  real32 PlayerB = 0.2f;

  real32 PlayerLeft = ScreenCenterX - 0.5f * MetersToPixels * PlayerWidth;
  real32 PlayerTop = ScreenCenterY - MetersToPixels * PlayerHeight;

  DrawRectangle(Buffer, PlayerLeft, PlayerTop, PlayerLeft + MetersToPixels * PlayerWidth, PlayerTop + MetersToPixels * PlayerHeight, PlayerR, PlayerG, PlayerB);

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
