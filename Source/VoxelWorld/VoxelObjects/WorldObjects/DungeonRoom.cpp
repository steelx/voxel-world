// Copyright Ajinkya Borade 2026 (c).


#include "DungeonRoom.h"

FVoxelStructure UDungeonRoom::GenerateStructure() const
{
	FVoxelStructure Dungeon;
	Dungeon.Init(9, 9, 7);

	for (int x = 0; x < 9; x++) {
		for (int y = 0; y < 9; y++) {
			for (int z = 0; z < 7; z++) {
				// If it's the outer shell, place Cobblestone. Otherwise, carve Air.
				if (x == 0 || x == 8 || y == 0 || y == 8 || z == 0 || z == 6) {
					Dungeon.SetBlock(x, y, z, EVoxelType::Cobblestone);
				} else {
					Dungeon.SetBlock(x, y, z, EVoxelType::Empty); // Hollow inside
				}
			}
		}
	}
	return Dungeon;
}
