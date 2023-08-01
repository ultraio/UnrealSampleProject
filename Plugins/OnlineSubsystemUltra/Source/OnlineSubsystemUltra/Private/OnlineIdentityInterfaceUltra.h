#pragma once

#include "CoreMinimal.h"
#include "Online/CoreOnline.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Http.h"

#include "OnlineSubsystemUltraTypes.h"

struct FUltraTokenRequest
{
	bool bAuthInProgress = false;

	// OAuth fields
	int32 Interval = 0; // seconds
	FString DeviceCode;
	FString ValidationUrl;
	FDateTime StartDate;
	FDateTime NextRequestDate;
	FDateTime ExpiresDate;

	void Reset()
	{
		bAuthInProgress = false;

		Interval = 0; // seconds
		DeviceCode.Empty();
		StartDate = FDateTime();
		NextRequestDate = FDateTime();
		ExpiresDate = FDateTime();
	}
};

class FOnlineIdentityUltra :
	public IOnlineIdentity
{
public:

	FOnlineIdentityUltra(class FOnlineSubsystemUltra* InSubsystem);
	virtual ~FOnlineIdentityUltra();
	
	virtual bool Login(int32 LocalUserNum, const FOnlineAccountCredentials& AccountCredentials) override;
	virtual bool Logout(int32 LocalUserNum) override;
	virtual bool AutoLogin(int32 LocalUserNum) override;

private:

	void HandleAuthDeviceResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully, int32 LocalUserNum);
	void HandleTokenResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully, int32 LocalUserNum);
	void HandleUserInfoResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully, int32 LocalUserNum, FUltraValidToken ValidTokenCopy);

	void OpenVerificationUrl(const FString& Url);
	void CancelTokenRequest(int32 LocalUserNum);
	void CancelAuthentication(int32 LocalUserNum);

public:

	virtual TSharedPtr<FUserOnlineAccount> GetUserAccount(const FUniqueNetId& UserId) const override;
	virtual TArray<TSharedPtr<FUserOnlineAccount> > GetAllUserAccounts() const override;
	virtual FUniqueNetIdPtr GetUniquePlayerId(int32 LocalUserNum) const override;
	virtual FUniqueNetIdPtr CreateUniquePlayerId(uint8* Bytes, int32 Size) override;
	virtual FUniqueNetIdPtr CreateUniquePlayerId(const FString& Str) override;
	virtual ELoginStatus::Type GetLoginStatus(int32 LocalUserNum) const override;
	virtual ELoginStatus::Type GetLoginStatus(const FUniqueNetId& UserId) const override;
	virtual FString GetPlayerNickname(int32 LocalUserNum) const override;
	virtual FString GetPlayerNickname(const FUniqueNetId& UserId) const override;
	virtual FString GetAuthToken(int32 LocalUserNum) const override;
	virtual void RevokeAuthToken(const FUniqueNetId& LocalUserId, const FOnRevokeAuthTokenCompleteDelegate& Delegate) override;
	virtual void GetUserPrivilege(const FUniqueNetId& LocalUserId, EUserPrivileges::Type Privilege, const FOnGetUserPrivilegeCompleteDelegate& Delegate) override;
	virtual FPlatformUserId GetPlatformUserIdFromUniqueNetId(const FUniqueNetId& UniqueNetId) const override;
	virtual FString GetAuthType() const override;

	void Tick(float DeltaTime);

private:

	class FOnlineSubsystemUltra* Subsystem;

	FString ClientId;
	FString AuthenticationUrl;
	FString ApplicationProtocol;
	bool bUseBrowser;

	FUltraTokenRequest PendingTokenRequests[MAX_LOCAL_PLAYERS];
	FUniqueNetIdPtr LocalUserMap[MAX_LOCAL_PLAYERS];
	TMap<FUniqueNetIdPtr, TSharedPtr<FUserOnlineAccountUltra>> UltraOnlineAccounts;
};

typedef TSharedPtr<FOnlineIdentityUltra, ESPMode::ThreadSafe> FOnlineIdentityUltraPtr;
