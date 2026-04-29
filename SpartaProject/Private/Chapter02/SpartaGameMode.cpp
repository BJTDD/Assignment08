// Fill out your copyright notice in the Description page of Project Settings.


#include "Chapter02/SpartaGameMode.h"
#include "Chapter02/SpartaCharacter.h"
#include "Chapter02/SpartaPlayerController.h"
#include "Chapter03/SpartaGameState.h"

ASpartaGameMode::ASpartaGameMode()
{
	// SpartaCharacter 클래스 사용
	DefaultPawnClass = ASpartaCharacter::StaticClass();
	// SpartaPlayerController 클래스 사용
	PlayerControllerClass = ASpartaPlayerController::StaticClass();

	// 우리가 만든 GameState로 설정
	GameStateClass = ASpartaGameState::StaticClass();

}
