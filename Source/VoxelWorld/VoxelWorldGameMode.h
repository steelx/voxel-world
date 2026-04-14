// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "VoxelWorldGameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class AVoxelWorldGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	AVoxelWorldGameMode();
};



