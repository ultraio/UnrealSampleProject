#include "UltraSampleMainMenuWidget.h"
#include "OnlineSubsystemUtils.h"
#include "UltraSampleGameInstance.h"

void UUltraSampleMainMenuWidget::UpdatePlayerInfo()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (ULocalPlayer* LocalPlayer = GameInstance->GetFirstGamePlayer())
		{
			FUniqueNetIdRepl UserId = LocalPlayer->GetPreferredUniqueNetId();
			if (UserId.IsValid() && UserId->IsValid())
			{
				if (const IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GameInstance->GetWorld()))
				{
					const IOnlineIdentityPtr IdentityInterface = OnlineSub->GetIdentityInterface();
					if (ensure(IdentityInterface.IsValid()))
					{
						TSharedPtr<FUserOnlineAccount> UserAccount = IdentityInterface->GetUserAccount(*UserId);
						if (UserAccount.IsValid())
						{
							PlayerDispayName = UserAccount->GetDisplayName();
							bool bWalletFound = UserAccount->GetUserAttribute(TEXT("wallet"), WalletId);
							check(bWalletFound);
						}
					}
				}
			}
		}
	}
}
