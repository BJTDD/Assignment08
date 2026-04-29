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

void ASpartaGameState::CheckScoreRewards()
{
    // 임계값과 보상 메시지를 쌍으로 정의
    struct FScoreReward
    {
        int32 Threshold;
        const TCHAR* Message;
    };

    // 원하는 만큼 여기에 추가하면 됨
    static const FScoreReward Rewards[] =
    {
        { 50,  TEXT("50점 달성! 이동 속도가 증가합니다!") },
        { 100, TEXT("축하합니다! 보너스 체력 20을 획득했습니다.") },
        { 200, TEXT("200점 달성! 특별 아이템이 해금되었습니다!") },
    };

    for (const FScoreReward& Reward : Rewards)
    {
        // 점수가 임계값 이상이고, 아직 지급하지 않았다면
        if (Score >= Reward.Threshold && !RewardedThresholds.Contains(Reward.Threshold))
        {
            RewardedThresholds.Add(Reward.Threshold);
            UE_LOG(LogTemp, Warning, TEXT("[보상] %s"), Reward.Message);

            // 여기서 실제 보상 로직을 추가할 수 있음
            // 예: 100점이면 플레이어 체력 +20 등
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

    // 레벨 시작 시, 코인 개수 초기화
    SpawnedCoinCount = 0;
    CollectedCoinCount = 0;

    // 현재 맵에 배치된 모든 SpawnVolume을 찾아 아이템 40개를 스폰
    TArray<AActor*> FoundVolumes;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpawnVolume::StaticClass(), FoundVolumes);

    const int32 ItemToSpawn = 40;

    for (int32 i = 0; i < ItemToSpawn; i++)
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

    // 30초 후에 OnLevelTimeUp()가 호출되도록 타이머 설정
    GetWorldTimerManager().SetTimer(
        LevelTimerHandle,
        this,
        &ASpartaGameState::OnLevelTimeUp,
        LevelDuration,
        false
    );

    UE_LOG(LogTemp, Warning, TEXT("Level %d Start!, Spawned %d coin"),
        CurrentLevelIndex + 1,
        SpawnedCoinCount);
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
            EndLevel();
        }
}

void ASpartaGameState::EndLevel()
{
    // 타이머 해제
    GetWorldTimerManager().ClearTimer(LevelTimerHandle);

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance);
        if (SpartaGameInstance)
        {
            AddScore(Score);
            CurrentLevelIndex++;
            SpartaGameInstance->CurrentLevelIndex = CurrentLevelIndex;
        }
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
    }
}