// Copyright Ajinkya Borade 2026 (c).


#include "VoxelWorldManager.h"

#include "VoxelChunk.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
AVoxelWorldManager::AVoxelWorldManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AVoxelWorldManager::BeginPlay()
{
	Super::BeginPlay();
	UpdateStreaming();
}

void AVoxelWorldManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!Player) return;

	FIntPoint CurrentGridPos = WorldToGrid(Player->GetActorLocation());

	// Throttling: Only update if the player has actually crossed a boundary
	if (CurrentGridPos != LastPlayerGridPos)
	{
		LastPlayerGridPos = CurrentGridPos;
		UpdateStreaming();
	}
}

FIntPoint AVoxelWorldManager::WorldToGrid(FVector Position) const
{
	const float ChunkWorldSize = ChunkSizeBlocks * 100.0f;
	return FIntPoint(
		FMath::FloorToInt(Position.X / ChunkWorldSize),
		FMath::FloorToInt(Position.Y / ChunkWorldSize)
	);
}

void AVoxelWorldManager::UpdateStreaming()
{
	const float ChunkWorldSize = ChunkSizeBlocks * 100.0f;

	for (int32 x = -RenderDistance; x <= RenderDistance; x++)
	{
		for (int32 y = -RenderDistance; y <= RenderDistance; y++)
		{
			FIntPoint TargetCoord = LastPlayerGridPos + FIntPoint(x, y);

			if (!ActiveChunks.Contains(TargetCoord))
			{
				FVector SpawnLocation(TargetCoord.X * ChunkWorldSize, TargetCoord.Y * ChunkWorldSize, 0.0f);

				FActorSpawnParameters Params;
				AVoxelChunk* NewChunk = GetWorld()->SpawnActor<AVoxelChunk>(ChunkClass, SpawnLocation, FRotator::ZeroRotator, Params);

				if (NewChunk)
				{
					// Map key: TargetCoord, Value: Pointer to the Actor
					ActiveChunks.Add(TargetCoord, NewChunk);
					NewChunk->InitializeChunk(WorldSeed, TargetCoord, 32);
				}
			}
		}
	}
}
