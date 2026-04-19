// Copyright Ajinkya Borade 2026 (c).

#pragma once

#include "CoreMinimal.h"
#include "VoxelWorldObject.h"
#include "Stalactite.generated.h"

/**
 * icicle-shaped mineral deposits that hang from cave ceilings
 */
UCLASS()
class VOXELWORLD_API UStalactite : public UVoxelWorldObject
{
	GENERATED_BODY()

public:
	virtual FVoxelStructure GenerateStructure() const override;
};
