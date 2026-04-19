// Copyright Ajinkya Borade 2026 (c).

#pragma once

#include "CoreMinimal.h"
#include "VoxelWorldObject.h"
#include "LavaPool.generated.h"

/**
 * 
 */
UCLASS()
class VOXELWORLD_API ULavaPool : public UVoxelWorldObject
{
	GENERATED_BODY()

public:
	virtual FVoxelStructure GenerateStructure() const override;
};
