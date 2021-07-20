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

internal int32_t RoundReal32ToInt32(real32 Real32)
{
  int32_t Result = (int32_t)(Real32 + 0.5f);
  //TODO: Intrinsic???
  return Result;
}

internal uint32_t RoundReal32ToUInt32(real32 Real32)
{
  uint32_t Result = (uint32_t)(Real32 + 0.5f);
  //TODO: Intrinsic???
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


extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == (ArrayCount(Input->Controllers[0].Buttons)));
  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

  game_state *GameState = (game_state *)Memory->PermanentStorage;
  if (!Memory->IsInitialized)
  {
    Memory->IsInitialized = true;
  }

  //NOTE: Input0 is usually Keyboard
  for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex)
  {
    game_controller_input *Controller = GetController(Input, ControllerIndex);
    if (Controller->IsAnalog)
    {
      //NOTE: use analog movement tuning
      GameState->PlayerX += (int)(1.0f*(Controller->StickAverageX));
      GameState->PlayerY += (int)(-1.0f*(Controller->StickAverageY));
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
      GameState->PlayerX += Input->dtForFrame*dPlayerX;
      GameState->PlayerY += Input->dtForFrame*dPlayerY;
    }
  }

  uint32_t TileMap[9][17] =
  {
    {1, 1, 1, 1,   1, 1, 1, 1,   0,   1, 1, 1, 1,   1, 1, 1, 1},
    {1, 1, 0, 0,   0, 0, 0, 0,   0,   0, 0, 0, 0,   0, 1, 0, 1},
    {1, 0, 0, 1,   0, 1, 0, 0,   0,   1, 0, 1, 1,   0, 0, 0, 1},
    {1, 0, 1, 0,   0, 0, 1, 0,   0,   0, 0, 0, 0,   0, 0, 1, 1},
    {0, 0, 0, 0,   0, 0, 1, 0,   1,   0, 1, 1, 0,   0, 0, 0, 0},
    {1, 0, 1, 0,   1, 0, 0, 0,   0,   0, 0, 0, 0,   0, 1, 0, 1},
    {1, 0, 1, 0,   0, 0, 0, 0,   0,   0, 1, 0, 0,   0, 0, 0, 1},
    {1, 0, 0, 0,   0, 0, 0, 0,   0,   0, 0, 1, 0,   0, 0, 1, 1},
    {1, 1, 1, 1,   1, 1, 1, 1,   0,   1, 1, 1, 1,   1, 1, 1, 1},
  };

  real32 UpperLeftX = -30;
  real32 UpperLeftY = 0;
  real32 TileWidth = 60;
  real32 TileHeight = 60;

  DrawRectangle(Buffer, 0.0f, 0.0f, (real32)Buffer->Width, (real32)Buffer->Height, 1.0f, 0.0f, 1.0f);

  for (int Row = 0; Row < 9; ++Row)
  {
    for (int Column = 0; Column < 17; ++Column)
    {
      uint32_t TileID = TileMap[Row][Column];
      real32 Gray = 0.3f;
      if (TileID)
      {
        Gray = 0.8f;
      }
      real32 MinX = UpperLeftX + ((real32)Column) * TileWidth;
      real32 MinY = UpperLeftY + ((real32)Row) * TileHeight;
      real32 MaxX = MinX + TileWidth;
      real32 MaxY = MinY + TileHeight;
      DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
    }
  }

  real32 PlayerR = 0.0f;
  real32 PlayerG = 1.0f;
  real32 PlayerB = 1.0f;
  real32 PlayerWidth = 0.75f * TileWidth;
  real32 PlayerHeight = TileHeight;
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
