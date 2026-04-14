## File Tree @ 2026-04-14 12:53:53

```
├── VoxelObjects
│   ├── VoxelBlockData.cpp
│   ├── VoxelBlockData.h
│   ├── VoxelChunk.cpp
│   └── VoxelChunk.h
├── VoxelWorld.cpp
├── VoxelWorld.h
├── VoxelWorldCharacter.cpp
├── VoxelWorldCharacter.h
├── VoxelWorldGameMode.cpp
├── VoxelWorldGameMode.h
├── VoxelWorldPlayerController.cpp
└── VoxelWorldPlayerController.h
```

## File Analysis @ 2026-04-14 12:53:53

- Total files: 12
- .cpp: 6
- .h: 6


---
#### VoxelObjects/VoxelBlockData.cpp @ 2026-04-14 12:53:53

```cpp
// Copyright Ajinkya Borade 2026 (c).


#include "VoxelBlockData.h"

```

---
#### VoxelObjects/VoxelBlockData.h @ 2026-04-14 12:53:53

```c
// Copyright Ajinkya Borade 2026 (c).

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "VoxelBlockData.generated.h"

// The Voxel ID
// We use uint8 to guarantee each voxel only takes up 1 byte in our arrays.
UENUM(BlueprintType)
enum class EVoxelType : uint8
{
	Empty UMETA(DisplayName = "Empty (Air)"),
	Dirt  UMETA(DisplayName = "Dirt"),
	Grass UMETA(DisplayName = "Grass"),
	Stone UMETA(DisplayName = "Stone"),
	Water UMETA(DisplayName = "Water"),
	Lava  UMETA(DisplayName = "Lava")
};

// The Voxel Properties
// Inheriting from FTableRowBase allows Unreal to use this in a DataTable.
USTRUCT(BlueprintType)
struct FVoxelBlockData : public FTableRowBase
{
	GENERATED_BODY()

	// Does this block stop the player from walking through it?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Properties")
	bool bIsSolid = true;

	// Does this block let light/sight through? (Crucial for greedy meshing logic later)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Properties")
	bool bIsTransparent = false;

	// The sound played when the block is destroyed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Audio")
	USoundBase* BreakSound = nullptr;

	// The sound played when the player walks on it
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Audio")
	USoundBase* FootstepSound = nullptr;

	// Future expansion: We will add Texture/UV Atlas coordinates here
};

```

---
#### VoxelObjects/VoxelChunk.cpp @ 2026-04-14 12:53:53

```cpp
// Copyright Ajinkya Borade 2026 (c).


#include "VoxelChunk.h"

#include "ProceduralMeshComponent.h"


// Sets default values
AVoxelChunk::AVoxelChunk()
{
	MeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMeshComp"));
    SetRootComponent(MeshComponent);
}

// Called when the game starts or when spawned
void AVoxelChunk::BeginPlay()
{
	Super::BeginPlay();
    GenerateVoxelData();
    GenerateMesh();
}

// 1D Flattening Math
int32 AVoxelChunk::GetVoxelIndex(const int32 X, const int32 Y, const int32 Z) const
{
    // The golden formula for 1D Array (https://eloquentjavascript.net/2nd_edition/07_elife.html)
    return X + (Y * ChunkSize) + (Z * ChunkSize * ChunkSize);
}

// Safe wrapper to prevent out-of-bounds array crashes
EVoxelType AVoxelChunk::GetVoxelType(const int32 X, const int32 Y, const int32 Z) const
{
    if (X < 0 || X >= ChunkSize || Y < 0 || Y >= ChunkSize || Z < 0 || Z >= ChunkSize)
        return EVoxelType::Empty; // Treat outside the chunk as air for now

    return VoxelData[GetVoxelIndex(X, Y, Z)];
}

// Step 1: Fill the memory with data
void AVoxelChunk::GenerateVoxelData()
{
    // Reserve memory upfront to prevent reallocation overhead
    VoxelData.Init(EVoxelType::Empty, ChunkSize * ChunkSize * ChunkSize);

    // Simple generation: Fill the bottom half with Dirt
    for (int32 Z = 0; Z < ChunkSize; Z++)
    {
        for (int32 Y = 0; Y < ChunkSize; Y++)
        {
            for (int32 X = 0; X < ChunkSize; X++)
            {
                if (Z < ChunkSize / 2)
                {
                    VoxelData[GetVoxelIndex(X, Y, Z)] = EVoxelType::Dirt;
                }
            }
        }
    }
}

// Step 2: The Naive Surface Mesher
void AVoxelChunk::GenerateMesh()
{
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    int32 VertexCount = 0;

    // Iterate through every single voxel
    for (int32 Z = 0; Z < ChunkSize; Z++)
    {
        for (int32 Y = 0; Y < ChunkSize; Y++)
        {
            for (int32 X = 0; X < ChunkSize; X++)
            {
                if (GetVoxelType(X, Y, Z) == EVoxelType::Empty) continue;

                // Scale factor: 1 Unreal Unit = 1cm. A 1m block is 100 units.
                FVector BlockPos(X * 100, Y * 100, Z * 100);

                // Check Top Face Neighbor (Z + 1)
                if (GetVoxelType(X, Y, Z + 1) == EVoxelType::Empty)
                {
                    // Generate the 4 corners of the top face
                    Vertices.Add(BlockPos + FVector(0, 0, 100));     // 0: Bottom-Left
                    Vertices.Add(BlockPos + FVector(100, 0, 100));   // 1: Bottom-Right
                    Vertices.Add(BlockPos + FVector(0, 100, 100));   // 2: Top-Left
                    Vertices.Add(BlockPos + FVector(100, 100, 100)); // 3: Top-Right

                    // Generate the 2 triangles using proper Clockwise winding order
                    Triangles.Add(VertexCount + 0);
                    Triangles.Add(VertexCount + 2);
                    Triangles.Add(VertexCount + 1);

                    Triangles.Add(VertexCount + 1);
                    Triangles.Add(VertexCount + 2);
                    Triangles.Add(VertexCount + 3);

                    VertexCount += 4;
                }

                // Repeat identical logic for Bottom, North, South, East, West faces
                // Check Bottom Face Neighbor (Z - 1)
                if (GetVoxelType(X, Y, Z - 1) == EVoxelType::Empty)
                {
                    // Generate the 4 corners of the bottom face
                    Vertices.Add(BlockPos + FVector(0, 0, 0));     // 0: Bottom-Left
                    Vertices.Add(BlockPos + FVector(0, 100, 0));   // 1: Top-Left
                    Vertices.Add(BlockPos + FVector(100, 0, 0));   // 2: Bottom-Right
                    Vertices.Add(BlockPos + FVector(100, 100, 0)); // 3: Top-Right

                    // Generate the 2 triangles using proper Clockwise winding order
                    Triangles.Add(VertexCount + 0);
                    Triangles.Add(VertexCount + 1);
                    Triangles.Add(VertexCount + 2);

                    Triangles.Add(VertexCount + 2);
                    Triangles.Add(VertexCount + 1);
                    Triangles.Add(VertexCount + 3);

                    VertexCount += 4;
                }

                // Check North Face Neighbor (Y + 1)
                if (GetVoxelType(X, Y + 1, Z) == EVoxelType::Empty)
                {
                    // Generate the 4 corners of the north face
                    Vertices.Add(BlockPos + FVector(0, 100, 0));     // 0: Bottom-Left
                    Vertices.Add(BlockPos + FVector(0, 100, 100));   // 1: Top-Left
                    Vertices.Add(BlockPos + FVector(100, 100, 0));   // 2: Bottom-Right
                    Vertices.Add(BlockPos + FVector(100, 100, 100)); // 3: Top-Right

                    // Generate the 2 triangles using proper Clockwise winding order
                    Triangles.Add(VertexCount + 0);
                    Triangles.Add(VertexCount + 1);
                    Triangles.Add(VertexCount + 2);

                    Triangles.Add(VertexCount + 2);
                    Triangles.Add(VertexCount + 1);
                    Triangles.Add(VertexCount + 3);

                    VertexCount += 4;
                }

                // Check South Face Neighbor (Y - 1)
                if (GetVoxelType(X, Y - 1, Z) == EVoxelType::Empty)
                {
                    // Generate the 4 corners of the south face
                    Vertices.Add(BlockPos + FVector(0, 0, 0));     // 0: Bottom-Left
                    Vertices.Add(BlockPos + FVector(100, 0, 0));   // 1: Bottom-Right
                    Vertices.Add(BlockPos + FVector(0, 0, 100));   // 2: Top-Left
                    Vertices.Add(BlockPos + FVector(100, 0, 100)); // 3: Top-Right

                    // Generate the 2 triangles using proper Clockwise winding order
                    Triangles.Add(VertexCount + 0);
                    Triangles.Add(VertexCount + 2);
                    Triangles.Add(VertexCount + 1);

                    Triangles.Add(VertexCount + 1);
                    Triangles.Add(VertexCount + 2);
                    Triangles.Add(VertexCount + 3);

                    VertexCount += 4;
                }

                // Check East Face Neighbor (X + 1)
                if (GetVoxelType(X + 1, Y, Z) == EVoxelType::Empty)
                {
                    // Generate the 4 corners of the east face
                    Vertices.Add(BlockPos + FVector(100, 0, 0));     // 0: Bottom-Left
                    Vertices.Add(BlockPos + FVector(100, 0, 100));   // 1: Top-Left
                    Vertices.Add(BlockPos + FVector(100, 100, 0));   // 2: Bottom-Right
                    Vertices.Add(BlockPos + FVector(100, 100, 100)); // 3: Top-Right

                    // Generate the 2 triangles using proper Clockwise winding order
                    Triangles.Add(VertexCount + 0);
                    Triangles.Add(VertexCount + 1);
                    Triangles.Add(VertexCount + 2);

                    Triangles.Add(VertexCount + 2);
                    Triangles.Add(VertexCount + 1);
                    Triangles.Add(VertexCount + 3);

                    VertexCount += 4;
                }

                // Check West Face Neighbor (X - 1)
                if (GetVoxelType(X - 1, Y, Z) == EVoxelType::Empty)
                {
                    // Generate the 4 corners of the west face
                    Vertices.Add(BlockPos + FVector(0, 0, 0));     // 0: Bottom-Left
                    Vertices.Add(BlockPos + FVector(0, 100, 0));   // 1: Bottom-Right
                    Vertices.Add(BlockPos + FVector(0, 0, 100));   // 2: Top-Left
                    Vertices.Add(BlockPos + FVector(0, 100, 100)); // 3: Top-Right

                    // Generate the 2 triangles using proper Clockwise winding order
                    Triangles.Add(VertexCount + 0);
                    Triangles.Add(VertexCount + 2);
                    Triangles.Add(VertexCount + 1);

                    Triangles.Add(VertexCount + 1);
                    Triangles.Add(VertexCount + 2);
                    Triangles.Add(VertexCount + 3);

                    VertexCount += 4;
                }
            }
        }
    }

    // Ship the arrays directly to the GPU!
    MeshComponent->CreateMeshSection_LinearColor(0, Vertices, Triangles, TArray<FVector>(), TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), true);
}

void AVoxelChunk::AddFace(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs, int32& VertexCount, FVector BlockPos, int32 FaceIndex)
{
    constexpr float Size = 100.0f; // One Unreal Unit = 1cm. 100 = 1 Meter Block.

    // The 8 corners of our local voxel cube
    // We define every face's vertices in the exact same logical order:
    // v0: Bottom-Left, v1: Bottom-Right, v2: Top-Left, v3: Top-Right
    FVector V0, V1, V2, V3, Normal;

    switch (FaceIndex)
    {
    default:
    case 0: // Top (Z+) - Looking down at it
        V0 = FVector(0, 0, Size); V1 = FVector(0, Size, Size); V2 = FVector(Size, 0, Size); V3 = FVector(Size, Size, Size);
        Normal = FVector(0, 0, 1);
        break;
    case 1: // Bottom (Z-) - Looking up at it
        V0 = FVector(Size, 0, 0); V1 = FVector(Size, Size, 0); V2 = FVector(0, 0, 0); V3 = FVector(0, Size, 0);
        Normal = FVector(0, 0, -1);
        break;
    case 2: // Front (X+) - Looking back at -X
        V0 = FVector(Size, Size, 0); V1 = FVector(Size, 0, 0); V2 = FVector(Size, Size, Size); V3 = FVector(Size, 0, Size);
        Normal = FVector(1, 0, 0);
        break;
    case 3: // Back (X-) - Looking forward at +X
        V0 = FVector(0, 0, 0); V1 = FVector(0, Size, 0); V2 = FVector(0, 0, Size); V3 = FVector(0, Size, Size);
        Normal = FVector(-1, 0, 0);
        break;
    case 4: // Right (Y+) - Looking left at -Y
        V0 = FVector(0, Size, 0); V1 = FVector(Size, Size, 0); V2 = FVector(0, Size, Size); V3 = FVector(Size, Size, Size);
        Normal = FVector(0, 1, 0);
        break;
    case 5: // Left (Y-) - Looking right at +Y
        V0 = FVector(Size, 0, 0); V1 = FVector(0, 0, 0); V2 = FVector(Size, 0, Size); V3 = FVector(0, 0, Size);
        Normal = FVector(0, -1, 0);
        break;
    }

    // Append Vertices
    Vertices.Add(BlockPos + V0);
    Vertices.Add(BlockPos + V1);
    Vertices.Add(BlockPos + V2);
    Vertices.Add(BlockPos + V3);

    // Append Normals (Crucial for lighting to interact with the block)
    for(int i = 0; i < 4; i++) Normals.Add(Normal);

    // Append UVs (Maps a standard texture to the square face)
    UVs.Add(FVector2D(0, 0));// Bottom Left
    UVs.Add(FVector2D(1, 0));// Bottom Right
    UVs.Add(FVector2D(0, 1));// Top Left
    UVs.Add(FVector2D(1, 1));// Top Right

    // Append Triangles (Creating 2 triangles from 4 vertices using Clockwise winding)
    Triangles.Add(VertexCount + 0);
    Triangles.Add(VertexCount + 2);
    Triangles.Add(VertexCount + 1);

    Triangles.Add(VertexCount + 1);
    Triangles.Add(VertexCount + 2);
    Triangles.Add(VertexCount + 3);

    VertexCount += 4;

#if WITH_EDITOR || UE_BUILD_DEVELOPMENT
    if (bShowDebugFaces)
    {
        // Reconstruct the Grid Coordinate to isolate our single target block
        int32 GridX = FMath::RoundToInt(BlockPos.X / 100.0f);
        int32 GridY = FMath::RoundToInt(BlockPos.Y / 100.0f);
        int32 GridZ = FMath::RoundToInt(BlockPos.Z / 100.0f);

        if (GridX == DebugVoxelCoordinate.X && GridY == DebugVoxelCoordinate.Y && GridZ == DebugVoxelCoordinate.Z)
        {
            // Calculate the absolute center point of this specific face
            FVector FaceCenter = BlockPos + (V0 + V1 + V2 + V3) / 4.0f;

            // Draw a yellow arrow pointing in the direction of the Normal
            DrawDebugDirectionalArrow(GetWorld(), FaceCenter, FaceCenter + (Normal * 60.0f), 10.0f, FColor::Yellow, true, -1.0f, 0, 2.0f);

            FString FaceName;
            switch(FaceIndex)
            {
            case 0: FaceName = TEXT("Top (Z+)"); break;
            case 1: FaceName = TEXT("Bottom (Z-)"); break;
            case 2: FaceName = TEXT("Front (X+)"); break;
            case 3: FaceName = TEXT("Back (X-)"); break;
            case 4: FaceName = TEXT("Right (Y+)"); break;
            case 5: FaceName = TEXT("Left (Y-)"); break;
            }

            // Draw the face name hovering slightly past the arrow
            DrawDebugString(GetWorld(), FaceCenter + (Normal * 80.0f), FaceName, nullptr, FColor::White, 1000.0f, false, 1.2f);
        }
    }
#endif
}

```

---
#### VoxelObjects/VoxelChunk.h @ 2026-04-14 12:53:53

```c
// Copyright Ajinkya Borade 2026 (c).

#pragma once

#include "CoreMinimal.h"
#include "VoxelBlockData.h"
#include "GameFramework/Actor.h"
#include "VoxelChunk.generated.h"

class UProceduralMeshComponent;

UCLASS()
class AVoxelChunk : public AActor
{
	GENERATED_BODY()

public:
	AVoxelChunk();

	// A static helper array representing the 6 directional neighbors (Top, Bottom, Front, Back, Right, Left)
	const FIntVector FaceOffsets[6] = {
		FIntVector(0, 0, 1),  // Top (Z+)
		FIntVector(0, 0, -1), // Bottom (Z-)
		FIntVector(1, 0, 0),  // Front (X+)
		FIntVector(-1, 0, 0), // Back (X-)
		FIntVector(0, 1, 0),  // Right (Y+)
		FIntVector(0, -1, 0)  // Left (Y-)
	};

protected:
	virtual void BeginPlay() override;

	// The View: The component that talks directly to the GPU
	UPROPERTY(VisibleAnywhere, Category="Voxel")
	TObjectPtr<UProceduralMeshComponent> MeshComponent;

	// Toggle this in the editor to see the normals and face names
	UPROPERTY(EditAnywhere, Category = "Voxel|Debug")
	bool bShowDebugFaces = false;

	// Pick a specific voxel coordinate to debug (e.g., 0, 15, 15 for the Back face)
	UPROPERTY(EditAnywhere, Category = "Voxel|Debug")
	FIntVector DebugVoxelCoordinate = FIntVector(0, 0, 15);

	// The dimensions of our chunk. 32x32x32 = 32,768 blocks.
	const int32 ChunkSize = 32;

	// The Model: Our 1D array of 8-bit Enums.
	TArray<EVoxelType> VoxelData;


	// Helper to convert 3D coordinates (X,Y,Z) into our 1D array index
	int32 GetVoxelIndex(const int32 X, const int32 Y, const int32 Z) const;
	EVoxelType GetVoxelType(const int32 X, const int32 Y, const int32 Z) const;

	// The lifecycle functions
	void GenerateVoxelData();
	void GenerateMesh();
	void AddFace(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs, int32& VertexCount, FVector BlockPos, int32 FaceIndex);
};
```

---
#### VoxelWorld.cpp @ 2026-04-14 12:53:53

```cpp
// Copyright Epic Games, Inc. All Rights Reserved.

#include "VoxelWorld.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, VoxelWorld, "VoxelWorld" );

DEFINE_LOG_CATEGORY(LogVoxelWorld)
```

---
#### VoxelWorld.h @ 2026-04-14 12:53:53

```c
// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** Main log category used across the project */
DECLARE_LOG_CATEGORY_EXTERN(LogVoxelWorld, Log, All);
```

---
#### VoxelWorldCharacter.cpp @ 2026-04-14 12:53:53

```cpp
// Copyright Epic Games, Inc. All Rights Reserved.

#include "VoxelWorldCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "VoxelWorld.h"

AVoxelWorldCharacter::AVoxelWorldCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void AVoxelWorldCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AVoxelWorldCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AVoxelWorldCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AVoxelWorldCharacter::Look);
	}
	else
	{
		UE_LOG(LogVoxelWorld, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AVoxelWorldCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AVoxelWorldCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AVoxelWorldCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AVoxelWorldCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AVoxelWorldCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void AVoxelWorldCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

```

---
#### VoxelWorldCharacter.h @ 2026-04-14 12:53:53

```c
// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "VoxelWorldCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AVoxelWorldCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

public:

	/** Constructor */
	AVoxelWorldCharacter();	

protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};


```

---
#### VoxelWorldGameMode.cpp @ 2026-04-14 12:53:53

```cpp
// Copyright Epic Games, Inc. All Rights Reserved.

#include "VoxelWorldGameMode.h"

AVoxelWorldGameMode::AVoxelWorldGameMode()
{
	// stub
}

```

---
#### VoxelWorldGameMode.h @ 2026-04-14 12:53:53

```c
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




```

---
#### VoxelWorldPlayerController.cpp @ 2026-04-14 12:53:53

```cpp
// Copyright Epic Games, Inc. All Rights Reserved.


#include "VoxelWorldPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "VoxelWorld.h"
#include "Widgets/Input/SVirtualJoystick.h"

void AVoxelWorldPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (ShouldUseTouchControls() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogVoxelWorld, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AVoxelWorldPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

bool AVoxelWorldPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

```

---
#### VoxelWorldPlayerController.h @ 2026-04-14 12:53:53

```c
// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "VoxelWorldPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class AVoxelWorldPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

};

```
