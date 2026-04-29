// Fill out your copyright notice in the Description page of Project Settings.


#include "Chapter03/KeyItem.h"

AKeyItem::AKeyItem()
{
	ItemType = "Key";
}

void AKeyItem::OnItemOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1, 5, FColor::Cyan, FString::Printf(TEXT("Key collected!")));
	}
	
}

void AKeyItem::ActivateItem(AActor* Activator)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1, 5, FColor::Blue, FString::Printf(TEXT("Key used to unlock the door!")));
	}
}
