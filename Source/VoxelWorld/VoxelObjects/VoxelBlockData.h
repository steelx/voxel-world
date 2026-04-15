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
	Empty UMETA(DisplayName = "Empty (Air)"),
	Dirt  UMETA(DisplayName = "Dirt"),
	Grass UMETA(DisplayName = "Grass"),
	Stone UMETA(DisplayName = "Stone"),
	Water UMETA(DisplayName = "Water"),
	Lava  UMETA(DisplayName = "Lava")
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
