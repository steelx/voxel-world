// Copyright Ajinkya Borade 2026 (c).


#include "VoxelChunk.h"

#include "ProceduralMeshComponent.h"
#include "WorldObjects/VoxelWorldObject.h"


// Sets default values
AVoxelChunk::AVoxelChunk()
{
    PrimaryActorTick.bCanEverTick = true;
	MeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMeshComp"));
    SetRootComponent(MeshComponent);
    // Crucial for characters to walk on procedural meshes without falling!
    MeshComponent->bUseComplexAsSimpleCollision = true;
}

void AVoxelChunk::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // POLLING: Check if the background work is finished
    if (AsyncMeshData.bIsDataReady) {
        // APPLY TO GPU (Must happen on Game Thread)
        MeshComponent->CreateMeshSection_LinearColor(0,
            AsyncMeshData.Vertices, AsyncMeshData.Triangles, AsyncMeshData.Normals,
            AsyncMeshData.UVs, AsyncMeshData.Colors, TArray<FProcMeshTangent>(), true);

        // --- MATERIAL ---
        if (bUseSolidColors && SolidMaterial)
        {
            MeshComponent->SetMaterial(0, SolidMaterial);
        }
        else if (!bUseSolidColors && VoxelMaterial)
        {
            MeshComponent->SetMaterial(0, VoxelMaterial);
        }

        AsyncMeshData.bIsDataReady = false; // Work is done!
    }
}

void AVoxelChunk::InitializeChunk(const int32 InGlobalSeed, const FIntPoint InTargetCoord, const int32 InChunkSize)
{
    this->GlobalSeed = InGlobalSeed;
    this->GridCoordinate = InTargetCoord;
    this->ChunkSize = InChunkSize;

    // --- CREATE A THREAD-SAFE SNAPSHOT for GetBlockData ---
    BlockDataCache.Empty();
    if (BlockDataTable)
    {
        const UEnum* VoxelEnum = StaticEnum<EVoxelType>();
        // Iterate through all Enum values and cache their DataTable rows
        for (int32 i = 0; i < VoxelEnum->NumEnums() - 1; i++)
        {
            int64 EnumValue = VoxelEnum->GetValueByIndex(i);
            FString EnumName = VoxelEnum->GetNameStringByIndex(i);

            if (FVoxelBlockData* RowData = BlockDataTable->FindRow<FVoxelBlockData>(FName(*EnumName), TEXT("Cache")))
            {
                BlockDataCache.Add(static_cast<EVoxelType>(EnumValue), *RowData);
            }
        }
    }

    // Kick off the background thread!
    (new FAutoDeleteAsyncTask<FVoxelGenerationWorker>(this))->StartBackgroundTask();
}

void AVoxelChunk::DoBackgroundGeneration()
{
    /// FastNoise
    // We use SimplexFractal directly as the noise type for the surface
    SurfaceNoise.SetNoiseType(FastNoise::SimplexFractal);
    SurfaceNoise.SetFrequency(SurfaceNoiseFrequency);
    SurfaceNoise.SetFractalOctaves(4);

    // Cellular noise creates great Swiss-cheese cave structures
    CaveNoise.SetNoiseType(FastNoise::Cellular);
    CaveNoise.SetFrequency(CaveNoiseFrequency);

    // BIOME NOISE
    BiomeNoise.SetNoiseType(FastNoise::Cellular);
    BiomeNoise.SetCellularReturnType(FastNoise::CellValue); // Gives flat, distinct regions
    BiomeNoise.SetCellularDistanceFunction(FastNoise::Natural); // Organic borders
    BiomeNoise.SetFrequency(BiomeNoiseFrequency);

    BasinNoise.SetNoiseType(FastNoise::SimplexFractal);
    BasinNoise.SetFrequency(BasinNoiseFrequency);

    // SEED
    SurfaceNoise.SetSeed(GlobalSeed);
    CaveNoise.SetSeed(GlobalSeed);
    BiomeNoise.SetSeed(GlobalSeed);
    BasinNoise.SetSeed(GlobalSeed);

    // Regenerate the world
    GenerateVoxelData();
    GenerateMesh();

    AsyncMeshData.bIsDataReady = true; // Signal completion
}

// 1D Flattening Math
int32 AVoxelChunk::GetVoxelIndex(const int32 X, const int32 Y, const int32 Z) const
{
    const int32 PaddedXY = ChunkSize + 2;
    return (X + 1) + ((Y + 1) * PaddedXY) + ((Z + 1) * PaddedXY * PaddedXY);
}

// Safe wrapper to prevent out-of-bounds array crashes
EVoxelType AVoxelChunk::GetVoxelType(const int32 X, const int32 Y, const int32 Z) const
{
    // Use ChunkHeight for the Z boundary check!
    if (X < -1 || X > ChunkSize || Y < -1 || Y > ChunkSize || Z < -1 || Z > ChunkHeight)
        return EVoxelType::Empty;

    return VoxelData[GetVoxelIndex(X, Y, Z)];
}

int32 AVoxelChunk::FindTopSolidZ(const int32 X, const int32 Y) const
{
    for (int32 Z = ChunkSize - 1; Z >= 0; Z--)
    {
        const EVoxelType T = GetVoxelType(X, Y, Z);
        if (T != EVoxelType::Empty && T != EVoxelType::Water && T != EVoxelType::Lava)
        {
            return Z;
        }
    }
    return -1;
}

bool AVoxelChunk::IsStableGround(const int32 X, const int32 Y, const int32 Z) const
{
    // Must be on solid ground (not water/lava/air)
    const EVoxelType Ground = GetVoxelType(X, Y, Z - 1);
    return Ground != EVoxelType::Empty &&
           Ground != EVoxelType::Water &&
           Ground != EVoxelType::Lava;
}

// Fill the memory with data
void AVoxelChunk::GenerateVoxelData()
{
    const int32 PaddedXY = ChunkSize + 2;
    const int32 PaddedZ = ChunkHeight + 2;

    VoxelData.Init(EVoxelType::Empty, PaddedXY * PaddedXY * PaddedZ);
    SurfaceHeightMap.Init(0, PaddedXY * PaddedXY);

    const FVector ChunkWorldPos = GetActorLocation();
    constexpr float VoxelSize = 100.0f;

    // Loop from -1 to ChunkSize (inclusive) to generate borders
    for (int32 X = -1; X <= ChunkSize; X++)
    {
        for (int32 Y = -1; Y <= ChunkSize; Y++)
        {
            const float WorldX = ChunkWorldPos.X + (X * VoxelSize);
            const float WorldY = ChunkWorldPos.Y + (Y * VoxelSize);

            // 1. DETERMINE BIOME (Returns a value roughly between -1.0 and 1.0)
            const float BiomeVal = BiomeNoise.GetNoise(WorldX, WorldY);

            // Fetch the base surface noise [0.0 to 1.0] to be manipulated by the biome
            const float RawSurfaceNoise = (SurfaceNoise.GetNoise(WorldX, WorldY) + 1.0f) / 2.0f;

            int32 SurfaceZ = 0;
            EVoxelType SurfaceBlock = EVoxelType::Grass;
            EVoxelType SubSurfaceBlock = EVoxelType::Dirt;

            // 2. BIOME OVER TERRAIN LOGIC
            if (BiomeVal < -0.3f)
            {
                // BIOME A: Oceans / Swamps
                // Flat terrain, set entirely below or right at Sea Level.
                SurfaceZ = FMath::RoundToInt(RawSurfaceNoise * 4.0f) + (SeaLevel - 2);
                SurfaceBlock = EVoxelType::Dirt; // Mud/Sand
                SubSurfaceBlock = EVoxelType::Dirt;
            }
            else if (BiomeVal < 0.4f)
            {
                // BIOME B: Grassy Plains / Hills
                // Gentle exponent, standard height variation.
                const float PlainsHeight = FMath::Pow(RawSurfaceNoise, 1.5f);
                SurfaceZ = FMath::RoundToInt(PlainsHeight * 20.0f) + BaseTerrainHeight;
                SurfaceBlock = EVoxelType::Grass;
                SubSurfaceBlock = EVoxelType::Dirt;
            }
            else
            {
                // BIOME C: Extreme Mountains
                // High exponent for sharp peaks, heavily elevated base height.
                const float MountainHeight = FMath::Pow(RawSurfaceNoise, 3.0f);
                SurfaceZ = FMath::RoundToInt(MountainHeight * 45.0f) + BaseTerrainHeight + 10;
                SurfaceBlock = EVoxelType::Stone; // Bare rock peaks
                SubSurfaceBlock = EVoxelType::Stone;
            }

            // GEOGRAPHICAL BASIN CARVING (LAKES)
            // We evaluate basin noise (0.0 to 1.0). If it crosses a threshold, we dig.
            const float RawBasinNoise = (BasinNoise.GetNoise(WorldX, WorldY) + 1.0f) / 2.0f;
            if (RawBasinNoise > 0.7f) // The higher the threshold, the rarer the lakes
            {
                // Smoothly depress the terrain.
                // A value of 0.7 gives 0 depth. A value of 1.0 gives MaxDepth.
                const float DepthRatio = (RawBasinNoise - 0.7f) / 0.3f;
                constexpr int32 MaxBasinDepth = 15;

                SurfaceZ -= FMath::RoundToInt(DepthRatio * MaxBasinDepth);

                // Ensure the basin floor is dirt/mud, not grass
                SurfaceBlock = EVoxelType::Dirt;
            }

            // Cache the final terrain height for decorators later
            SurfaceHeightMap[GetColumnIndex(X, Y)] = SurfaceZ;

            // 3. FILL THE VERTICAL COLUMN -- Loop Z from -1 to ChunkHeight
            for (int32 Z = -1; Z <= ChunkHeight; Z++)
            {
                const int32 VoxelIndex = GetVoxelIndex(X, Y, Z);
                const float WorldZ = ChunkWorldPos.Z + (Z * VoxelSize);

                // THE GLOBAL WATER TABLE
                if (Z > SurfaceZ)
                {
                    // If we are above the ground, but at or below Sea Level, it is definitively water.
                    // This naturally fills Oceans AND our newly carved Inland Basins.
                    if (Z <= SeaLevel)
                    {
                        VoxelData[VoxelIndex] = EVoxelType::Water;
                    }
                    continue; // Skip the rest of the solid block logic
                }

                // Apply Biome-specific Stratification
                EVoxelType CurrentBlock = EVoxelType::Stone;

                if (Z == SurfaceZ)
                {
                    CurrentBlock = SurfaceBlock;
                }
                else if (Z < SurfaceZ && Z >= SurfaceZ - 4)
                {
                    CurrentBlock = SubSurfaceBlock;
                }

                // RULE 6: Organic Cave Generation (Depth Attenuated)
                // We only want caves to start appearing well below the surface layer.
                constexpr int32 CaveRoofDepth = 8; // How many solid blocks must exist below the surface
                if (Z <= SurfaceZ - CaveRoofDepth)
                {
                    // Get the base cellular noise value (-1.0 to 1.0)
                    const float CaveNoiseVal = CaveNoise.GetNoise(WorldX, WorldY, WorldZ);

                    // DEPTH ATTENUATION:
                    // We calculate a ratio: 0.0 at the absolute bottom of the chunk, up to 1.0 at the CaveRoof boundary.
                    const float DepthRatio = static_cast<float>(Z) / static_cast<float>(SurfaceZ - CaveRoofDepth);

                    // We "harden" the rock as we get higher.
                    // FMath::Pow(DepthRatio, 2.0f) means it gets solid very quickly near the roof.
                    const float HardnessMultiplier = FMath::Pow(DepthRatio, 2.0f);

                    // The higher we are, the more we artificially inflate the noise value, making it harder to drop below CaveThreshold.
                    const float AttenuatedNoise = CaveNoiseVal + (HardnessMultiplier * 1.5f);

                    if (AttenuatedNoise < CaveThreshold)
                    {
                        CurrentBlock = EVoxelType::Empty; // Carve the cave
                    }
                    else if (CurrentBlock == EVoxelType::Stone)
                    {
                        // --- Mineral Spawning Logic ---
                        const float AbsoluteDepthRatio = 1.0f - (static_cast<float>(Z) / static_cast<float>(ChunkSize));
                        const float MineralRoll = FMath::FRand();

                        if (AbsoluteDepthRatio > 0.8f && MineralRoll < 0.02f) {
                            CurrentBlock = EVoxelType::Diamond;
                        } else if (AbsoluteDepthRatio > 0.3f && MineralRoll < 0.05f) {
                            CurrentBlock = EVoxelType::Coal;
                        }
                    }
                }

                VoxelData[VoxelIndex] = CurrentBlock;
            }
        }
    }

    // ==========================================
    // DECORATOR PASS (Structures & Flora)
    // ==========================================
    // Because a lake changes the shape of the ground, we must generate Lakes before we generate Trees.
    // If we place trees first, a lake might carve away the dirt underneath a tree, leaving floating wood in the sky.)
    // World-Altering Pass (Lakes, Caves)
    for (int32 X = 0; X < ChunkSize; X++) {
        for (int32 Y = 0; Y < ChunkSize; Y++) {
            for (int32 Z = ChunkSize - 1; Z >= 0; Z--) {
                const EVoxelType CurrentType = GetVoxelType(X, Y, Z);

                // Surface Structures
                if (CurrentType == EVoxelType::Grass) {
                    const float Roll = FMath::FRand();
                    // if (LakeAsset && Roll < 0.005f) {
                    //     CarveLake(X, Y, 8, 5); // Radius = 8, MaxDepth = 5
                    // }
                    if (CaveEntranceAsset && Roll > 0.995f) {
                        PasteStructure(CaveEntranceAsset->GenerateStructure(), X, Y, Z + 2, true);
                    }
                    break; // Surface found, move to next column
                }

                // Sub-Surface Structures
                if (CurrentType == EVoxelType::Stone && DungeonRoomAsset) {
                    // Only spawn in the mid-depth range
                    if (Z > 10 && Z < 25 && FMath::FRand() < 0.0005f) {
                        PasteStructure(DungeonRoomAsset->GenerateStructure(), X, Y, Z, true);
                    }
                }
            }
        }
    }


    // ==========================================
    // MICRO-DRESSERS (Trees, Flora, Stalactites)
    // ==========================================
    // We scan again because the Lake/Cave pass might have changed Grass to Water or Air.
    if (TreeAsset || StalactiteAsset) {
        for (int32 X = 0; X < ChunkSize; X++) {
            for (int32 Y = 0; Y < ChunkSize; Y++) {
                for (int32 Z = ChunkSize - 1; Z >= 0; Z--) {
                    const EVoxelType CurrentType = GetVoxelType(X, Y, Z);

                    if (CurrentType == EVoxelType::Grass && TreeAsset) {
                        // Ensure the grass block has solid ground below it
                        if (IsStableGround(X, Y, Z) && FMath::FRand() < TreeSpawnChance) {
                            PasteStructure(TreeAsset->GenerateStructure(), X, Y, Z + 1, false);
                        }
                        break;
                    }
                    else if (CurrentType == EVoxelType::Stone) {
                        // LAVA POOLS (Deep floor check)
                        if (LavaPoolAsset && Z < 10 && GetVoxelType(X, Y, Z + 1) == EVoxelType::Empty) {
                            if (FMath::FRand() < 0.005f) {
                                PasteStructure(LavaPoolAsset->GenerateStructure(), X, Y, Z, true);
                            }
                        }
                        // STALACTITES (Ceiling check)
                        if (StalactiteAsset && Z > 5 && GetVoxelType(X, Y, Z - 1) == EVoxelType::Empty) {
                            if (FMath::FRand() < 0.03f) {
                                PasteStructure(StalactiteAsset->GenerateStructure(), X, Y, Z - 4, false);
                            }
                        }
                    }
                }
            }
        }
    }
}

// The Naive Surface Mesher
void AVoxelChunk::GenerateMesh()
{
    AsyncMeshData.Vertices.Empty();
    AsyncMeshData.Triangles.Empty();
    AsyncMeshData.Normals.Empty();
    AsyncMeshData.UVs.Empty();
    AsyncMeshData.Colors.Empty();

    int32 VertexCount = 0;

    for (int32 FaceDirection = 0; FaceDirection < 6; FaceDirection++)
    {
        const bool bIsZAxis = (FaceDirection == 0 || FaceDirection == 1);
        const bool bIsXAxis = (FaceDirection == 2 || FaceDirection == 3);
        const bool bIsYAxis = (FaceDirection == 4 || FaceDirection == 5);

        // DYNAMIC LIMITS: Transforms the 16x16x128 pillar based on orientation
        const int32 SliceLimit = bIsZAxis ? ChunkHeight : ChunkSize;
        const int32 ULimit = ChunkSize;
        const int32 VLimit = bIsZAxis ? ChunkSize : ChunkHeight;

        for (int32 Slice = 0; Slice < SliceLimit; Slice++)
        {
            TArray<EVoxelType> Mask;
            Mask.Init(EVoxelType::Empty, ULimit * VLimit);

            for (int32 V = 0; V < VLimit; V++)
            {
                for (int32 U = 0; U < ULimit; U++)
                {
                    const int32 X = bIsXAxis ? Slice : U;
                    const int32 Y = bIsYAxis ? Slice : (bIsXAxis ? U : V);
                    const int32 Z = bIsZAxis ? Slice : V;

                    const EVoxelType CurrentBlock = GetVoxelType(X, Y, Z);

                    if (CurrentBlock != EVoxelType::Empty)
                    {
                        const int32 NeighborX = X + FaceOffsets[FaceDirection].X;
                        const int32 NeighborY = Y + FaceOffsets[FaceDirection].Y;
                        const int32 NeighborZ = Z + FaceOffsets[FaceDirection].Z;

                        if (GetVoxelType(NeighborX, NeighborY, NeighborZ) == EVoxelType::Empty)
                        {
                            Mask[U + (V * ULimit)] = CurrentBlock;
                        }
                    }
                }
            }

            // The Greedy Sweep
            for (int32 V = 0; V < VLimit; V++)
            {
                for (int32 U = 0; U < ULimit; U++)
                {
                    const EVoxelType MaskType = Mask[U + (V * ULimit)];

                    if (MaskType != EVoxelType::Empty)
                    {
                        int32 Width = 1;
                        int32 Height = 1;

                        while (U + Width < ULimit && Mask[(U + Width) + (V * ULimit)] == MaskType) Width++;

                        bool bDone = false;
                        while (V + Height < VLimit && !bDone)
                        {
                            for (int32 W = 0; W < Width; W++)
                            {
                                if (Mask[(U + W) + ((V + Height) * ULimit)] != MaskType)
                                {
                                    bDone = true;
                                    break;
                                }
                            }
                            if (!bDone) Height++;
                        }

                        const int32 StartX = bIsXAxis ? Slice : U;
                        const int32 StartY = bIsYAxis ? Slice : (bIsXAxis ? U : V);
                        const int32 StartZ = bIsZAxis ? Slice : V;
                        FVector BlockPos(StartX * 100.0f, StartY * 100.0f, StartZ * 100.0f);

                        AddFace(AsyncMeshData.Vertices, AsyncMeshData.Triangles, AsyncMeshData.Normals,
                                AsyncMeshData.UVs, AsyncMeshData.Colors, VertexCount,
                                BlockPos, FaceDirection, Width, Height, MaskType);

                        for (int32 y = 0; y < Height; y++)
                        {
                            for (int32 x = 0; x < Width; x++)
                            {
                                Mask[(U + x) + ((V + y) * ULimit)] = EVoxelType::Empty;
                            }
                        }
                    }
                }
            }
        }
    }
}

void AVoxelChunk::AddFace(
    TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs,
    TArray<FLinearColor>& VertexColors, int32& VertexCount, const FVector& BlockPos, const int32 FaceIndex,
    const int32 Width, const int32 Height, const EVoxelType BlockType) const
{
    // 1. Calculate the scaled dimensions for this specific face
    float X_Scale = 1.0f, Y_Scale = 1.0f, Z_Scale = 1.0f;

    switch (FaceIndex)
    {
        case 0: case 1: default:// Z-Axis (Top/Bottom)
            X_Scale = Width; Y_Scale = Height; break;
        case 2: case 3: // X-Axis (Front/Back)
            Y_Scale = Width; Z_Scale = Height; break;
        case 4: case 5: // Y-Axis (Right/Left)
            X_Scale = Width; Z_Scale = Height; break;
    }

    // 2. Convert our scales into actual Unreal Engine distance (100cm blocks)
    const FVector ScaleVec(X_Scale * 100.0f, Y_Scale * 100.0f, Z_Scale * 100.0f);

    // 3. Query the DataTable for Color/Atlas Data
    FLinearColor FaceColor = FLinearColor::Gray; // Default fallback

    if (const FVoxelBlockData* Data = GetBlockData(BlockType))
    {
        if (bUseSolidColors)
        {
            // THE LEGO LOOK: Just pass the direct color from the DataTable
            FaceColor = Data->BlockColor;
        }
        else
        {
            // TEXTURE ATLAS: Compute the X/Y offset
            const FVector2D AtlasCoord = Data->AtlasCoordinate;
            const float TextureBlockSize = Data->TextureBlockSize;

            FaceColor = FLinearColor(
                AtlasCoord.X / TextureBlockSize,
                AtlasCoord.Y / TextureBlockSize,
                0.0f,
                1.0f
            );
        }
    }

    // 4. Append Vertices, Normals, and Colors
    for (int i = 0; i < 4; i++)
    {
        Vertices.Add(BlockPos + (VertexBlock[FaceVertices[FaceIndex][i]] * ScaleVec));
        Normals.Add(FaceNormals[FaceIndex]);
        VertexColors.Add(FaceColor); // GPU receives either the raw color or the atlas coordinate
    }

    // 5. Scaled UV Mapping based on the lookup table's specific order
    UVs.Add(FVector2D(0, Height));      // Bottom-Left
    UVs.Add(FVector2D(0, 0));           // Top-Left
    UVs.Add(FVector2D(Width, 0));       // Top-Right
    UVs.Add(FVector2D(Width, Height));  // Bottom-Right

    // 6. reversed Clockwise Triangles!
    Triangles.Add(VertexCount + 0);
    Triangles.Add(VertexCount + 2);
    Triangles.Add(VertexCount + 1);

    Triangles.Add(VertexCount + 0);
    Triangles.Add(VertexCount + 3);
    Triangles.Add(VertexCount + 2);

    VertexCount += 4;
}


const FVoxelBlockData* AVoxelChunk::GetBlockData(EVoxelType BlockType) const
{
    // THREAD-SAFE: Read purely from our local C++ cache. No UObjects touched!
    return BlockDataCache.Find(BlockType);
}

void AVoxelChunk::PasteStructure(const FVoxelStructure& Structure, int32 RootX, int32 RootY, int32 RootZ, bool bCanOverwriteSolid)
{
    // Offset X and Y so the "Root" passed into the function is exactly the center of the bottom layer
    const int32 OffsetX = Structure.SizeX / 2;
    const int32 OffsetY = Structure.SizeY / 2;

    for (int32 x = 0; x < Structure.SizeX; x++) {
        for (int32 y = 0; y < Structure.SizeY; y++) {
            for (int32 z = 0; z < Structure.SizeZ; z++) {

                const EVoxelType BlockToPlace = Structure.GetBlock(x, y, z);

                if (BlockToPlace != EVoxelType::Ignore) {
                    const int32 TargetX = RootX + x - OffsetX;
                    const int32 TargetY = RootY + y - OffsetY;
                    const int32 TargetZ = RootZ + z;

                    // BOUNDARY CHECK: Prevent spilling over the chunk edge!
                    if (TargetX >= 0 && TargetX < ChunkSize &&
                        TargetY >= 0 && TargetY < ChunkSize &&
                        TargetZ >= 0 && TargetZ < ChunkSize)
                    {
                        const int32 TargetIndex = GetVoxelIndex(TargetX, TargetY, TargetZ);
                        const EVoxelType ExistingBlock = VoxelData[TargetIndex];

                        // Only overwrite if it's air, OR if we force it to carve into solid ground
                        if (ExistingBlock == EVoxelType::Empty || bCanOverwriteSolid) {
                            VoxelData[TargetIndex] = BlockToPlace;
                        }
                    }
                }
            }
        }
    }
}

void AVoxelChunk::CarveLake(int32 CenterX, int32 CenterY, int32 Radius, int32 MaxDepth)
{
    // 1. Find the actual ground height at the lake's center
      const int32 SurfaceZ = FindTopSolidZ(CenterX, CenterY);
      if (SurfaceZ < 0) return; // No solid ground, cannot carve

      // 2. Define the lake basin as a parabolic depression
      for (int32 dx = -Radius; dx <= Radius; dx++)
      {
          for (int32 dy = -Radius; dy <= Radius; dy++)
          {
              const int32 X = CenterX + dx;
              const int32 Y = CenterY + dy;

              // 2D radial distance from lake center
              const float Dist2D = FMath::Sqrt(
              FMath::Square(static_cast<float>(dx)) + FMath::Square(static_cast<float>(dy))
              );
              if (Dist2D > Radius) continue; // Outside the circular lake area

              // Parabolic shape: deepest at center, shallow at edges
              const float DepthRatio = 1.0f - (Dist2D / Radius);
              const int32 CarveDepth = FMath::RoundToInt(DepthRatio * MaxDepth);

              // Lake bottom sits 'CarveDepth' blocks below the surface
              const int32 LakeBottomZ = SurfaceZ - CarveDepth;
              const int32 WaterSurfaceZ = LakeBottomZ + 1; // One block of water above bottom

              // 3. Carve the column from surface down to lake bottom
              for (int32 Z = SurfaceZ; Z >= LakeBottomZ; Z--)
              {
                  if (Z < 0 || Z >= ChunkSize) continue;

                  const int32 Index = GetVoxelIndex(X, Y, Z);
                  const EVoxelType Existing = VoxelData[Index];

                  // At the very bottom, place dirt as the lake bed
                  if (Z == LakeBottomZ)
                  {
                      VoxelData[Index] = EVoxelType::Dirt;
                  }
                  // Above the bottom, fill with water
                  else if (Z <= WaterSurfaceZ && Z > LakeBottomZ)
                  {
                      VoxelData[Index] = EVoxelType::Water;
                  }
                  // Remove any solid blocks in between (carve the bowl)
                  else if (Existing != EVoxelType::Empty)
                  {
                      VoxelData[Index] = EVoxelType::Empty;
                  }
              }

              // 4. Mark this column as part of a lake (prevents random water cubes)
              if (X >= 0 && X < ChunkSize && Y >= 0 && Y < ChunkSize)
              {
                  LakeMask[X + (Y * ChunkSize)] = true;
              }
          }
      }
}
