// Copyright Epic Games, Inc. All Rights Reserved.

#include "TutorialResearchGameMode.h"
#include "TutorialResearchCharacter.h"
#include "UObject/ConstructorHelpers.h"

ATutorialResearchGameMode::ATutorialResearchGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
