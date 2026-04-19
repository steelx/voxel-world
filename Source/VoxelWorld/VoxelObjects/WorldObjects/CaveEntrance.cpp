// Copyright Ajinkya Borade 2026 (c).


#include "CaveEntrance.h"

FVoxelStructure UCaveEntrance::GenerateStructure() const
{
	FVoxelStructure Entrance;
	Entrance.Init(5, 15, 15);

	// Carve a downward slanting tunnel along the Y axis
	for (int y = 0; y < 15; y++) {
		// As Y increases (moves forward), the center Z drops
		const int zCenter = 13 - y;

		for (int x = 1; x < 4; x++) { // 3 blocks wide
			for (int z = zCenter - 1; z <= zCenter + 3; z++) { // 4 blocks tall
				Entrance.SetBlock(x, y, z, EVoxelType::Empty); // The Drill
			}
		}
	}
	return Entrance;
}
