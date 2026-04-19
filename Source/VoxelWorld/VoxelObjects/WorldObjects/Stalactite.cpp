// Copyright Ajinkya Borade 2026 (c).


#include "Stalactite.h"

FVoxelStructure UStalactite::GenerateStructure() const
{
	FVoxelStructure Spike;
	Spike.Init(3, 3, 4);

	// Center pillar (longest)
	for(int z = 0; z < 4; z++) Spike.SetBlock(1, 1, z, EVoxelType::Stone);

	// Cross shape base (shorter)
	Spike.SetBlock(1, 0, 3, EVoxelType::Stone);
	Spike.SetBlock(1, 2, 3, EVoxelType::Stone);
	Spike.SetBlock(0, 1, 3, EVoxelType::Stone);
	Spike.SetBlock(2, 1, 3, EVoxelType::Stone);

	return Spike;
}
