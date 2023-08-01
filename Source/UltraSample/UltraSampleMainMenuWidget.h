#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UltraSampleMainMenuWidget.generated.h"

/**
 * 
 */
UCLASS()
class ULTRASAMPLE_API UUltraSampleMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	UFUNCTION(BlueprintCallable)
		void UpdatePlayerInfo();

	UPROPERTY(BlueprintReadOnly)
		FString PlayerDispayName = TEXT("{PlayerName}");

	UPROPERTY(BlueprintReadOnly)
		FString WalletId = TEXT("{WalletId}");
};
