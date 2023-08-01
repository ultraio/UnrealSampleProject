// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "UltraSampleGameInstance.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLoginEvent, bool, bSuccess, int32, UserIndex);

/**
 * 
 */
UCLASS()
class ULTRASAMPLE_API UUltraSampleGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable)
		FLoginEvent OnLoginEvent;

	UFUNCTION(BlueprintCallable)
		void SkipWelcomeScreen(int32 UserIndex);

private:
	FDelegateHandle LoginCompleteHandle;
	void OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);
};
