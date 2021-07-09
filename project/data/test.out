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
      uint8_t Blue = (uint8_t)(x + XOffset);
      uint8_t Green = (uint8_t)(y + YOffset);
      uint8_t Red = uint8_t((x + y) / 16);

      *Pixel++ = ((Red << 16) | (Green << 8) | Blue);
    }
    Row += Buffer->Pitch;
  }
}

internal void GameUpdateAndRender(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer, game_sound_output_buffer *SoundBuffer){

  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

  game_state *GameState = (game_state *)Memory->PermanentStorage;
  if (!Memory->IsInitialized){
    char *FileName = __FILE__;
    debug_read_file_result File = DEBUGPlatformReadEntireFile(FileName);
    if (File.Contents){
      DEBUGPlatformWriteEntireFile("z:/handmade_game/project/data/test.out", File.ContentsSize, File.Contents);
      DEBUGPlatformFreeFileMemory(File.Contents);
    }

    GameState->ToneHz = 256;
    //TODO: This may be more appropriate to do in the platform layer
    Memory->IsInitialized = true;
  }

  //NOTE: Input0 is usually Keyboard
  for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex){
    game_controller_input *Controller = GetController(Input, ControllerIndex);
    if (Controller->IsAnalog){
      //NOTE: use analog movement tuning
      GameState->YOffset += (int)(4.0f*(Controller->StickAverageX));
      GameState->ToneHz = 256 + (int)(128.0f*(Controller->StickAverageY));
    }
    else {
      //NOTE: use digital movement tuning
      if (Controller->MoveLeft.EndedDown){
        GameState->XOffset -= 1;
      }
      if (Controller->MoveRight.EndedDown){
        GameState->XOffset += 1;
      }
      if (Controller->MoveDown.EndedDown){
        GameState->YOffset += 1;
      }
      if (Controller->MoveUp.EndedDown){
        GameState->YOffset -= 1;
      }
    }
  }

  // TODO: Allow sample offsets here for more robust platform options
  GameOutputSound(SoundBuffer, GameState->ToneHz);
  RenderWeirdGradient(Buffer, GameState->XOffset, GameState->YOffset);
}
