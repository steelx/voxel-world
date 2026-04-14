// Copyright Ajinkya Borade 2026 (c).


#include "VoxelChunk.h"

#include "ProceduralMeshComponent.h"


// Sets default values
AVoxelChunk::AVoxelChunk()
{
	MeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMeshComp"));
    SetRootComponent(MeshComponent);
}

// Called when the game starts or when spawned
void AVoxelChunk::BeginPlay()
{
	Super::BeginPlay();
    GenerateVoxelData();
    GenerateMesh();
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

// Step 1: Fill the memory with data
void AVoxelChunk::GenerateVoxelData()
{
    // Reserve memory upfront to prevent reallocation overhead
    VoxelData.Init(EVoxelType::Empty, ChunkSize * ChunkSize * ChunkSize);

    // Simple generation: Fill the bottom half with Dirt
    for (int32 Z = 0; Z < ChunkSize; Z++)
    {
        for (int32 Y = 0; Y < ChunkSize; Y++)
        {
            for (int32 X = 0; X < ChunkSize; X++)
            {
                if (Z < ChunkSize / 2)
                {
                    VoxelData[GetVoxelIndex(X, Y, Z)] = EVoxelType::Dirt;
                }
            }
        }
    }
}

// Step 2: The Naive Surface Mesher
void AVoxelChunk::GenerateMesh()
{
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    int32 VertexCount = 0;

    // Iterate through every single voxel
    for (int32 Z = 0; Z < ChunkSize; Z++)
    {
        for (int32 Y = 0; Y < ChunkSize; Y++)
        {
            for (int32 X = 0; X < ChunkSize; X++)
            {
                if (GetVoxelType(X, Y, Z) == EVoxelType::Empty) continue;

                // Scale factor: 1 Unreal Unit = 1cm. A 1m block is 100 units.
                FVector BlockPos(X * 100, Y * 100, Z * 100);

                // Check Top Face Neighbor (Z + 1)
                if (GetVoxelType(X, Y, Z + 1) == EVoxelType::Empty)
                {
                    // Generate the 4 corners of the top face
                    Vertices.Add(BlockPos + FVector(0, 0, 100));     // 0: Bottom-Left
                    Vertices.Add(BlockPos + FVector(100, 0, 100));   // 1: Bottom-Right
                    Vertices.Add(BlockPos + FVector(0, 100, 100));   // 2: Top-Left
                    Vertices.Add(BlockPos + FVector(100, 100, 100)); // 3: Top-Right

                    // Generate the 2 triangles using proper Clockwise winding order
                    Triangles.Add(VertexCount + 0);
                    Triangles.Add(VertexCount + 2);
                    Triangles.Add(VertexCount + 1);

                    Triangles.Add(VertexCount + 1);
                    Triangles.Add(VertexCount + 2);
                    Triangles.Add(VertexCount + 3);

                    VertexCount += 4;
                }

                // Repeat identical logic for Bottom, North, South, East, West faces
                // Check Bottom Face Neighbor (Z - 1)
                if (GetVoxelType(X, Y, Z - 1) == EVoxelType::Empty)
                {
                    // Generate the 4 corners of the bottom face
                    Vertices.Add(BlockPos + FVector(0, 0, 0));     // 0: Bottom-Left
                    Vertices.Add(BlockPos + FVector(0, 100, 0));   // 1: Top-Left
                    Vertices.Add(BlockPos + FVector(100, 0, 0));   // 2: Bottom-Right
                    Vertices.Add(BlockPos + FVector(100, 100, 0)); // 3: Top-Right

                    // Generate the 2 triangles using proper Clockwise winding order
                    Triangles.Add(VertexCount + 0);
                    Triangles.Add(VertexCount + 1);
                    Triangles.Add(VertexCount + 2);

                    Triangles.Add(VertexCount + 2);
                    Triangles.Add(VertexCount + 1);
                    Triangles.Add(VertexCount + 3);

                    VertexCount += 4;
                }

                // Check North Face Neighbor (Y + 1)
                if (GetVoxelType(X, Y + 1, Z) == EVoxelType::Empty)
                {
                    // Generate the 4 corners of the north face
                    Vertices.Add(BlockPos + FVector(0, 100, 0));     // 0: Bottom-Left
                    Vertices.Add(BlockPos + FVector(0, 100, 100));   // 1: Top-Left
                    Vertices.Add(BlockPos + FVector(100, 100, 0));   // 2: Bottom-Right
                    Vertices.Add(BlockPos + FVector(100, 100, 100)); // 3: Top-Right

                    // Generate the 2 triangles using proper Clockwise winding order
                    Triangles.Add(VertexCount + 0);
                    Triangles.Add(VertexCount + 1);
                    Triangles.Add(VertexCount + 2);

                    Triangles.Add(VertexCount + 2);
                    Triangles.Add(VertexCount + 1);
                    Triangles.Add(VertexCount + 3);

                    VertexCount += 4;
                }

                // Check South Face Neighbor (Y - 1)
                if (GetVoxelType(X, Y - 1, Z) == EVoxelType::Empty)
                {
                    // Generate the 4 corners of the south face
                    Vertices.Add(BlockPos + FVector(0, 0, 0));     // 0: Bottom-Left
                    Vertices.Add(BlockPos + FVector(100, 0, 0));   // 1: Bottom-Right
                    Vertices.Add(BlockPos + FVector(0, 0, 100));   // 2: Top-Left
                    Vertices.Add(BlockPos + FVector(100, 0, 100)); // 3: Top-Right

                    // Generate the 2 triangles using proper Clockwise winding order
                    Triangles.Add(VertexCount + 0);
                    Triangles.Add(VertexCount + 2);
                    Triangles.Add(VertexCount + 1);

                    Triangles.Add(VertexCount + 1);
                    Triangles.Add(VertexCount + 2);
                    Triangles.Add(VertexCount + 3);

                    VertexCount += 4;
                }

                // Check East Face Neighbor (X + 1)
                if (GetVoxelType(X + 1, Y, Z) == EVoxelType::Empty)
                {
                    // Generate the 4 corners of the east face
                    Vertices.Add(BlockPos + FVector(100, 0, 0));     // 0: Bottom-Left
                    Vertices.Add(BlockPos + FVector(100, 0, 100));   // 1: Top-Left
                    Vertices.Add(BlockPos + FVector(100, 100, 0));   // 2: Bottom-Right
                    Vertices.Add(BlockPos + FVector(100, 100, 100)); // 3: Top-Right

                    // Generate the 2 triangles using proper Clockwise winding order
                    Triangles.Add(VertexCount + 0);
                    Triangles.Add(VertexCount + 1);
                    Triangles.Add(VertexCount + 2);

                    Triangles.Add(VertexCount + 2);
                    Triangles.Add(VertexCount + 1);
                    Triangles.Add(VertexCount + 3);

                    VertexCount += 4;
                }

                // Check West Face Neighbor (X - 1)
                if (GetVoxelType(X - 1, Y, Z) == EVoxelType::Empty)
                {
                    // Generate the 4 corners of the west face
                    Vertices.Add(BlockPos + FVector(0, 0, 0));     // 0: Bottom-Left
                    Vertices.Add(BlockPos + FVector(0, 100, 0));   // 1: Bottom-Right
                    Vertices.Add(BlockPos + FVector(0, 0, 100));   // 2: Top-Left
                    Vertices.Add(BlockPos + FVector(0, 100, 100)); // 3: Top-Right

                    // Generate the 2 triangles using proper Clockwise winding order
                    Triangles.Add(VertexCount + 0);
                    Triangles.Add(VertexCount + 2);
                    Triangles.Add(VertexCount + 1);

                    Triangles.Add(VertexCount + 1);
                    Triangles.Add(VertexCount + 2);
                    Triangles.Add(VertexCount + 3);

                    VertexCount += 4;
                }
            }
        }
    }

    // Ship the arrays directly to the GPU!
    MeshComponent->CreateMeshSection_LinearColor(0, Vertices, Triangles, TArray<FVector>(), TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), true);
}

void AVoxelChunk::AddFace(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs, int32& VertexCount, FVector BlockPos, int32 FaceIndex)
{
    constexpr float Size = 100.0f; // One Unreal Unit = 1cm. 100 = 1 Meter Block.

    // The 8 corners of our local voxel cube
    // We define every face's vertices in the exact same logical order:
    // v0: Bottom-Left, v1: Bottom-Right, v2: Top-Left, v3: Top-Right
    FVector V0, V1, V2, V3, Normal;

    switch (FaceIndex)
    {
    default:
    case 0: // Top (Z+) - Looking down at it
        V0 = FVector(0, 0, Size); V1 = FVector(Size, 0, Size); V2 = FVector(0, Size, Size); V3 = FVector(Size, Size, Size);
        Normal = FVector(0, 0, 1);
        break;
    case 1: // Bottom (Z-) - Looking up at it
        V0 = FVector(Size, 0, 0); V1 = FVector(Size, Size, 0); V2 = FVector(0, 0, 0); V3 = FVector(0, Size, 0);
        Normal = FVector(0, 0, -1);
        break;
    case 2: // Front (X+) - Looking back at -X
        V0 = FVector(Size, Size, 0); V1 = FVector(Size, 0, 0); V2 = FVector(Size, Size, Size); V3 = FVector(Size, 0, Size);
        Normal = FVector(1, 0, 0);
        break;
    case 3: // Back (X-) - Looking forward at +X
        V0 = FVector(0, 0, 0); V1 = FVector(0, Size, 0); V2 = FVector(0, 0, Size); V3 = FVector(0, Size, Size);
        Normal = FVector(-1, 0, 0);
        break;
    case 4: // Right (Y+) - Looking left at -Y
        V0 = FVector(0, Size, 0); V1 = FVector(Size, Size, 0); V2 = FVector(0, Size, Size); V3 = FVector(Size, Size, Size);
        Normal = FVector(0, 1, 0);
        break;
    case 5: // Left (Y-) - Looking right at +Y
        V0 = FVector(Size, 0, 0); V1 = FVector(0, 0, 0); V2 = FVector(Size, 0, Size); V3 = FVector(0, 0, Size);
        Normal = FVector(0, -1, 0);
        break;
    }

    // Append Vertices
    Vertices.Add(BlockPos + V0);
    Vertices.Add(BlockPos + V1);
    Vertices.Add(BlockPos + V2);
    Vertices.Add(BlockPos + V3);

    // Append Normals (Crucial for lighting to interact with the block)
    for(int i = 0; i < 4; i++) Normals.Add(Normal);

    // Append UVs (Maps a standard texture to the square face)
    UVs.Add(FVector2D(0, 0));// Bottom Left
    UVs.Add(FVector2D(1, 0));// Bottom Right
    UVs.Add(FVector2D(0, 1));// Top Left
    UVs.Add(FVector2D(1, 1));// Top Right

    // Append Triangles (Creating 2 triangles from 4 vertices using Clockwise winding)
    Triangles.Add(VertexCount + 0);
    Triangles.Add(VertexCount + 2);
    Triangles.Add(VertexCount + 1);

    Triangles.Add(VertexCount + 1);
    Triangles.Add(VertexCount + 2);
    Triangles.Add(VertexCount + 3);

    VertexCount += 4;

#if WITH_EDITOR
    if (bShowDebugFaces)
    {
        int32 GridX = FMath::RoundToInt(BlockPos.X / 100.0f);
        int32 GridY = FMath::RoundToInt(BlockPos.Y / 100.0f);
        int32 GridZ = FMath::RoundToInt(BlockPos.Z / 100.0f);

        if (GridX == DebugVoxelCoordinate.X && GridY == DebugVoxelCoordinate.Y && GridZ == DebugVoxelCoordinate.Z)
        {
            // FIX: Convert Local BlockPos into Global World coordinates so the debugger actually hits your mesh!
            FVector WorldFaceCenter = GetActorLocation() + BlockPos + ((V0 + V1 + V2 + V3) / 4.0f);

            DrawDebugDirectionalArrow(GetWorld(), WorldFaceCenter, WorldFaceCenter + (Normal * 60.0f), 10.0f, FColor::Yellow, true, -1.0f, 0, 2.0f);

            FString FaceName;
            switch(FaceIndex)
            {
            case 0: FaceName = TEXT("Top (Z+)"); break;
            case 1: FaceName = TEXT("Bottom (Z-)"); break;
            case 2: FaceName = TEXT("Front (X+)"); break;
            case 3: FaceName = TEXT("Back (X-)"); break;
            case 4: FaceName = TEXT("Right (Y+)"); break;
            case 5: FaceName = TEXT("Left (Y-)"); break;
            }

            DrawDebugString(GetWorld(), WorldFaceCenter + (Normal * 80.0f), FaceName, nullptr, FColor::White, 1000.0f, false, 1.2f);
        }
    }
#endif
}
