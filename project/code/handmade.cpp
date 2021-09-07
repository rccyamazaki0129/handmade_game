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

    uint32_t RedMask = Header->RedMask;
    uint32_t GreenMask = Header->GreenMask;
    uint32_t BlueMask = Header->BlueMask;
    uint32_t AlphaMask = ~(RedMask | GreenMask | BlueMask);

    bit_scan_result RedScan = FindLeastSignificantSetBit(RedMask);
    bit_scan_result GreenScan = FindLeastSignificantSetBit(GreenMask);
    bit_scan_result BlueScan = FindLeastSignificantSetBit(BlueMask);
    bit_scan_result AlphaScan = FindLeastSignificantSetBit(AlphaMask);

    Assert(RedScan.Found);
    Assert(GreenScan.Found);
    Assert(BlueScan.Found);
    Assert(AlphaScan.Found);

    int32_t RedShift = 16 - (int32_t)RedScan.Index;
    int32_t GreenShift = 8 - (int32_t)GreenScan.Index;
    int32_t BlueShift = 0 - (int32_t)BlueScan.Index;
    int32_t AlphaShift = 24 - (int32_t)AlphaScan.Index;

    uint32_t *SourceDest = Pixels;
    for (int32_t Y = 0; Y < Header->Height; ++Y)
    {
      for (int32_t X = 0; X < Header->Width; ++X)
      {
        uint32_t C = *SourceDest;

        *SourceDest++ = (RotateLeft(C & RedMask, RedShift) | RotateLeft(C & GreenMask, GreenShift) | RotateLeft(C & BlueMask, BlueShift) | RotateLeft(C & AlphaMask, AlphaShift));
      }
    }
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

inline entity* GetEntity(game_state *GameState, uint32_t Index)
{
  entity *Entity = 0;
  if ((Index > 0) && (Index < ArrayCount(GameState->Entities)))
  {
    Entity = &GameState->Entities[Index];
  }

  return Entity;
}

internal void InitializePlayer(game_state *GameState, uint32_t EntityIndex)
{
  entity *Entity = GetEntity(GameState, EntityIndex);
  Entity->Exists = true;
  Entity->P.AbsTileX = 1;
  Entity->P.AbsTileY = 3;
  Entity->P.Offset.X = 5.0f;
  Entity->P.Offset.Y = 5.0f;

  Entity->Height = 1.4f;
  Entity->Width = 0.75f * Entity->Height;

  if (!GetEntity(GameState, GameState->CameraFollowingEntityIndex))
  {
    GameState->CameraFollowingEntityIndex = EntityIndex;
  }
}

internal uint32_t AddEntity(game_state *GameState)
{
  uint32_t EntityIndex = GameState->EntityCount++;
  Assert(GameState->EntityCount < ArrayCount(GameState->Entities));
  entity *Entity = &GameState->Entities[GameState->EntityCount++];
  *Entity = {};
  return EntityIndex;
}

internal void MovePlayer(game_state *GameState, entity *Entity, real32 dt, v2 ddP)
{
  tile_map *TileMap = GameState->World->TileMap;

  if ((ddP.X != 0.0f) && (ddP.Y != 0.0f))
  {
    //NOTE: sqrt(1/2) = 0.707106781187
    ddP *= 0.707106781187f;
  }

  real32 PlayerSpeed = 40.0f;// m/s^2
  ddP *= PlayerSpeed;
  //TODO: ODE here!
  ddP += -5.0f * Entity->dP;

  tile_map_position OldPlayerP = Entity->P;
  tile_map_position NewPlayerP = OldPlayerP;
  v2 PlayerDelta = 0.5f * ddP * Square(dt) + Entity->dP * dt;
  NewPlayerP.Offset += PlayerDelta;
  Entity->dP = ddP * dt + Entity->dP;
  NewPlayerP = RecanonicalizePosition(TileMap, NewPlayerP);
  //TODO: Delta function that auto-recanonicalizes

#if 1
  tile_map_position PlayerLeft = NewPlayerP;
  PlayerLeft.Offset.X -= 0.5f * Entity->Width;
  PlayerLeft = RecanonicalizePosition(TileMap, PlayerLeft);

  tile_map_position PlayerRight = NewPlayerP;
  PlayerRight.Offset.X += 0.5f * Entity->Width;
  PlayerRight = RecanonicalizePosition(TileMap, PlayerRight);

  bool Collided = false;
  tile_map_position ColP = {};
  if (!IsTileMapPointEmpty(TileMap, NewPlayerP))
  {
    ColP = NewPlayerP;
    Collided = true;
  }
  if (!IsTileMapPointEmpty(TileMap, PlayerLeft))
  {
    ColP = PlayerLeft;
    Collided = true;
  }
  if (!IsTileMapPointEmpty(TileMap, PlayerRight))
  {
    ColP = PlayerRight;
    Collided = true;
  }

  if (Collided)
  {
    v2 r = {};
    if (ColP.AbsTileX < Entity->P.AbsTileX)
    {
      r = v2{1, 0};
    }
    if (ColP.AbsTileX > Entity->P.AbsTileX)
    {
      r = v2{1, 0};
    }
    if (ColP.AbsTileY < Entity->P.AbsTileY)
    {
      r = v2{0, 1};
    }
    if (ColP.AbsTileY > Entity->P.AbsTileY)
    {
      r = v2{0, 1};
    }
    Entity->dP = Entity->dP - 1 * Inner(Entity->dP, r) * r;
  }
  else
  {
    Entity->P = NewPlayerP;
  }
#else
  uint32_t MinTileX = 0;
  uint32_t MinTileY = 0;
  uint32_t OnePastMaxTileX = 0;
  uint32_t OnePastMaxTileY = 0;
  uint32_t AbsTileZ = Entity->P.AbsTileZ;
  tile_map_position BestPoint = Entity->P;
  real32 BestDistanceSq = LengthSq(PlayerDelta);
  for (uint32_t AbsTileY = MinTileY; AbsTileY != OnePastMaxTileY; ++AbsTileY)
  {
    for (uint32_t AbsTileX = MinTileX; AbsTileX != OnePastMaxTileX; ++AbsTileX)
    {
      tile_map_position TestTileP = CenteredTilePoint(AbsTileX, AbsTileY, AbsTileZ);
      uint32_t TileValue = GetTileValue(TileMap, TestTileP);
      if (IsTileValueEmpty(TileValue))
      {
        v2 MinCorner = -0.5f * v2{TileMap->TileSideInMeters, TileMap->TileSideInMeters};
        v2 MaxCorner = 0.5f * v2{TileMap->TileSideInMeters, TileMap->TileSideInMeters};

        tile_map_difference RelNewPlayerP = SubtractInReal32(TileMap, &TestTileP, &NewPlayerP);
        v2 TestP = ClosestPointInRectangle(MinCorner, MaxCorner, RelNewPlayerP);
        if (BestDistanceSq > TestDistanceSq)
        {
          BestPlayerP = ;
          BestDistanceSq = ;
        }
      }
    }
  }
#endif

  //NOTE: Update camera/player Z based on last movement.
  if (!AreOnSameTile(&OldPlayerP, &Entity->P))
  {
    uint32_t NewTileValue = GetTileValue(TileMap, Entity->P);
    if (NewTileValue == 3)
    {
      ++Entity->P.AbsTileZ;
    }
    else if (NewTileValue == 4)
    {
      --Entity->P.AbsTileZ;
    }
  }

  if (Entity->dP.X == 0.0f && Entity->dP.Y == 0.0f)
  {
    //NOTE: Leave FacingDirection whatever it was
  }
  else if (AbsoluteValue(Entity->dP.X) > AbsoluteValue(Entity->dP.Y))
  {
    if (Entity->dP.X > 0)
    {
      Entity->FacingDirection = 3;
    }
    else
    {
      Entity->FacingDirection = 2;
    }
  }
  else
  {
    if (Entity->dP.Y > 0)
    {
      Entity->FacingDirection = 1;
    }
    else
    {
      Entity->FacingDirection = 0;
    }
  }
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == (ArrayCount(Input->Controllers[0].Buttons)));
  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

  game_state *GameState = (game_state *)Memory->PermanentStorage;
  if (!Memory->IsInitialized)
  {
    //NOTE: Reserve entity slot 0 for the null entity
    uint32_t NullEntityIndex = AddEntity(GameState);
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
    entity *ControllingEntity = GetEntity(GameState, GameState->PlayerIndexForController[ControllerIndex]);
    if (ControllingEntity)
    {
      v2 ddP = {};

      if (Controller->IsAnalog)
      {
        //NOTE: use analog movement tuning
        ddP = v2{Controller->StickAverageX, Controller->StickAverageY};
#if 0
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
        Entity->P.Offset.X += Input->dtForFrame*dPlayerX;
        Entity->P.Offset.Y += Input->dtForFrame*dPlayerY;
#endif
      }
      else
      {
        //NOTE: use digital movement tuning

        if (Controller->MoveUp.EndedDown)
        {
          ddP.Y = 1.0f;
        }
        if (Controller->MoveDown.EndedDown)
        {
          ddP.Y = -1.0f;
        }
        if (Controller->MoveLeft.EndedDown)
        {
          ddP.X = -1.0f;
        }
        if (Controller->MoveRight.EndedDown)
        {
          ddP.X = 1.0f;
        }
      }

      MovePlayer(GameState, ControllingEntity, Input->dtForFrame, ddP);
    }
    else
    {
      if (Controller->Start.EndedDown)
      {
        uint32_t EntityIndex = AddEntity(GameState);
        InitializePlayer(GameState, EntityIndex);
        GameState->PlayerIndexForController[ControllerIndex] = EntityIndex;
      }
    }
  }

  entity *CameraFollowingEntity = GetEntity(GameState, GameState->CameraFollowingEntityIndex);
  if (CameraFollowingEntity)
  {
    GameState->CameraP.AbsTileZ = CameraFollowingEntity->P.AbsTileZ;

    tile_map_difference Diff = SubtractInReal32(TileMap, &CameraFollowingEntity->P, &GameState->CameraP);
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
  }

  //NOTE: Render
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

  entity* Entity = GameState->Entities;
  for (uint32_t EntityIndex = 0; EntityIndex < GameState->EntityCount; ++EntityIndex, ++Entity)
  {
    //TODO: Culling of entities based on Z / camera view
    if (Entity->Exists)
    {
      tile_map_difference Diff = SubtractInReal32(TileMap, &Entity->P, &GameState->CameraP);

      real32 PlayerR = 0.7f;
      real32 PlayerG = 0.8f;
      real32 PlayerB = 0.2f;
      real32 PlayerGroundPointX = ScreenCenterX + MetersToPixels * Diff.dXY.X;
      real32 PlayerGroundPointY = ScreenCenterY - MetersToPixels * Diff.dXY.Y;
      v2 PlayerLeftTop = {PlayerGroundPointX - 0.5f * MetersToPixels * Entity->Width, PlayerGroundPointY - MetersToPixels * Entity->Height};
      v2 EntityWidthHeight = {Entity->Width, Entity->Height};

      DrawRectangle(Buffer, PlayerLeftTop, PlayerLeftTop + MetersToPixels * EntityWidthHeight, PlayerR, PlayerG, PlayerB);
      hero_bitmaps *HeroBitmaps = &GameState->HeroBitmaps[Entity->FacingDirection];
      DrawBitmap(Buffer, &HeroBitmaps->Character, PlayerGroundPointX, PlayerGroundPointY, HeroBitmaps->AlignX, HeroBitmaps->AlignY);
    }
  }
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
