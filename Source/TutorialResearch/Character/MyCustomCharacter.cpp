// Fill out your copyright notice in the Description page of Project Settings.


#include "MyCustomCharacter.h"
#include "TutCharacterMovementComponent.h"


AMyCustomCharacter::AMyCustomCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UTutCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	
}


UTutCharacterMovementComponent* AMyCustomCharacter::GetCustomCharacterMovement() const
{
	return GetCharacterMovement<UTutCharacterMovementComponent>();
}

FCollisionQueryParams AMyCustomCharacter::GetIgnoreCharacterParams() const
{
	FCollisionQueryParams Params;

	TArray<AActor*> CharacterChildren;
	GetAllChildActors(CharacterChildren);
	Params.AddIgnoredActors(CharacterChildren);
	Params.AddIgnoredActor(this);

	return Params;
}