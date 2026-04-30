// Fill out your copyright notice in the Description page of Project Settings.


#include "Chapter03/SpartaGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Chapter03/SpawnVolume.h"
#include "Chapter03/CoinItem.h"
#include "Chapter03/SpartaGameInstance.h"
#include "Chapter02/SpartaPlayerController.h"
#include "Components/TextBlock.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"

ASpartaGameState::ASpartaGameState()
{
    Score = 0;
    SpawnedCoinCount = 0;
    CollectedCoinCount = 0;
    LevelDuration = 30.0f; // 한 레벨당 30초
    CurrentLevelIndex = 0;
    MaxLevels = 3;
    CurrentWaveIndex = 0;
}

void ASpartaGameState::BeginPlay()
{
    Super::BeginPlay();

    CachedGameInstance = GetGameInstance<USpartaGameInstance>();
    CachedPlayerController = Cast<ASpartaPlayerController>(
        GetWorld()->GetFirstPlayerController());

    UpdateHUD();
    StartLevel();

    GetWorldTimerManager().SetTimer(
        HUDUpdateTimerHandle,
        this,
        &ASpartaGameState::UpdateHUD,
        0.1f,
        true
    );
}

int32 ASpartaGameState::GetScore() const
{
    return Score;
}

void ASpartaGameState::AddScore(int32 Amount)
{
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance);
        if (SpartaGameInstance)
        {
            SpartaGameInstance->AddToScore(Amount);
        }
    }
}

void ASpartaGameState::StartLevel()
{
    if (CachedPlayerController)
    {
        CachedPlayerController->ShowGameHUD();
    }

    if (CachedGameInstance)
    {
        CurrentLevelIndex = CachedGameInstance->CurrentLevelIndex;
    }

    CurrentWaveIndex = 0;
    StartWave();
}

void ASpartaGameState::StartWave()
{
    if (!Waves.IsValidIndex(CurrentWaveIndex))
    {
        EndLevel();
        return;
    }

    // 이전 웨이브 남은 코인만 제거
    TArray<AActor*> RemainingCoins;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACoinItem::StaticClass(), RemainingCoins);
    for (AActor* Coin : RemainingCoins)
    {
        Coin->Destroy();
    }

    const FWaveInfo& CurrentWave = Waves[CurrentWaveIndex];

    // 레벨 시작 시, 코인 개수 초기화
    SpawnedCoinCount = 0;
    CollectedCoinCount = 0;

    // 아이템 스폰
    TArray<AActor*> FoundVolumes;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpawnVolume::StaticClass(), FoundVolumes);

    for (int32 i = 0; i < CurrentWave.ItemCount; i++)
    {
        if (FoundVolumes.Num() > 0)
        {
            ASpawnVolume* SpawnVolume = Cast<ASpawnVolume>(FoundVolumes[0]);
            if (SpawnVolume)
            {
                AActor* SpawnedActor = SpawnVolume->SpawnRandomItem();

                // 만약 스폰된 액터가 코인 타입이라면 SpawnedCoinCount 증가
                if (SpawnedActor && SpawnedActor->IsA(ACoinItem::StaticClass()))
                {
                    SpawnedCoinCount++;
                }
            }
        }
    }

    // 웨이브 타이머
    LevelDuration = CurrentWave.Duration;
    GetWorldTimerManager().SetTimer(
        LevelTimerHandle,
        this,
        &ASpartaGameState::OnWaveTimeUp,
        LevelDuration,
        false
    );

    // 알림
    GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow,
        FString::Printf(TEXT("Wave %d 시작! (아이템: %d, 시간: %.0f초)"),
            CurrentWaveIndex + 1, CurrentWave.ItemCount, CurrentWave.Duration));

    UE_LOG(LogTemp, Warning, TEXT("Level %d - Wave %d Start! Items: %d, Duration: %.0f"),
        CurrentLevelIndex + 1, CurrentWaveIndex + 1,
        CurrentWave.ItemCount, CurrentWave.Duration);
}

void ASpartaGameState::OnWaveTimeUp()
{
    EndWave();
}

void ASpartaGameState::EndWave()
{
    GetWorldTimerManager().ClearTimer(LevelTimerHandle);

    CurrentWaveIndex++;

    // 남은 웨이브가 있으면 다음 웨이브
    if (CurrentWaveIndex < Waves.Num())
    {
        StartWave();
    }
    else
    {
        // 모든 웨이브 클리어 → 레벨 종료
        EndLevel();
    }
}

void ASpartaGameState::OnLevelTimeUp()
{
    // 시간이 다 되면 레벨을 종료
    EndLevel();
}

void ASpartaGameState::OnCoinCollected()
{
    CollectedCoinCount++;

    UE_LOG(LogTemp, Warning, TEXT("Coin Collected: %d / %d"),
        CollectedCoinCount,
        SpawnedCoinCount)

        // 현재 레벨에서 스폰된 코인을 전부 주웠다면 즉시 레벨 종료
        if (SpawnedCoinCount > 0 && CollectedCoinCount >= SpawnedCoinCount)
        {
            EndWave();
        }
}

void ASpartaGameState::EndLevel()
{
    if (CachedGameInstance)
    {
        CachedGameInstance->AddToScore(Score);
        CurrentLevelIndex++;
        CachedGameInstance->CurrentLevelIndex = CurrentLevelIndex;
    }

    // 모든 레벨을 다 돌았다면 게임 오버 처리
    if (CurrentLevelIndex >= MaxLevels)
    {
        OnGameOver();
        return;
    }

    // 레벨 맵 이름이 있다면 해당 맵 불러오기
    if (LevelMapNames.IsValidIndex(CurrentLevelIndex))
    {
        // OpenLevel시 지금 월드가 제거되고 새로운 맵이 로드되면서 BeginPlay()가 호출됨
        // GameState도 새로 생성되기때문에 이전 레벨에서 유지하던 변수가 모두 초기화 될수있다.
        UGameplayStatics::OpenLevel(GetWorld(), LevelMapNames[CurrentLevelIndex]);
    }
    else
    {
        // 맵 이름이 없으면 게임오버
        OnGameOver();
    }
}

void ASpartaGameState::OnGameOver()
{
    if (CachedPlayerController)
    {
        CachedPlayerController->SetPause(true);
        CachedPlayerController->ShowMainMenu(true, true);
    }
}

void ASpartaGameState::UpdateHUD()
{
    if (!CachedPlayerController || !CachedGameInstance) return;

    if (UUserWidget* HUDWidget = CachedPlayerController->GetHUDWidget())
    {
        if (UTextBlock* TimeText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Time"))))
        {
            float RemainingTime = GetWorldTimerManager().GetTimerRemaining(LevelTimerHandle);
            TimeText->SetText(FText::FromString(FString::Printf(TEXT("Time: %.1f"), RemainingTime)));
        }
        if (UProgressBar* TimeBar = Cast<UProgressBar>(HUDWidget->GetWidgetFromName(TEXT("TimeBar"))))
        {
            float RemainingTime = GetWorldTimerManager().GetTimerRemaining(LevelTimerHandle);
            float Percent = FMath::Clamp(RemainingTime / LevelDuration, 0.0f, 1.0f);
            
            FLinearColor BarColor;
            if (Percent > 0.5f)
                BarColor = FLinearColor::Green;
            else if (Percent > 0.25f)
                BarColor = FLinearColor::Yellow;
            else
                BarColor = FLinearColor::Red;

            TimeBar->SetFillColorAndOpacity(BarColor);
            TimeBar->SetPercent(Percent);
        }

        if (UTextBlock* ScoreText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Score"))))
        {
            ScoreText->SetText(FText::FromString(FString::Printf(TEXT("Score: %d"),
                CachedGameInstance->TotalScore)));
        }

        if (UTextBlock* LevelIndexText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Level"))))
        {
            LevelIndexText->SetText(FText::FromString(FString::Printf(TEXT("Level: %d"),
                CurrentLevelIndex + 1)));
        }

        if (UTextBlock* WaveText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Wave"))))
        {
            WaveText->SetText(FText::FromString(
                FString::Printf(TEXT("Wave: %d / %d"), CurrentWaveIndex + 1, Waves.Num())));
        }

        if (UTextBlock* CoinText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("CoinCount"))))
        {
            int32 RemainingCoins = SpawnedCoinCount - CollectedCoinCount;
            CoinText->SetText(FText::FromString(
                FString::Printf(TEXT("Coins: %d / %d"), RemainingCoins, SpawnedCoinCount)));
        }
    }
}