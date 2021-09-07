#ifndef HANDMADE_INTRINSICS_H
#define HANDMADE_INTRINSICS_H

#include "math.h"
#include <stdlib.h>
//TODO: Convert all of these to platform-efficient versions and remove math.h
inline real32 AbsoluteValue(real32 Real32)
{
  real32 Result = (real32)fabs(Real32);
  return Result;
}

inline uint32_t RotateLeft(uint32_t Value, int32_t Amount)
{
  uint32_t Result = _rotl(Value, Amount);
  return Result;
}

inline uint32_t RotateRight(uint32_t Value, int32_t Amount)
{
  uint32_t Result = _rotr(Value, Amount);
  return Result;
}

inline int32_t RoundReal32ToInt32(real32 Real32)
{
  int32_t Result = (int32_t)roundf(Real32);
  return Result;
}

inline uint32_t RoundReal32ToUInt32(real32 Real32)
{
  uint32_t Result = (uint32_t)roundf(Real32);
  //TODO: Intrinsic???
  return Result;
}

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

inline real32 Sin(real32 Angle)
{
  real32 Result = sinf(Angle);
  return Result;
}

inline real32 Cos(real32 Angle)
{
  real32 Result = cosf(Angle);
  return Result;
}

inline real32 ATan2(real32 Y, real32 X)
{
  real32 Result = atan2f(Y, X);
  return Result;
}

struct bit_scan_result
{
  bool Found;
  uint32_t Index;
};

inline bit_scan_result FindLeastSignificantSetBit(uint32_t Value)
{
  bit_scan_result Result = {};

#if COMPILER_MSVC
  Result.Found = _BitScanForward((unsigned log *)&Result.Index, Value);
#else
  for (uint32_t Test = 0; Test < 32; ++Test)
  {
    if (Value & (1 << Test))
    {
      Result.Index = Test;
      Result.Found = true;
      break;
    }
  }
#endif
  return Result;
}

#endif
