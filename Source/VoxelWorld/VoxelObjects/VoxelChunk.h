// Copyright Ajinkya Borade 2026 (c).

#pragma once

#include "CoreMinimal.h"
#include "VoxelBlockData.h"
#include "GameFramework/Actor.h"
#include "VoxelChunk.generated.h"

class UProceduralMeshComponent;

UCLASS()
class AVoxelChunk : public AActor
{
	GENERATED_BODY()

public:
	AVoxelChunk();

	// A static helper array representing the 6 directional neighbors (Top, Bottom, Front, Back, Right, Left)
	const FIntVector FaceOffsets[6] = {
		FIntVector(0, 0, 1),  // Top (Z+)
		FIntVector(0, 0, -1), // Bottom (Z-)
		FIntVector(1, 0, 0),  // Front (X+)
		FIntVector(-1, 0, 0), // Back (X-)
		FIntVector(0, 1, 0),  // Right (Y+)
		FIntVector(0, -1, 0)  // Left (Y-)
	};

protected:
	virtual void BeginPlay() override;

	// The View: The component that talks directly to the GPU
	UPROPERTY(VisibleAnywhere, Category="Voxel")
	TObjectPtr<UProceduralMeshComponent> MeshComponent;

	// Toggle this in the editor to see the normals and face names
	UPROPERTY(EditAnywhere, Category = "Voxel|Debug")
	bool bShowDebugFaces = false;

	// Pick a specific voxel coordinate to debug (e.g., 0, 15, 15 for the Back face)
	UPROPERTY(EditAnywhere, Category = "Voxel|Debug")
	FIntVector DebugVoxelCoordinate = FIntVector(0, 0, 15);

	// The dimensions of our chunk. 32x32x32 = 32,768 blocks.
	const int32 ChunkSize = 32;

	// The Model: Our 1D array of 8-bit Enums.
	TArray<EVoxelType> VoxelData;


	// Helper to convert 3D coordinates (X,Y,Z) into our 1D array index
	int32 GetVoxelIndex(const int32 X, const int32 Y, const int32 Z) const;
	EVoxelType GetVoxelType(const int32 X, const int32 Y, const int32 Z) const;

	// The lifecycle functions
	void GenerateVoxelData();
	void GenerateMesh();
	void AddFace(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs, int32& VertexCount, FVector BlockPos, int32 FaceIndex);
};