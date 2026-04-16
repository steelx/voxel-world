// Copyright Ajinkya Borade 2026 (c).

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "VoxelBlockData.generated.h"

// The Voxel ID
// We use uint8 to guarantee each voxel only takes up 1 byte in our arrays.
UENUM(BlueprintType)
enum class EVoxelType : uint8
{
	Ignore UMETA(Hidden),
	Empty UMETA(DisplayName = "Empty (Air)"),
	Dirt  UMETA(DisplayName = "Dirt"),
	Grass UMETA(DisplayName = "Grass"),
	Stone UMETA(DisplayName = "Stone"),
	Cobblestone UMETA(DisplayName = "Cobblestone"),
	Water UMETA(DisplayName = "Water"),
	Lava  UMETA(DisplayName = "Lava"),
	Coal  UMETA(DisplayName = "Coal"),
	Diamond UMETA(DisplayName = "Diamond"),
	Wood  UMETA(DisplayName = "Wood"),
	Leaves UMETA(DisplayName = "Leaves")
};

USTRUCT(BlueprintType)
struct FVoxelStructure
{
	GENERATED_BODY()

	int32 SizeX = 0;
	int32 SizeY = 0;
	int32 SizeZ = 0;
	TArray<EVoxelType> Blocks;

	// Initializes the 3D grid size for the prefab
	void Init(const int32 InX, const int32 InY, const int32 InZ) {
		SizeX = InX; SizeY = InY; SizeZ = InZ;
		Blocks.Init(EVoxelType::Empty, SizeX * SizeY * SizeZ);
	}

	// Safely sets a block inside the local prefab space
	void SetBlock(const int32 X, const int32 Y, const int32 Z, const EVoxelType BlockType) {
		if (X >= 0 && X < SizeX && Y >= 0 && Y < SizeY && Z >= 0 && Z < SizeZ) {
			Blocks[X + (Y * SizeX) + (Z * SizeX * SizeY)] = BlockType;
		}
	}

	// Retrieves a block from the prefab space
	EVoxelType GetBlock(const int32 X, const int32 Y, const int32 Z) const {
		if (X >= 0 && X < SizeX && Y >= 0 && Y < SizeY && Z >= 0 && Z < SizeZ) {
			return Blocks[X + (Y * SizeX) + (Z * SizeX * SizeY)];
		}
		return EVoxelType::Ignore;// Return Ignore if out of bounds
	}
};

// The Voxel Properties
// Inheriting from FTableRowBase allows Unreal to use this in a DataTable.
USTRUCT(BlueprintType)
struct FVoxelBlockData : public FTableRowBase
{
	GENERATED_BODY()

	// Does this block stop the player from walking through it?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Properties")
	bool bIsSolid = true;

	// Does this block let light/sight through? (Crucial for greedy meshing logic later)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Properties")
	bool bIsTransparent = false;

	// The sound played when the block is destroyed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Audio")
	USoundBase* BreakSound = nullptr;

	// The sound played when the player walks on it
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Audio")
	USoundBase* FootstepSound = nullptr;

	// The color that will be passed to the Vertex Color buffer
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Visuals")
	FLinearColor BlockColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Visuals")
	FVector2D AtlasCoordinate = FVector2D(0, 0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel|Visuals")
	float TextureBlockSize {16};
};
