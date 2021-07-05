#include "handmade.h"

internal void GameOutputSound(game_sound_output_buffer *SoundBuffer, int ToneHz){
  local_persist real32 tSine;
  int16_t ToneVolume = 6000;
  int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

  int16_t *SampleOut = SoundBuffer->Samples;

  for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex){
    real32 SineValue = sinf(tSine);
    int16_t SampleValue = (int16_t)(SineValue * ToneVolume);
    *SampleOut++ = SampleValue;
    *SampleOut++ = SampleValue;

    tSine += 2.0f * Pi32 * 1.0f / (real32)WavePeriod;
  }
}


internal void RenderWeirdGradient(game_offscreen_buffer *Buffer, int XOffset, int YOffset){
  //TODO: Let's see what the optimizer does
  uint8_t *Row = (uint8_t *)Buffer->Memory;

  for (int y = 0; y < Buffer->Height; y++){

    uint32_t *Pixel = (uint32_t *)Row;

    for (int x = 0; x < Buffer->Width; x++){
      /*
        Pixel in memory: 00 00 00 00
                         BB GG RR XX
        0x XXRRGGBB
      */
      uint8_t Blue = (x + XOffset);
      uint8_t Green = (y + YOffset);
      uint8_t Red = uint8_t((x + y) / 16);

      *Pixel++ = ((Red << 16) | (Green << 8) | Blue);
    }
    Row += Buffer->Pitch;
  }
}

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer, int XOffset, int YOffset, game_sound_output_buffer *SoundBuffer, int ToneHz){
  // TODO: Allow sample offsets here for more robust platform options
  GameOutputSound(SoundBuffer, ToneHz);
  RenderWeirdGradient(Buffer, XOffset, YOffset);
}
