#include "OnlineSubsystemUltra.h"
#include "OnlineIdentityInterfaceUltra.h"

FOnlineSubsystemUltra::FOnlineSubsystemUltra(FName InInstanceName) : FOnlineSubsystemImpl(TEXT("ULTRA"), InInstanceName)
{
}

FOnlineSubsystemUltra::~FOnlineSubsystemUltra()
{
}

bool FOnlineSubsystemUltra::Init()
{
	StartTicker();
	IdentityInterface = MakeShareable(new FOnlineIdentityUltra(this));
	return true;
}

bool FOnlineSubsystemUltra::Shutdown()
{
	FOnlineSubsystemImpl::Shutdown();
	StopTicker();

	if (IdentityInterface.IsValid())
	{
		IdentityInterface = nullptr;
	}

	return true;
}

IOnlineSessionPtr FOnlineSubsystemUltra::GetSessionInterface() const
{
	return nullptr;
}

IOnlineFriendsPtr FOnlineSubsystemUltra::GetFriendsInterface() const
{
	return nullptr;
}

IOnlineIdentityPtr FOnlineSubsystemUltra::GetIdentityInterface() const
{
	return IdentityInterface;
}

FString FOnlineSubsystemUltra::GetAppId() const
{
	const FString AppId;
	return AppId;
}

FText FOnlineSubsystemUltra::GetOnlineServiceName() const
{
	return NSLOCTEXT("OnlineSubsystemUltra", "OnlineServiceName", "Ultra");
}

bool FOnlineSubsystemUltra::Tick(float DeltaTime)
{
	if (!bTickerStarted)
	{
		return true;
	}

	if (ensure(IdentityInterface.IsValid()))
	{
		IdentityInterface->Tick(DeltaTime);
	}

	return FOnlineSubsystemImpl::Tick(DeltaTime);
}
