// Copyright Ajinkya Borade 2026 (c).

#pragma once

#include "CoreMinimal.h"
#include "VoxelWorldObject.h"
#include "DungeonRoom.generated.h"

/**
 * 
 */
UCLASS()
class VOXELWORLD_API UDungeonRoom : public UVoxelWorldObject
{
	GENERATED_BODY()

public:
	virtual FVoxelStructure GenerateStructure() const override;
};
