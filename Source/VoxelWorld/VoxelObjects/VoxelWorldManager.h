// Copyright Ajinkya Borade 2026 (c).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoxelWorldManager.generated.h"

class AVoxelChunk;

UCLASS()
class VOXELWORLD_API AVoxelWorldManager : public AActor
{
	GENERATED_BODY()

public:
	AVoxelWorldManager();
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category = "Voxel")
	TSubclassOf<AVoxelChunk> ChunkClass;

	UPROPERTY(EditAnywhere, Category = "Voxel|Settings")
	int32 RenderDistance = 4; // Radius of chunks to spawn

	UPROPERTY(EditAnywhere, Category = "Voxel|Settings")
	int32 ChunkSizeBlocks = 16;

	UPROPERTY(EditAnywhere, Category = "Voxel|Settings")
	int32 WorldSeed = 42;

protected:
	virtual void BeginPlay() override;

private:
	// Equivalent to: const activeChunks = new Map<string, AVoxelChunk*>();
	UPROPERTY()
	TMap<FIntPoint, AVoxelChunk*> ActiveChunks;

	FIntPoint LastPlayerGridPos;

	FIntPoint WorldToGrid(FVector Position) const;
	void UpdateStreaming();
};
