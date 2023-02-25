// Copyright Epic Games, Inc. All Rights Reserved.

#include "TakeoffSendarioGameMode.h"
#include "TakeoffSendarioCharacter.h"
#include "UObject/ConstructorHelpers.h"

ATakeoffSendarioGameMode::ATakeoffSendarioGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
