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

internal void DrawRectangle(game_offscreen_buffer *Buffer, real32 RealMinX, real32 RealMinY, real32 RealMaxX, real32 RealMaxY, uint32_t Color)
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
    }
    else
    {
      //NOTE: use digital movement tuning
    }
  }
  DrawRectangle(Buffer, 0.0f, 0.0f, (real32)Buffer->Width, (real32)Buffer->Height, 0x000000FF);
  DrawRectangle(Buffer, 100.0f, 10.0f, 130.0f, 40.0f, 0xFF00FF00);
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
