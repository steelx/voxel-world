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
                if (Z < ChunkSize)
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
                    int32 X = bIsXAxis ? Slice : U;
                    int32 Y = bIsYAxis ? Slice : (bIsXAxis ? U : V);
                    int32 Z = bIsZAxis ? Slice : V;

                    EVoxelType CurrentBlock = GetVoxelType(X, Y, Z);

                    if (CurrentBlock != EVoxelType::Empty)
                    {
                        int32 NeighborX = X + FaceOffsets[FaceDirection].X;
                        int32 NeighborY = Y + FaceOffsets[FaceDirection].Y;
                        int32 NeighborZ = Z + FaceOffsets[FaceDirection].Z;

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
                    EVoxelType MaskType = Mask[U + (V * ChunkSize)];

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

                        AddFace(Vertices, Triangles, Normals, UVs, VertexCount, BlockPos, FaceDirection, Width, Height);

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

    MeshComponent->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertexColors, TArray<FProcMeshTangent>(), true);
    if (VoxelMaterial)
    {
        MeshComponent->SetMaterial(0, VoxelMaterial);
    }
}

void AVoxelChunk::AddFace(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs, int32& VertexCount, const FVector& BlockPos, const int32 FaceIndex, const int32 Width, const int32 Height)
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

    // 4. Convert our scales into actual Unreal Engine distance (100cm blocks)
    FVector ScaleVec(X_Scale * 100.0f, Y_Scale * 100.0f, Z_Scale * 100.0f);

    // 5. Append Vertices and Normals
    for (int i = 0; i < 4; i++)
    {
        // We multiply the 0-1 unit corner by our Greedy Scale to stretch the face
        Vertices.Add(BlockPos + (VertexBlock[FaceVertices[FaceIndex][i]] * ScaleVec));
        Normals.Add(FaceNormals[FaceIndex]);
    }

    // 6. Scaled UV Mapping based on the lookup table's specific order
    UVs.Add(FVector2D(0, Height));      // Bottom-Left
    UVs.Add(FVector2D(0, 0));           // Top-Left
    UVs.Add(FVector2D(Width, 0));       // Top-Right
    UVs.Add(FVector2D(Width, Height));  // Bottom-Right

    // 7. reversed Clockwise Triangles!
    Triangles.Add(VertexCount + 0);
    Triangles.Add(VertexCount + 2);
    Triangles.Add(VertexCount + 1);

    Triangles.Add(VertexCount + 0);
    Triangles.Add(VertexCount + 3);
    Triangles.Add(VertexCount + 2);

    VertexCount += 4;
}
