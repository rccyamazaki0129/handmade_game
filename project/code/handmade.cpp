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

internal void DrawRectangle(game_offscreen_buffer *Buffer, v2 vMin, v2 vMax, real32 R, real32 G, real32 B)
{
  int32_t MinX = RoundReal32ToInt32(vMin.X);
  int32_t MinY = RoundReal32ToInt32(vMin.Y);
  int32_t MaxX = RoundReal32ToInt32(vMax.X);
  int32_t MaxY = RoundReal32ToInt32(vMax.Y);

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

internal loaded_bitmap DEBUGLoadBMP(thread_context *Thread, debug_platform_read_entire_file *ReadEntireFile, char *FileName)
{
  loaded_bitmap Result = {};
  //NOTE: Byte order in memory is BB GG RR AA
  // In little endian -> 0x AA RR GG BB
  debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);
  if (ReadResult.ContentsSize != 0)
  {
    bitmap_header *Header = (bitmap_header *)ReadResult.Contents;
    uint32_t *Pixels = (uint32_t *)((uint8_t *)ReadResult.Contents + Header->BitmapOffset);
    Result.Pixels = Pixels;
    Result.Width = Header->Width;
    Result.Height = Header->Height;
  }
  return Result;
}

internal void DrawBitmap(game_offscreen_buffer *Buffer, loaded_bitmap *Bitmap, real32 RealX, real32 RealY, int32_t AlignX = 0, int32_t AlignY = 0)
{
  RealX -= (real32)AlignX;
  RealY -= (real32)AlignY;
  int32_t MinX = RoundReal32ToInt32(RealX);
  int32_t MinY = RoundReal32ToInt32(RealY);
  int32_t MaxX = RoundReal32ToInt32(RealX + (real32)Bitmap->Width);
  int32_t MaxY = RoundReal32ToInt32(RealY + (real32)Bitmap->Height);

  int32_t SourceOffsetX = 0;
  if (MinX < 0)
  {
    SourceOffsetX = -MinX;
    MinX = 0;
  }
  int32_t SourceOffsetY = 0;
  if (MinY < 0)
  {
    SourceOffsetY = -MinY;
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

  //TODO: SourceRow needs to be changed based on clipping
  uint32_t *SourceRow = Bitmap->Pixels + Bitmap->Width * (Bitmap->Height - 1);
  SourceRow += -Bitmap->Width * SourceOffsetY + SourceOffsetX;
  uint8_t *DestRow = ((uint8_t *)(Buffer->Memory) + MinX*Buffer->BytesPerPixel + MinY*Buffer->Pitch);

  for (int32_t Y = MinY; Y < MaxY; ++Y)
  {
    uint32_t *Dest = (uint32_t *)DestRow;
    uint32_t *Source = SourceRow;
    for (int32_t X = MinX; X < MaxX; ++X)
    {
      real32 Alpha = (real32)((*Source >> 24) & 0xFF) / 255.0f;
      real32 SourceRed = (real32)((*Source >> 16) & 0xFF);
      real32 SourceGreen = (real32)((*Source >> 8) & 0xFF);
      real32 SourceBlue = (real32)((*Source >> 0) & 0xFF);

      real32 DestRed = (real32)((*Dest >> 16) & 0xFF);
      real32 DestGreen = (real32)((*Dest >> 8) & 0xFF);
      real32 DestBlue = (real32)((*Dest >> 0) & 0xFF);

      //NOTE: Blend equetion
      //TODO: Someday, we need to talk about premultiplied alpha.
      real32 Red = (1.0f - Alpha) * DestRed + Alpha * SourceRed;
      real32 Green = (1.0f - Alpha) * DestGreen + Alpha * SourceGreen;
      real32 Blue = (1.0f - Alpha) * DestBlue + Alpha * SourceBlue;

      *Dest = (((uint32_t)(Red + 0.5f) << 16) |
               ((uint32_t)(Green + 0.5f) << 8) |
               ((uint32_t)(Blue + 0.5f) << 0));
      // uint8_t Alpha = (*Source >> 24);
      // real32 AlphaRate = (real32)Alpha / (real32)255.0f;
      // int32_t Diff = *Source - *Dest;
      // *Dest += uint32_t(AlphaRate * Diff);

      ++Dest;
      ++Source;
    }
    DestRow += Buffer->Pitch;
    SourceRow -= Bitmap->Width;
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
    //NOTE: At the momoent, every BMP needs to have alpha channel to be loaded properly.
    GameState->Backdrop = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "cobblestone-road.bmp");

    //NOTE: Boy's Bmp resolution is withxHeihgt = 35x60
    hero_bitmaps *Bitmap;
    int32_t BmpAlignX = 18;
    int32_t BmpAlignY = 55;

    Bitmap = GameState->HeroBitmaps;
    Bitmap->Character = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "boy_front.bmp");
    Bitmap->AlignX = BmpAlignX;
    Bitmap->AlignY = BmpAlignY;
    ++Bitmap;

    Bitmap->Character = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "boy_back.bmp");
    Bitmap->AlignX = BmpAlignX;
    Bitmap->AlignY = BmpAlignY;
    ++Bitmap;

    Bitmap->Character = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "boy_left.bmp");
    Bitmap->AlignX = BmpAlignX;
    Bitmap->AlignY = BmpAlignY;
    ++Bitmap;

    Bitmap->Character = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "boy_right.bmp");
    Bitmap->AlignX = BmpAlignX;
    Bitmap->AlignY = BmpAlignY;

    GameState->CameraP.AbsTileX = 17 / 2;
    GameState->CameraP.AbsTileY = 9 / 2;
    GameState->PlayerP.AbsTileX = 1;
    GameState->PlayerP.AbsTileY = 3;
    GameState->PlayerP.Offset.X = 5.0f;
    GameState->PlayerP.Offset.Y = 5.0f;
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

      bool CreatedZDoor = false;

      if (RandomChoice == 2)
      {
        CreatedZDoor = true;
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
      if (CreatedZDoor)
      {
        DoorDown = !DoorDown;
        DoorUp = !DoorUp;
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
      GameState->PlayerP.Offset.X += Input->dtForFrame*dPlayerX;
      GameState->PlayerP.Offset.Y += Input->dtForFrame*dPlayerY;
    }
    else
    {
      //NOTE: use digital movement tuning
      v2 ddPlayer = {};

      if (Controller->MoveUp.EndedDown)
      {
        GameState->HeroFacingDirection = 1;
        ddPlayer.Y = 1.0f;
      }
      if (Controller->MoveDown.EndedDown)
      {
        GameState->HeroFacingDirection = 0;
        ddPlayer.Y = -1.0f;
      }
      if (Controller->MoveLeft.EndedDown)
      {
        GameState->HeroFacingDirection = 2;
        ddPlayer.X = -1.0f;
      }
      if (Controller->MoveRight.EndedDown)
      {
        GameState->HeroFacingDirection = 3;
        ddPlayer.X = 1.0f;
      }
      real32 PlayerSpeed = 10.0f;// m/s^2
      if (Controller->ActionUp.EndedDown)
      {
        PlayerSpeed = 40.0f;// m/s^2
      }
      ddPlayer *= PlayerSpeed;

      if ((ddPlayer.X != 0.0f) && (ddPlayer.Y != 0.0f))
      {
        //NOTE: sqrt(1/2) = 0.707106781187
        ddPlayer *= 0.707106781187f;
      }

      //TODO: ODE here!
      ddPlayer += -1.9f * GameState->dPlayerP;

      tile_map_position NewPlayerP = GameState->PlayerP;
      NewPlayerP.Offset = (0.5f * ddPlayer * Square(Input->dtForFrame) +
                          GameState->dPlayerP * Input->dtForFrame +
                          NewPlayerP.Offset);
      GameState->dPlayerP = ddPlayer * Input->dtForFrame + GameState->dPlayerP;



      NewPlayerP = RecanonicalizePosition(TileMap, NewPlayerP);
      //TODO: Delta function that auto-recanonicalizes

      tile_map_position PlayerLeft = NewPlayerP;
      PlayerLeft.Offset.X -= 0.5f * PlayerWidth;
      PlayerLeft = RecanonicalizePosition(TileMap, PlayerLeft);

      tile_map_position PlayerRight = NewPlayerP;
      PlayerRight.Offset.X += 0.5f * PlayerWidth;
      PlayerRight = RecanonicalizePosition(TileMap, PlayerRight);

      if (IsTileMapPointEmpty(TileMap, NewPlayerP)
          && IsTileMapPointEmpty(TileMap, PlayerLeft)
          && IsTileMapPointEmpty(TileMap, PlayerRight))
      {
        if (!AreOnSameTile(&GameState->PlayerP, &NewPlayerP))
        {
          uint32_t NewTileValue = GetTileValue(TileMap, NewPlayerP);
          if (NewTileValue == 3)
          {
            ++NewPlayerP.AbsTileZ;
          }
          else if (NewTileValue == 4)
          {
            --NewPlayerP.AbsTileZ;
          }
        }
        GameState->PlayerP = NewPlayerP;
      }
      tile_map_difference Diff = SubtractInReal32(TileMap, &GameState->PlayerP, &GameState->CameraP);
      if (Diff.dXY.X > 9.0f * TileMap->TileSideInMeters)
      {
        GameState->CameraP.AbsTileX += 17;
      }
      if (Diff.dXY.X < -9.0f * TileMap->TileSideInMeters)
      {
        GameState->CameraP.AbsTileX -= 17;
      }
      if (Diff.dXY.Y > 5.0f * TileMap->TileSideInMeters)
      {
        GameState->CameraP.AbsTileY += 9;
      }
      if (Diff.dXY.Y < -5.0f * TileMap->TileSideInMeters)
      {
        GameState->CameraP.AbsTileY -= 9;
      }
      GameState->CameraP.AbsTileZ = GameState->PlayerP.AbsTileZ;
    }
  }

  DrawBitmap(Buffer, &GameState->Backdrop, 0, 0);

  real32 ScreenCenterX = 0.5f * (real32)Buffer->Width;
  real32 ScreenCenterY = 0.5f * (real32)Buffer->Height;
  for (int32_t RelRow = -10; RelRow < 10; ++RelRow)
  {
    for (int32_t RelColumn = -20; RelColumn < 20; ++RelColumn)
    {
      uint32_t Column = GameState->CameraP.AbsTileX + RelColumn;
      uint32_t Row = GameState->CameraP.AbsTileY + RelRow;
      uint32_t TileID = GetTileValue(TileMap, Column, Row, GameState->CameraP.AbsTileZ);
      if (TileID > 1)
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

        if ((Column == GameState->CameraP.AbsTileX) && (Row == GameState->CameraP.AbsTileY))
        {
          Gray = 0.1f;
        }

        v2 TileSide = {0.5f * TileSideInPixles, 0.5f * TileSideInPixles};
        v2 Cen = {ScreenCenterX - MetersToPixels * GameState->CameraP.Offset.X + ((real32)RelColumn) * TileSideInPixles, ScreenCenterY + MetersToPixels * GameState->CameraP.Offset.Y - ((real32)RelRow) * TileSideInPixles};
        v2 Min = Cen - TileSide;
        v2 Max = Cen + TileSide;
        DrawRectangle(Buffer, Min, Max, Gray, Gray, Gray);
      }
    }
  }

  tile_map_difference Diff = SubtractInReal32(TileMap, &GameState->PlayerP, &GameState->CameraP);

  real32 PlayerR = 0.7f;
  real32 PlayerG = 0.8f;
  real32 PlayerB = 0.2f;
  real32 PlayerGroundPointX = ScreenCenterX + MetersToPixels * Diff.dXY.X;
  real32 PlayerGroundPointY = ScreenCenterY - MetersToPixels * Diff.dXY.Y;
  v2 PlayerLeftTop = {PlayerGroundPointX - 0.5f * MetersToPixels * PlayerWidth, PlayerGroundPointY - MetersToPixels * PlayerHeight};
  v2 PlayerWidthHeight = {PlayerWidth, PlayerHeight};

  DrawRectangle(Buffer, PlayerLeftTop, PlayerLeftTop + MetersToPixels * PlayerWidthHeight, PlayerR, PlayerG, PlayerB);
  hero_bitmaps *HeroBitmaps = &GameState->HeroBitmaps[GameState->HeroFacingDirection];
  DrawBitmap(Buffer, &HeroBitmaps->Character, PlayerGroundPointX, PlayerGroundPointY, HeroBitmaps->AlignX, HeroBitmaps->AlignY);
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
