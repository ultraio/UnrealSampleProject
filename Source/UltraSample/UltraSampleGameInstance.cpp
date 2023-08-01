#include "UltraSampleGameInstance.h"
#include "OnlineSubsystemUtils.h"
#include "Components/ListView.h"

void UUltraSampleGameInstance::SkipWelcomeScreen(int32 UserIndex)
{
	if (const IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld()))
	{
		const IOnlineIdentityPtr IdentityInterface = OnlineSub->GetIdentityInterface();
		if (ensure(IdentityInterface.IsValid()))
		{
			const ELoginStatus::Type LoginStatus = IdentityInterface->GetLoginStatus(UserIndex);
			if (LoginStatus == ELoginStatus::NotLoggedIn)
			{
				LoginCompleteHandle = IdentityInterface->AddOnLoginCompleteDelegate_Handle(
					UserIndex,
					FOnLoginCompleteDelegate::CreateUObject(this, &UUltraSampleGameInstance::OnLoginComplete)
				);

				IdentityInterface->AutoLogin(UserIndex);
			}
		}
	}
}

void UUltraSampleGameInstance::OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	if (bWasSuccessful)
	{
		if (ULocalPlayer* LocalPlayer = GetFirstGamePlayer())
		{
			if (const IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld()))
			{
				const IOnlineIdentityPtr IdentityInterface = OnlineSub->GetIdentityInterface();
				if (ensure(IdentityInterface.IsValid()))
				{
					IdentityInterface->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, LoginCompleteHandle);
					FUniqueNetIdPtr UserId = IdentityInterface->GetUniquePlayerId(LocalUserNum);
					LocalPlayer->SetCachedUniqueNetId(UserId);
				}
			}
		}
	}

	if (OnLoginEvent.IsBound())
	{
		OnLoginEvent.Broadcast(bWasSuccessful, LocalUserNum);
	}
}
