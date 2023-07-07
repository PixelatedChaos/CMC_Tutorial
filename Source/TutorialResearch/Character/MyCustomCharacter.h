// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../TutorialResearchCharacter.h"
#include "MyCustomCharacter.generated.h"

class UTutCharacterMovementComponent;

/**
 * 
 */
UCLASS()
class TUTORIALRESEARCH_API AMyCustomCharacter : public ATutorialResearchCharacter
{
	GENERATED_BODY()
	
public:

	AMyCustomCharacter(const FObjectInitializer& ObjectInitializer);

	/**
	 * Returns CustomCharacterMovement subobject.
	 */
	UFUNCTION(BlueprintCallable)
	UTutCharacterMovementComponent* GetCustomCharacterMovement() const;

	/**
	 * Helper for gathering ignored actors list. Can be extended.
	 */
	FCollisionQueryParams GetIgnoreCharacterParams() const;
};
