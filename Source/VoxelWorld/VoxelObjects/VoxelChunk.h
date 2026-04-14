// Copyright Ajinkya Borade 2026 (c).

#pragma once

#include "CoreMinimal.h"
#include "VoxelBlockData.h"
#include "GameFramework/Actor.h"
#include "VoxelChunk.generated.h"

class UProceduralMeshComponent;


/*
* NAIVE MESHING: A 3x2 Wall of Dirt

 V0  V1  V2  V3
  +---+---+---+
  |\  |\  |\  |
  | \ | \ | \ |  Row 1 (3 Quads, 6 Triangles)
  |  \|  \|  \|
  +---+---+---+
  |\  |\  |\  |
  | \ | \ | \ |  Row 0 (3 Quads, 6 Triangles)
  |  \|  \|  \|
  +---+---+---+

 Result: 6 distinct Quads.
 Vertex Count: 24 vertices (overlapping).
 Triangle Count: 12 triangles.
 Draw Calls: 1 (Good!), but High Poly Count (Bad!).
 */
UCLASS()
class AVoxelChunk : public AActor
{
	GENERATED_BODY()

public:
	AVoxelChunk();

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

private:
	// A static helper array representing the 6 directional neighbors (Top, Bottom, Front, Back, Right, Left)
	const FIntVector FaceOffsets[6] = {
		FIntVector(0, 0, 1),  // Top (Z+)
		FIntVector(0, 0, -1), // Bottom (Z-)
		FIntVector(1, 0, 0),  // Front (X+)
		FIntVector(-1, 0, 0), // Back (X-)
		FIntVector(0, 1, 0),  // Right (Y+)
		FIntVector(0, -1, 0)  // Left (Y-)
	};

	// The 8 absolute corners of a 100cm voxel block
	const FVector V[8] = {
		FVector(0, 0, 0),       // 0: Bottom-Back-Left
		FVector(100, 0, 0),     // 1: Bottom-Front-Left
		FVector(0, 100, 0),     // 2: Bottom-Back-Right
		FVector(100, 100, 0),   // 3: Bottom-Front-Right
		FVector(0, 0, 100),     // 4: Top-Back-Left
		FVector(100, 0, 100),   // 5: Top-Front-Left
		FVector(0, 100, 100),   // 6: Top-Back-Right
		FVector(100, 100, 100)  // 7: Top-Front-Right
	};

	// Lookup table: Maps each face to 4 specific vertices in perfect CLOCKWISE order
	// Order: BottomLeft, TopLeft, TopRight, BottomRight (from the camera's perspective)
	const int32 FaceVertices[6][4] = {
		{4, 5, 7, 6}, // 0: Top (Z+)
		{2, 3, 1, 0}, // 1: Bottom (Z-)
		{3, 7, 5, 1}, // 2: Front (X+)
		{0, 4, 6, 2}, // 3: Back (X-)
		{2, 6, 7, 3}, // 4: Right (Y+)
		{1, 5, 4, 0}  // 5: Left (Y-)
	};

	// Outward-facing normals for lighting and physics collision
	const FVector FaceNormals[6] = {
		FVector(0, 0, 1),  // Top
		FVector(0, 0, -1), // Bottom
		FVector(1, 0, 0),  // Front
		FVector(-1, 0, 0), // Back
		FVector(0, 1, 0),  // Right
		FVector(0, -1, 0)  // Left
	};
};