// Copyright Ajinkya Borade 2026 (c).


#include "Lake.h"

FVoxelStructure ULake::GenerateStructure() const
{
	FVoxelStructure Lake;
	Lake.Init(15, 15, 7);

	for (int x = 0; x < 15; x++) {
		for (int y = 0; y < 15; y++) {

			// Calculate flat 2D distance from the center of the lake
			const float Dist2D = FMath::Sqrt(FMath::Square(x - 7.0f) + FMath::Square(y - 7.0f));

			// If we are inside the 15x15 circle
			if (Dist2D <= 6.5f)
			{
				// BasinZ dictates the shape of the bowl.
				// Center (Dist=0) -> BasinZ = 0 (Deepest part of the lake)
				// Edge (Dist=6.5) -> BasinZ = 3 (Shallow shores)
				const int32 BasinZ = FMath::RoundToInt((Dist2D / 6.5f) * 3.0f);

				// Build the column from bottom to top
				for (int z = 0; z < 7; z++) {
					if (z > 4) {
						// Carve away the terrain above the water so it isn't buried
						Lake.SetBlock(x, y, z, EVoxelType::Empty);
					} else if (z > BasinZ) {
						// Fill the bowl with water
						Lake.SetBlock(x, y, z, EVoxelType::Water);
					} else if (z >= BasinZ - 1) {
						// Lay down a solid dirt crust to hold the water (guaranteed floor!)
						Lake.SetBlock(x, y, z, EVoxelType::Dirt);
					}
				}
			}
		}
	}
	return Lake;
}
