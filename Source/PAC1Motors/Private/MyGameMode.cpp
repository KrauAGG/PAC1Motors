// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameMode.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

AMyGameMode::AMyGameMode()
{
    static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClass(
        TEXT("/Game/Blueprints/BP_PlayerCharacter"));

    if (PlayerPawnClass.Class != nullptr)
    {
        DefaultPawnClass = PlayerPawnClass.Class;
    }
}