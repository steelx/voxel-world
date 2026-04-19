// Copyright Ajinkya Borade 2026 (c).


#include "VoxelTree.h"

#include "VoxelObjects/VoxelBlockData.h"

FVoxelStructure UVoxelTree::GenerateStructure() const
{
    FVoxelStructure Tree;
    Tree.Init(5, 5, 7); // A 5x5 wide box, 7 blocks tall

    // 1. Build the Trunk (Center is X:2, Y:2)
    for (int32 Z = 0; Z < 5; Z++) {
        Tree.SetBlock(2, 2, Z, EVoxelType::Wood);
    }

    // 2. Build the Leaves (Spherical canopy)
    for (int32 X = 0; X < 5; X++) {
        for (int32 Y = 0; Y < 5; Y++) {
            for (int32 Z = 3; Z < 7; Z++) {
                if (X == 2 && Y == 2 && Z < 5) continue;
                const float DistSq = FMath::Square(X - 2.0f) + FMath::Square(Y - 2.0f) + FMath::Square(Z - 4.5f);
                if (DistSq <= 5.5f) {
                    Tree.SetBlock(X, Y, Z, EVoxelType::Leaves);
                }
            }
        }
    }
    return Tree;
}
