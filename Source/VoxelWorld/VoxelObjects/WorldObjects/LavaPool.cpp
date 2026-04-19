// Copyright Ajinkya Borade 2026 (c).


#include "LavaPool.h"

FVoxelStructure ULavaPool::GenerateStructure() const
{
	FVoxelStructure Pool;
	Pool.Init(7, 7, 3);

	for (int x = 0; x < 7; x++) {
		for (int y = 0; y < 7; y++) {
			// Cut off the absolute corners to make it circular
			if ((x==0||x==6) && (y==0||y==6)) continue;

			Pool.SetBlock(x, y, 0, EVoxelType::Stone); // The containing basin
			Pool.SetBlock(x, y, 1, EVoxelType::Lava);  // The liquid
			Pool.SetBlock(x, y, 2, EVoxelType::Empty); // Air above the lava to ensure space
		}
	}
	return Pool;
}
