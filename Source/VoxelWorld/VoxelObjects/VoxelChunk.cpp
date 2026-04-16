// Copyright Ajinkya Borade 2026 (c).


#include "VoxelChunk.h"

#include "ProceduralMeshComponent.h"


// Sets default values
AVoxelChunk::AVoxelChunk()
{
	MeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMeshComp"));
    SetRootComponent(MeshComponent);
    // Crucial for characters to walk on procedural meshes without falling!
    MeshComponent->bUseComplexAsSimpleCollision = true;
}

void AVoxelChunk::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    RandomizeSeed();
}

void AVoxelChunk::RandomizeSeed()
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

    // Generate a random 32-bit integer
    const int32 NewSeed = FMath::Rand();

    // Apply it to both noise generators
    SurfaceNoise.SetSeed(NewSeed);
    CaveNoise.SetSeed(NewSeed);
    BiomeNoise.SetSeed(NewSeed);

    // Regenerate the world
    GenerateVoxelData();
    GenerateMesh();
}

void AVoxelChunk::BeginPlay()
{
    Super::BeginPlay();
    RandomizeSeed();
}

// 1D Flattening Math
int32 AVoxelChunk::GetVoxelIndex(const int32 X, const int32 Y, const int32 Z) const
{
    // The golden formula for 1D Array (https://eloquentjavascript.net/2nd_edition/07_elife.html)
    return X + (Y * ChunkSize) + (Z * ChunkSize * ChunkSize);
}

// Safe wrapper to prevent out-of-bounds array crashes
EVoxelType AVoxelChunk::GetVoxelType(const int32 X, const int32 Y, const int32 Z) const
{
    if (X < 0 || X >= ChunkSize || Y < 0 || Y >= ChunkSize || Z < 0 || Z >= ChunkSize)
        return EVoxelType::Empty; // Treat outside the chunk as air for now

    return VoxelData[GetVoxelIndex(X, Y, Z)];
}

// Fill the memory with data
void AVoxelChunk::GenerateVoxelData()
{
    VoxelData.Init(EVoxelType::Empty, ChunkSize * ChunkSize * ChunkSize);

    const FVector ChunkWorldPos = GetActorLocation();
    constexpr float VoxelSize = 100.0f;

    for (int32 X = 0; X < ChunkSize; X++)
    {
        for (int32 Y = 0; Y < ChunkSize; Y++)
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

            // 3. FILL THE VERTICAL COLUMN
            for (int32 Z = 0; Z < ChunkSize; Z++)
            {
                const int32 VoxelIndex = GetVoxelIndex(X, Y, Z);
                const float WorldZ = ChunkWorldPos.Z + (Z * VoxelSize);

                if (Z > SurfaceZ)
                {
                    if (Z <= SeaLevel) VoxelData[VoxelIndex] = EVoxelType::Water;
                    continue;
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

                // RULE 6: Cave Generation & Minerals
                if (Z <= SurfaceZ - 2)
                {
                    const float CaveNoiseVal = CaveNoise.GetNoise(WorldX, WorldY, WorldZ);
                    const float DepthPercentage = FMath::Clamp(static_cast<float>(SurfaceZ - Z) / 8.0f, 0.0f, 1.0f);

                    if (CaveNoiseVal + (1.0f - DepthPercentage) < CaveThreshold)
                    {
                        CurrentBlock = EVoxelType::Empty;
                    }
                    else if (CurrentBlock == EVoxelType::Stone)
                    {
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
    FVoxelStructure OakTree = MakeOakTree();

    for (int32 X = 0; X < ChunkSize; X++)
    {
        for (int32 Y = 0; Y < ChunkSize; Y++)
        {
            // Scan from the top of the sky downwards to find the surface
            for (int32 Z = ChunkSize - 1; Z >= 0; Z--)
            {
                int32 Index = GetVoxelIndex(X, Y, Z);

                if (VoxelData[Index] == EVoxelType::Grass)
                {
                    // We hit a grass block! Roll the 1.5% chance
                    if (FMath::FRand() < TreeSpawnChance)
                    {
                        // Paste the tree sitting ON TOP of the grass (Z + 1)
                        PasteStructure(OakTree, X, Y, Z + 1, false);
                    }
                    break; // Stop scanning this column once we hit the ground
                }
                if (VoxelData[Index] != EVoxelType::Empty && VoxelData[Index] != EVoxelType::Leaves)
                {
                    // We hit stone, water, or dirt instead of grass. Break.
                    break;
                }
            }
        }
    }
}

// The Naive Surface Mesher
void AVoxelChunk::GenerateMesh()
{
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FLinearColor> VertexColors;
    int32 VertexCount = 0;

    for (int32 FaceDirection = 0; FaceDirection < 6; FaceDirection++)
    {
        const bool bIsZAxis = (FaceDirection == 0 || FaceDirection == 1);
        const bool bIsXAxis = (FaceDirection == 2 || FaceDirection == 3);
        const bool bIsYAxis = (FaceDirection == 4 || FaceDirection == 5);

        for (int32 Slice = 0; Slice < ChunkSize; Slice++)
        {
            TArray<EVoxelType> Mask;
            Mask.Init(EVoxelType::Empty, ChunkSize * ChunkSize);

            for (int32 V = 0; V < ChunkSize; V++)
            {
                for (int32 U = 0; U < ChunkSize; U++)
                {
                    // THE FIX: Bulletproof 2D-to-3D axis mapping
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
                            Mask[U + (V * ChunkSize)] = CurrentBlock;
                        }
                    }
                }
            }

            // The Greedy Sweep
            for (int32 V = 0; V < ChunkSize; V++)
            {
                for (int32 U = 0; U < ChunkSize; U++)
                {
                    const EVoxelType MaskType = Mask[U + (V * ChunkSize)];

                    if (MaskType != EVoxelType::Empty)
                    {
                        int32 Width = 1;
                        int32 Height = 1;

                        while (U + Width < ChunkSize && Mask[(U + Width) + (V * ChunkSize)] == MaskType) Width++;

                        bool bDone = false;
                        while (V + Height < ChunkSize && !bDone)
                        {
                            for (int32 W = 0; W < Width; W++)
                            {
                                if (Mask[(U + W) + ((V + Height) * ChunkSize)] != MaskType)
                                {
                                    bDone = true;
                                    break;
                                }
                            }
                            if (!bDone) Height++;
                        }

                        // THE FIX: Match the Start positions perfectly with our clean axis mapping
                        const int32 StartX = bIsXAxis ? Slice : U;
                        const int32 StartY = bIsYAxis ? Slice : (bIsXAxis ? U : V);
                        const int32 StartZ = bIsZAxis ? Slice : V;
                        FVector BlockPos(StartX * 100.0f, StartY * 100.0f, StartZ * 100.0f);

                        AddFace(Vertices, Triangles, Normals, UVs, VertexColors, VertexCount, BlockPos, FaceDirection, Width, Height, MaskType);

                        for (int32 y = 0; y < Height; y++)
                        {
                            for (int32 x = 0; x < Width; x++)
                            {
                                Mask[(U + x) + ((V + y) * ChunkSize)] = EVoxelType::Empty;
                            }
                        }
                    }
                }
            }
        }
    }

    // --- MATERIAL ---
    if (bUseSolidColors && SolidMaterial)
    {
        MeshComponent->SetMaterial(0, SolidMaterial);
    }
    else if (!bUseSolidColors && VoxelMaterial)
    {
        MeshComponent->SetMaterial(0, VoxelMaterial);
    }

    MeshComponent->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertexColors, TArray<FProcMeshTangent>(), true);
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


FVoxelBlockData* AVoxelChunk::GetBlockData(EVoxelType BlockType) const
{
    // Reflection Magic: This safely extracts the raw text name of the Enum (e.g., "Dirt", "Grass")
    const FString EnumName = StaticEnum<EVoxelType>()->GetNameStringByValue(static_cast<int64>(BlockType));
    // Safety check: Don't query if the table is missing or the block is Air
    if (!BlockDataTable || BlockType == EVoxelType::Empty)
    {
        UE_LOG(LogTemp, Warning, TEXT("BlockDataTable is null or BlockType - %s - is Empty"), *EnumName);
        return nullptr;
    }

    // Search the DataTable for a Row Name that exactly matches our Enum name
    FVoxelBlockData* Result = BlockDataTable->FindRow<FVoxelBlockData>(FName(*EnumName), TEXT("VoxelDataLookup"));
    if (!Result)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GetBlockData] Could not find block data for: %s"), *EnumName);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[GetBlockData] Found block data for: %s, AtlasCoord: (%f, %f)"), *EnumName, Result->AtlasCoordinate.X, Result->AtlasCoordinate.Y);
    }

    return Result;
}

FVoxelStructure AVoxelChunk::MakeOakTree() const
{
    FVoxelStructure Tree;
    Tree.Init(5, 5, 7); // A 5x5 wide box, 7 blocks tall

    // 1. Build the Trunk (Center is X:2, Y:2)
    for (int32 Z = 0; Z < 5; Z++) {
        Tree.SetBlock(2, 2, Z, EVoxelType::Wood);
    }

    // 2. Build the Leaves (Spherical canopy)
    for (int32 X = 0; X < 5; X++) {
        for (int32 Y = 0; Y < 5; Y++) {
            for (int32 Z = 3; Z < 7; Z++) {
                // Don't overwrite the bottom of the trunk
                if (X == 2 && Y == 2 && Z < 5) continue;

                // Simple sphere check from center (2, 2, 4.5)
                const float DistSq = FMath::Square(X - 2.0f) + FMath::Square(Y - 2.0f) + FMath::Square(Z - 4.5f);
                if (DistSq <= 5.5f) {
                    Tree.SetBlock(X, Y, Z, EVoxelType::Leaves);
                }
            }
        }
    }
    return Tree;
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

                if (BlockToPlace != EVoxelType::Empty) {
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
