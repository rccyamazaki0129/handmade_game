inline tile_chunk* GetTileChunk(tile_map *TileMap, uint32_t TileChunkX, uint32_t TileChunkY, uint32_t TileChunkZ)
{
  tile_chunk *TileChunk = 0;

  if ((TileChunkX >= 0) && (TileChunkX < TileMap->TileChunkCountX) && (TileChunkY >= 0) && (TileChunkY < TileMap->TileChunkCountY) && (TileChunkZ >= 0) && (TileChunkZ < TileMap->TileChunkCountZ))
  {
    TileChunk = &TileMap->TileChunks[TileChunkZ * TileMap->TileChunkCountY * TileMap->TileChunkCountX + TileChunkY * TileMap->TileChunkCountX + TileChunkX];
  }
  return TileChunk;
}

inline uint32_t GetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, uint32_t TileX, uint32_t TileY)
{
  Assert(TileChunk);
  Assert((TileX < TileMap->ChunkDim) && (TileY < TileMap->ChunkDim));
  uint32_t TileChunkValue = TileChunk->Tiles[TileY * TileMap->ChunkDim + TileX];
  return TileChunkValue;
}

inline void SetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, uint32_t TileX, uint32_t TileY, uint32_t TileValue)
{
  Assert(TileChunk);
  Assert((TileX < TileMap->ChunkDim) && (TileY < TileMap->ChunkDim));
  TileChunk->Tiles[TileY * TileMap->ChunkDim + TileX] = TileValue;
}

internal uint32_t GetTileValue(tile_map *TileMap, tile_chunk *TileChunk, uint32_t TestTileX, uint32_t TestTileY)
{
  uint32_t TileChunkValue = 0;
  if (TileChunk && TileChunk->Tiles)
  {
    TileChunkValue = GetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY);
  }
  return TileChunkValue;
}

internal void SetTileValue(tile_map *TileMap, tile_chunk *TileChunk, uint32_t TestTileX, uint32_t TestTileY, uint32_t TileValue)
{
  if (TileChunk && TileChunk->Tiles)
  {
    SetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY, TileValue);
  }
}

inline tile_chunk_position GetChunkPositionFor(tile_map *TileMap, uint32_t AbsTileX, uint32_t AbsTileY, uint32_t AbsTileZ)
{
  tile_chunk_position Result;
  Result.TileChunkX = AbsTileX >> TileMap->ChunkShift;
  Result.TileChunkY = AbsTileY >> TileMap->ChunkShift;
  Result.TileChunkZ = AbsTileZ;
  Result.RelTileX = AbsTileX & TileMap->ChunkMask;
  Result.RelTileY = AbsTileY & TileMap->ChunkMask;
  return Result;
}

internal uint32_t GetTileValue(tile_map *TileMap, uint32_t AbsTileX, uint32_t AbsTileY, uint32_t AbsTileZ)
{
  tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
  tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY, ChunkPos.TileChunkZ);
  uint32_t TileChunkValue = GetTileValue(TileMap, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY);

  return TileChunkValue;
}

internal uint32_t GetTileValue(tile_map *TileMap, tile_map_position Pos)
{
  uint32_t TileChunkValue = GetTileValue(TileMap, Pos.AbsTileX, Pos.AbsTileY, Pos.AbsTileZ);
  return TileChunkValue;
}

internal bool IsTileValueEmpty(uint32_t TileValue)
{
  bool Empty = (TileValue == 1) || (TileValue == 3) || (TileValue == 4);
  return Empty;
}

internal bool IsTileMapPointEmpty(tile_map *TileMap, tile_map_position Pos)
{
  uint32_t TileChunkValue = GetTileValue(TileMap, Pos);
  bool Empty = IsTileValueEmpty(TileChunkValue);
  return Empty;
}

internal void SetTileValue(memory_arena *Arena, tile_map *TileMap, uint32_t AbsTileX, uint32_t AbsTileY, uint32_t AbsTileZ, uint32_t TileValue)
{
  tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
  tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY, ChunkPos.TileChunkZ);

  Assert(TileChunk);
  if (!TileChunk->Tiles)
  {
    uint32_t TileCount = TileMap->ChunkDim * TileMap->ChunkDim;
    TileChunk->Tiles = PushArray(Arena, TileCount, uint32_t);
    for (uint32_t TileIndex = 0; TileIndex < TileCount; ++TileIndex)
    {
      TileChunk->Tiles[TileIndex] = 1;
    }
  }

  SetTileValue(TileMap, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY, TileValue);
}
//
//TODO: Do these really belong in more of a "positioning" or "geometry" file?
//
//
inline void RecanonicalizeCoord(tile_map *TileMap, uint32_t *Tile, real32 *TileRel)
{
  //TODO: Need to do something that doesn't use the divide/multiply method for recanonicalizing because this can end up rounding back on to the tile you just came from.

  //NOTE:  TileMap is assumed to be toroidal, if you step off one end you come back on the other
  int32_t Offset = RoundReal32ToInt32(*TileRel / TileMap->TileSideInMeters);
  *Tile += Offset;
  *TileRel -= Offset * TileMap->TileSideInMeters;

  //TODO: Fix floating point math so this can be < ?
  Assert(*TileRel > -0.5001f*TileMap->TileSideInMeters);
  Assert(*TileRel < 0.5001f*TileMap->TileSideInMeters);

}

inline tile_map_position RecanonicalizePosition(tile_map *TileMap, tile_map_position Pos)
{
  tile_map_position Result = Pos;

  RecanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.Offset.X);
  RecanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.Offset.Y);

  return Result;
}

inline bool AreOnSameTile(tile_map_position *A, tile_map_position *B)
{
  bool Result = (A->AbsTileX == B->AbsTileX) && (A->AbsTileY == B->AbsTileY) && (A->AbsTileZ == B->AbsTileZ);
  return Result;
}

tile_map_difference SubtractInReal32(tile_map *TileMap, tile_map_position *A, tile_map_position *B)
{
  tile_map_difference Result;

  v2 dTileXY = {(real32)A->AbsTileX - (real32)B->AbsTileX, (real32)A->AbsTileY - (real32)B->AbsTileY};
  real32 dTileZ = (real32)A->AbsTileZ - (real32)B->AbsTileZ;
  Result.dXY = TileMap->TileSideInMeters * dTileXY + (A->Offset - B->Offset);
  //TODO: Think about what we want to do about Z
  Result.dZ = TileMap->TileSideInMeters * dTileZ + 0.0f;
  return Result;
}

inline tile_map_position CenteredTilePoint(uint32_t AbsTileX, uint32_t AbsTileY, uint32_t AbsTileZ)
{
  tile_map_position Result = {};
  Result.AbsTileX = AbsTileX;
  Result.AbsTileY = AbsTileY;
  Result.AbsTileZ = AbsTileZ;
  return Result;
}
