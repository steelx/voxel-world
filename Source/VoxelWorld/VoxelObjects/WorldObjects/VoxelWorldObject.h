// Copyright Ajinkya Borade 2026 (c).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "VoxelObjects/VoxelBlockData.h"
#include "VoxelWorldObject.generated.h"

/**
 * 
 */
UCLASS()
class VOXELWORLD_API UVoxelWorldObject : public UDataAsset
{
	GENERATED_BODY()
public:
	// Every structure must implement this to return its voxel layout
	virtual FVoxelStructure GenerateStructure() const PURE_VIRTUAL(UVoxelWorldObject::GenerateStructure, return FVoxelStructure(););
};
