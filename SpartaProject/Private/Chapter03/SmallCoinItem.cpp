// Fill out your copyright notice in the Description page of Project Settings.


#include "Chapter03/SmallCoinItem.h"

ASmallCoinItem::ASmallCoinItem()
{
	PointValue = 10;
	ItemType = "SmallCoin";
}

void ASmallCoinItem::ActivateItem(AActor* Activator)
{
	// 부모의 기본 점수 획득 로직 사용
	Super::ActivateItem(Activator);

	// 스몰 코인만의 별도 동작은 여기에 추가
}
