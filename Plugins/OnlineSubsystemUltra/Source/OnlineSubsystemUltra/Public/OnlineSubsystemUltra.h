#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystemImpl.h"

class FOnlineIdentityUltra;

typedef TSharedPtr<class FOnlineIdentityUltra, ESPMode::ThreadSafe> FOnlineIdentityUltraPtr;

class ONLINESUBSYSTEMULTRA_API FOnlineSubsystemUltra : 
	public FOnlineSubsystemImpl
{
public:

	FOnlineSubsystemUltra(FName InInstanceName);
	virtual ~FOnlineSubsystemUltra();

	virtual bool Init() override;
	virtual bool Shutdown() override;
	
	virtual IOnlineSessionPtr GetSessionInterface() const override;
	virtual IOnlineFriendsPtr GetFriendsInterface() const override;
	virtual IOnlineIdentityPtr GetIdentityInterface() const override;
	
	virtual FString GetAppId() const override;

	virtual FText GetOnlineServiceName() const override;

	virtual bool Tick(float DeltaTime) override;

private:
	FOnlineIdentityUltraPtr IdentityInterface;
};

typedef TSharedPtr<FOnlineSubsystemUltra, ESPMode::ThreadSafe> FOnlineSubsystemUltraPtr;

