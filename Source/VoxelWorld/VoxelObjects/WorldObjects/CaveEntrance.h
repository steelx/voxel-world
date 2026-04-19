// Copyright Ajinkya Borade 2026 (c).

#pragma once

#include "CoreMinimal.h"
#include "VoxelWorldObject.h"
#include "CaveEntrance.generated.h"

/**
 * 
 */
UCLASS()
class VOXELWORLD_API UCaveEntrance : public UVoxelWorldObject
{
	GENERATED_BODY()
public:
	virtual FVoxelStructure GenerateStructure() const override;
};
