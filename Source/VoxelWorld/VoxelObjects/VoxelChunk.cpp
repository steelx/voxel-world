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

// Fill the memory with data
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

// The Naive Surface Mesher
void AVoxelChunk::GenerateMesh()
{
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    int32 VertexCount = 0;

    for (int32 Z = 0; Z < ChunkSize; Z++)
    {
        for (int32 Y = 0; Y < ChunkSize; Y++)
        {
            for (int32 X = 0; X < ChunkSize; X++)
            {
                if (GetVoxelType(X, Y, Z) == EVoxelType::Empty) continue;

                FVector BlockPos(X * 100.0f, Y * 100.0f, Z * 100.0f);

                // Check all 6 adjacent blocks using the lookup table
                for (int32 FaceIndex = 0; FaceIndex < 6; FaceIndex++)
                {
                    int32 NeighborX = X + FaceOffsets[FaceIndex].X;
                    int32 NeighborY = Y + FaceOffsets[FaceIndex].Y;
                    int32 NeighborZ = Z + FaceOffsets[FaceIndex].Z;

                    // If the neighbor is Air (or out of bounds), generate the face
                    if (GetVoxelType(NeighborX, NeighborY, NeighborZ) == EVoxelType::Empty)
                    {
                        AddFace(Vertices, Triangles, Normals, UVs, VertexCount, BlockPos, FaceIndex);
                    }
                }
            }
        }
    }

    // Ship the calculated geometry to the GPU, including Normals and UVs
    MeshComponent->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, TArray<FLinearColor>(), TArray<FProcMeshTangent>(), true);
}

void AVoxelChunk::AddFace(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs, int32& VertexCount, FVector BlockPos, int32 FaceIndex)
{
    // 1. Add the 4 Vertices & Normals for this specific face
    for (int i = 0; i < 4; i++)
    {
        Vertices.Add(BlockPos + V[FaceVertices[FaceIndex][i]]);
        Normals.Add(FaceNormals[FaceIndex]);
    }

    // 2. Add standard UV Mapping
    UVs.Add(FVector2D(0, 1)); // Bottom-Left
    UVs.Add(FVector2D(0, 0)); // Top-Left
    UVs.Add(FVector2D(1, 0)); // Top-Right
    UVs.Add(FVector2D(1, 1)); // Bottom-Right

    // 3. Add 6 Triangle Indices (Creating 2 perfectly clockwise triangles)
    Triangles.Add(VertexCount + 0);
    Triangles.Add(VertexCount + 2);
    Triangles.Add(VertexCount + 1);

    Triangles.Add(VertexCount + 0);
    Triangles.Add(VertexCount + 3);
    Triangles.Add(VertexCount + 2);

    VertexCount += 4;
}
