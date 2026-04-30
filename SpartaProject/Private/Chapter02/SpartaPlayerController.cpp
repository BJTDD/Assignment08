// Fill out your copyright notice in the Description page of Project Settings.


#include "Chapter02/SpartaPlayerController.h"
#include "Chapter03/SpartaGameState.h"
#include "Chapter03/SpartaGameInstance.h"
// Enhanced Input System의 Local Player Subsystem을 사용하기위해 Include
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Blueprint/UserWidget.h" 
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

ASpartaPlayerController::ASpartaPlayerController()
	:InputMappingContext(nullptr),
	MoveAction(nullptr),
	JumpAction(nullptr),
	LookAction(nullptr),
	SprintAction(nullptr),
	MenuToggleAction(nullptr),
	HUDWidgetClass(nullptr),
	HUDWidgetInstance(nullptr),
	MainMenuWidgetClass(nullptr),
	MainMenuWidgetInstance(nullptr)
{

}

void ASpartaPlayerController::BeginPlay()
{
	// 현재 PlayerController에 연결된 Local Player 객체를 가져옴
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		// Local Player에서 EnhancedInputLocalPlayerSubsystem을 획득
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (InputMappingContext)
			{
				// Subsystem을 통해 우리가 할당한 IMC를 활성화
				// 우선순위(Priority)는 0이 가장 높은 우선순위
				Subsystem->AddMappingContext(InputMappingContext, 0);
			}
		}
	}

	// 게임 실행 시 메뉴 레벨에서 메뉴 UI 먼저 표시
	FString CurrentMapName = GetWorld()->GetMapName();
	if (CurrentMapName.Contains("MenuLevel"))
	{
		ShowMainMenu(false, false);
	}
}

UUserWidget* ASpartaPlayerController::GetHUDWidget() const
{
	return HUDWidgetInstance;
}

// 메뉴 UI 표시
void ASpartaPlayerController::ShowMainMenu(bool bIsRestart, bool bIsGameOver)
{
	// HUD가 켜져 있다면 닫기
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->RemoveFromParent();
		HUDWidgetInstance = nullptr;
	}

	// 이미 메뉴가 떠 있으면 제거
	if (MainMenuWidgetInstance)
	{
		MainMenuWidgetInstance->RemoveFromParent();
		MainMenuWidgetInstance = nullptr;
	}

	// 메뉴 UI 생성
	if (MainMenuWidgetClass)
	{
		MainMenuWidgetInstance = CreateWidget<UUserWidget>(this, MainMenuWidgetClass);
		if (MainMenuWidgetInstance)
		{
			MainMenuWidgetInstance->AddToViewport();
			bShowMouseCursor = true;
			FInputModeGameAndUI InputMode;
			InputMode.SetHideCursorDuringCapture(false);
			SetInputMode(InputMode);
		}

		// 위젯 가져오기
		UWidget* StartBtn = MainMenuWidgetInstance->GetWidgetFromName(TEXT("StartButton"));
		UWidget* RestartBtn = MainMenuWidgetInstance->GetWidgetFromName(TEXT("RestartButton"));
		UWidget* QuitBtn = MainMenuWidgetInstance->GetWidgetFromName(TEXT("QuitButton"));
		UTextBlock* TitleText = Cast<UTextBlock>(MainMenuWidgetInstance->GetWidgetFromName(TEXT("TitleText")));
		UWidget* TotalScoreText = MainMenuWidgetInstance->GetWidgetFromName(TEXT("TotalScoreText"));

		if (bIsRestart)
		{
			// Tab 또는 게임오버: Start 숨기고 Restart, Quit 보이기
			if (StartBtn) StartBtn->SetVisibility(ESlateVisibility::Collapsed);
			if (RestartBtn) RestartBtn->SetVisibility(ESlateVisibility::Visible);
			if (QuitBtn) QuitBtn->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			// 첫 시작: Start만 보이기
			if (StartBtn) StartBtn->SetVisibility(ESlateVisibility::Visible);
			if (RestartBtn) RestartBtn->SetVisibility(ESlateVisibility::Collapsed);
			if (QuitBtn) QuitBtn->SetVisibility(ESlateVisibility::Visible);
		}

		// 게임오버일 때만 애니메이션, 점수 표시
		if (bIsGameOver)
		{
			// 타이틀 → Game Over
			if (TitleText) TitleText->SetText(FText::FromString(TEXT("Game Over")));

			UFunction* PlayAnimFunc = MainMenuWidgetInstance->FindFunction(FName("PlayGameOverAnim"));
			if (PlayAnimFunc)
			{
				MainMenuWidgetInstance->ProcessEvent(PlayAnimFunc, nullptr);
			}

			// 점수 표시
			if (TotalScoreText) TotalScoreText->SetVisibility(ESlateVisibility::Visible);
			if (UTextBlock* ScoreText = Cast<UTextBlock>(TotalScoreText))
			{
				if (USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(UGameplayStatics::GetGameInstance(this)))
				{
					ScoreText->SetText(FText::FromString(
						FString::Printf(TEXT("Total Score: %d"), SpartaGameInstance->TotalScore)
					));
				}
			}
		}
		else
		{
			// 타이틀 → 게임 이름
			if (TitleText)
			{
				TitleText->SetText(FText::FromString(TEXT("Sparta Game")));
				TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.84f, 0.0f)));

				FSlateFontInfo FontInfo = TitleText->GetFont();
				FontInfo.Size = 92;
				TitleText->SetFont(FontInfo);
			}
			if (TotalScoreText) TotalScoreText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void ASpartaPlayerController::ShowGameHUD()
{
	// HUD가 켜져 있다면 닫기
	if (HUDWidgetInstance)
	{
		HUDWidgetInstance->RemoveFromParent();
		HUDWidgetInstance = nullptr;
	}

	// 이미 메뉴가 떠 있으면 제거
	if (MainMenuWidgetInstance)
	{
		MainMenuWidgetInstance->RemoveFromParent();
		MainMenuWidgetInstance = nullptr;
	}

	// HUD 생성
	if (HUDWidgetClass)
	{
		HUDWidgetInstance = CreateWidget<UUserWidget>(this, HUDWidgetClass);
		if (HUDWidgetInstance)
		{
			HUDWidgetInstance->AddToViewport();

			bShowMouseCursor = false;
			SetInputMode(FInputModeGameOnly());
		}
		
		ASpartaGameState* SpartaGameState = GetWorld() ? GetWorld()->GetGameState<ASpartaGameState>() : nullptr;
		if (SpartaGameState)
		{
			SpartaGameState->UpdateHUD();
		}
	}
}

// 게임 시작 - BasicLevel 오픈, GameInstance 데이터 리셋
void ASpartaPlayerController::StartGame()
{
	if (USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		SpartaGameInstance->CurrentLevelIndex = 0;
		SpartaGameInstance->TotalScore = 0;
	}

	UGameplayStatics::OpenLevel(GetWorld(), FName("BasicLevel"));
	SetPause(false);
}

void ASpartaPlayerController::RestartGame()
{
	if (USpartaGameInstance* GI = Cast<USpartaGameInstance>(
		UGameplayStatics::GetGameInstance(this)))
	{
		GI->CurrentLevelIndex = 0;
		GI->TotalScore = 0;
	}

	UGameplayStatics::OpenLevel(GetWorld(), FName("BasicLevel"));
}

void ASpartaPlayerController::ToggleMenu()
{
	FString MapName = GetWorld()->GetMapName();
	if (MapName.Contains("MenuLevel")) return;

	if (MainMenuWidgetInstance)
	{
		MainMenuWidgetInstance->RemoveFromParent();
		MainMenuWidgetInstance = nullptr;
		bShowMouseCursor = false;
		SetInputMode(FInputModeGameOnly());
		ShowGameHUD();
		return;
	}

	ShowMainMenu(true, false);
}

void ASpartaPlayerController::QuitGame()
{
	UKismetSystemLibrary::QuitGame(GetWorld(), this, EQuitPreference::Quit, false);
}