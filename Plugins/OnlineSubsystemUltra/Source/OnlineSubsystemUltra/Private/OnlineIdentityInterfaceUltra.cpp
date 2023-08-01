#include "OnlineIdentityInterfaceUltra.h"

#include "OnlineSubsystemUltra.h"
#include "OnlineSubsystemUltraPrivate.h"

bool CheckLocalUserNum(int32 LocalUserNum)
{
	if (LocalUserNum < 0 || LocalUserNum > MAX_LOCAL_PLAYERS)
	{
		return false;
	}

	return true;
}

FOnlineIdentityUltra::FOnlineIdentityUltra(class FOnlineSubsystemUltra* InSubsystem)
: AuthenticationUrl(TEXT("https://auth.ultra.io/auth/realms/ultraio/protocol/openid-connect"))
, ApplicationProtocol(TEXT("ultra"))
, bUseBrowser(false)
{
	const TCHAR* SectionName(TEXT("OnlineSubsystemUltra"));
	GConfig->GetString(SectionName, TEXT("ClientId"), ClientId, GEngineIni);
	GConfig->GetString(SectionName, TEXT("AuthenticationUrl"), AuthenticationUrl, GEngineIni);
	GConfig->GetString(SectionName, TEXT("ApplicationProtocol"), ApplicationProtocol, GEngineIni);
	GConfig->GetBool(SectionName, TEXT("bUseBrowser"), bUseBrowser, GEngineIni);
}

FOnlineIdentityUltra::~FOnlineIdentityUltra()
{
}

bool FOnlineIdentityUltra::Login(int32 LocalUserNum, const FOnlineAccountCredentials& AccountCredentials)
{
	if (!CheckLocalUserNum(LocalUserNum))
	{
		UE_LOG_ONLINE_IDENTITY(Error, TEXT("Invalid LocalPlayerNum: %d"), LocalUserNum);
		TriggerOnLoginCompleteDelegates(LocalUserNum, false, *FUniqueNetIdUltra::EmptyId(), TEXT("Authentication canceled"));
		return false;
	}

	if (AccountCredentials.Token.IsEmpty())
	{
		UE_LOG_ONLINE_IDENTITY(Error, TEXT("Invalid credentials"));
		CancelAuthentication(LocalUserNum);
		return false;
	}

	TSharedPtr<FJsonObject> JsonToken;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(AccountCredentials.Token);

	bool bSuccess = true;
	if (FJsonSerializer::Deserialize(JsonReader, JsonToken) &&
		JsonToken.IsValid() &&
		JsonToken->HasTypedField<EJson::String>(TEXT("access_token")) &&
		JsonToken->HasTypedField<EJson::Number>(TEXT("expires_in")) &&
		JsonToken->HasTypedField<EJson::Number>(TEXT("refresh_expires_in")) &&
		JsonToken->HasTypedField<EJson::String>(TEXT("refresh_token")) &&
		JsonToken->HasTypedField<EJson::String>(TEXT("token_type")) &&
		JsonToken->HasTypedField<EJson::String>(TEXT("id_token")) &&
		JsonToken->HasTypedField<EJson::Number>(TEXT("not-before-policy")) &&
		JsonToken->HasTypedField<EJson::String>(TEXT("session_state")) &&
		JsonToken->HasTypedField<EJson::String>(TEXT("scope")))
	{
		PendingTokenRequests[LocalUserNum].Reset();

		FUltraValidToken ValidToken;
		ValidToken.AccessToken = JsonToken->GetStringField(TEXT("access_token"));
		ValidToken.IdToken = JsonToken->GetStringField(TEXT("id_token"));
		ValidToken.RefreshToken = JsonToken->GetStringField(TEXT("refresh_token"));
		ValidToken.TokenType = JsonToken->GetStringField(TEXT("token_type"));
		ValidToken.ExpiresDate = FDateTime::Now() + FTimespan::FromSeconds(StaticCast<int32>(JsonToken->GetNumberField(TEXT("expires_in"))));
		ValidToken.RefreshExpiresDate = FDateTime::Now() + FTimespan::FromSeconds(StaticCast<int32>(JsonToken->GetNumberField(TEXT("refresh_expires_in"))));

		const FString AuthDeviceUrl(FString::Printf(TEXT("%s/userinfo"), *AuthenticationUrl));

		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
		HttpRequest->OnProcessRequestComplete().BindRaw(this, &FOnlineIdentityUltra::HandleUserInfoResponse, LocalUserNum, ValidToken);
		HttpRequest->SetURL(AuthDeviceUrl);
		HttpRequest->SetHeader(TEXT("Authorization"), *FString::Printf(TEXT("%s %s"), *ValidToken.TokenType, *ValidToken.AccessToken));
		HttpRequest->SetVerb(TEXT("GET"));

		HttpRequest->ProcessRequest();
	}
	else // Error
	{
		if (JsonToken.IsValid() && JsonToken->HasTypedField<EJson::String>(TEXT("error")) && JsonToken->HasTypedField<EJson::String>(TEXT("error_description")))
		{
			const FString ErrorString = JsonToken->GetStringField(TEXT("error"));
			if (ErrorString == TEXT("authorization_pending") || ErrorString == TEXT("slow_down"))
			{
				UE_LOG_ONLINE_IDENTITY(Log, TEXT("Pending request for device %s"), *PendingTokenRequests[LocalUserNum].DeviceCode);
			}
			else
			{
				bSuccess = false;
				UE_LOG_ONLINE_IDENTITY(Error, TEXT("Token request failed with error: %s (%s)"), *ErrorString, *JsonToken->GetStringField(TEXT("error_description")));
				CancelTokenRequest(LocalUserNum);
			}
		}
		else
		{
			bSuccess = false;
			UE_LOG_ONLINE_IDENTITY(Error, TEXT("Invalid token response json"));
			CancelTokenRequest(LocalUserNum);
		}
	}

	return bSuccess;
}

bool FOnlineIdentityUltra::Logout(int32 LocalUserNum)
{
	unimplemented();
	return false;
}

bool FOnlineIdentityUltra::AutoLogin(int32 LocalUserNum)
{
	if (!CheckLocalUserNum(LocalUserNum))
	{
		UE_LOG_ONLINE_IDENTITY(Error, TEXT("Invalid LocalPlayerNum: %d"), LocalUserNum);
		TriggerOnLoginCompleteDelegates(LocalUserNum, false, *FUniqueNetIdUltra::EmptyId(), TEXT("Invalid local user"));
		return false;
	}

	if (AuthenticationUrl.IsEmpty() || ClientId.IsEmpty())
	{
		UE_LOG_ONLINE_IDENTITY(Error, TEXT("Missing AuthenticationUrl or ClientId in ini file"));
		TriggerOnLoginCompleteDelegates(LocalUserNum, false, *FUniqueNetIdUltra::EmptyId(), TEXT("Invalid settings"));
		return false;
	}

	if (PendingTokenRequests[LocalUserNum].bAuthInProgress)
	{
		UE_LOG_ONLINE_IDENTITY(Warning, TEXT("Authentication already pending for local user %d"), LocalUserNum);

		if (!PendingTokenRequests[LocalUserNum].DeviceCode.IsEmpty())
		{
			OpenVerificationUrl(PendingTokenRequests[LocalUserNum].ValidationUrl);
		}

		return true;
	}

	PendingTokenRequests[LocalUserNum].Reset();
	PendingTokenRequests[LocalUserNum].bAuthInProgress = true;

	const FString AuthDeviceUrl(FString::Printf(TEXT("%s/auth/device"), *AuthenticationUrl));
	const FString Content(FString::Printf(TEXT("client_id=%s&scope=openid"), *ClientId));

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->OnProcessRequestComplete().BindRaw(this, &FOnlineIdentityUltra::HandleAuthDeviceResponse, LocalUserNum);
	HttpRequest->SetURL(AuthDeviceUrl);
	HttpRequest->SetContentAsString(Content);
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/x-www-form-urlencoded"));
	HttpRequest->SetVerb(TEXT("POST"));

	return HttpRequest->ProcessRequest();
}

void FOnlineIdentityUltra::HandleAuthDeviceResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully, int32 LocalUserNum)
{
	if (!bConnectedSuccessfully)
	{
		UE_LOG_ONLINE_IDENTITY(Error, TEXT("Unable to send the auth request"));
		CancelTokenRequest(LocalUserNum);
		return;
	}

	if (!Response.IsValid() && Response->GetContentType().StartsWith(TEXT("application/json")))
	{
		UE_LOG_ONLINE_IDENTITY(Error, TEXT("Invalid auth response"));
		CancelTokenRequest(LocalUserNum);
		return;
	}

	const FString ResponseStr = Response->GetContentAsString();
	if (ResponseStr.IsEmpty())
	{
		UE_LOG_ONLINE_IDENTITY(Error, TEXT("Empty auth response"));
		CancelTokenRequest(LocalUserNum);
		return;
	}

	TSharedPtr<FJsonObject> JsonAuthDevice;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ResponseStr);

	if (FJsonSerializer::Deserialize(JsonReader, JsonAuthDevice) &&
		JsonAuthDevice.IsValid() &&
		JsonAuthDevice->HasTypedField<EJson::String>(TEXT("device_code")) &&
		JsonAuthDevice->HasTypedField<EJson::String>(TEXT("user_code")) &&
		JsonAuthDevice->HasTypedField<EJson::String>(TEXT("verification_uri")) &&
		JsonAuthDevice->HasTypedField<EJson::String>(TEXT("verification_uri_complete")) &&
		JsonAuthDevice->HasTypedField<EJson::Number>(TEXT("expires_in")) &&
		JsonAuthDevice->HasTypedField<EJson::Number>(TEXT("interval")))
	{
		FUltraTokenRequest& TokenRequest = PendingTokenRequests[LocalUserNum];
		// Adding 1 second to avoid slow_down error
		TokenRequest.Interval = StaticCast<int32>(JsonAuthDevice->GetNumberField(TEXT("interval"))) + 1;
		TokenRequest.DeviceCode = JsonAuthDevice->GetStringField(TEXT("device_code"));
		TokenRequest.ValidationUrl = JsonAuthDevice->GetStringField(TEXT("verification_uri_complete"));
		TokenRequest.StartDate = FDateTime::Now();
		// We start the first request earlier
		TokenRequest.NextRequestDate = TokenRequest.StartDate + FTimespan::FromSeconds(1);
		TokenRequest.ExpiresDate = TokenRequest.StartDate + FTimespan::FromSeconds(StaticCast<int32>(JsonAuthDevice->GetNumberField(TEXT("expires_in"))));

		OpenVerificationUrl(TokenRequest.ValidationUrl);
	}
	else // Error
	{
		CancelTokenRequest(LocalUserNum);
		if (JsonAuthDevice->HasTypedField<EJson::String>(TEXT("error")) && JsonAuthDevice->HasTypedField<EJson::String>(TEXT("error_description")))
		{
			UE_LOG_ONLINE_IDENTITY(Error, TEXT("Auth device failed with error: %s (%s)"), *JsonAuthDevice->GetStringField(TEXT("error")), *JsonAuthDevice->GetStringField(TEXT("error_description")));
		}
		else
		{
			UE_LOG_ONLINE_IDENTITY(Error, TEXT("Invalid auth device response json"));
		}
	}
}

void FOnlineIdentityUltra::HandleTokenResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully, int32 LocalUserNum)
{
	if (!bConnectedSuccessfully)
	{
		UE_LOG_ONLINE_IDENTITY(Error, TEXT("Unable to send the token request"));
		// Do not cancel request since the next retry might be successful
		return;
	}

	if (!Response.IsValid() && Response->GetContentType().StartsWith(TEXT("application/json")))
	{
		UE_LOG_ONLINE_IDENTITY(Error, TEXT("Invalid token response"));
		// Do not cancel request since the next retry might be successful
		return;
	}

	const FString ResponseStr = Response->GetContentAsString();
	if (ResponseStr.IsEmpty())
	{
		UE_LOG_ONLINE_IDENTITY(Error, TEXT("Empty token response"));
		// Do not cancel request since the next retry might be successful
		return;
	}

	FOnlineAccountCredentials AccountCredentials;
	AccountCredentials.Token = ResponseStr;
	Login(LocalUserNum, AccountCredentials);
}

void FOnlineIdentityUltra::HandleUserInfoResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully, int32 LocalUserNum, FUltraValidToken ValidTokenCopy)
{
	if (!bConnectedSuccessfully)
	{
		UE_LOG_ONLINE_IDENTITY(Error, TEXT("Unable to send the user info request"));
		CancelAuthentication(LocalUserNum);
		return;
	}

	if (!Response.IsValid() && Response->GetContentType().StartsWith(TEXT("application/json")))
	{
		UE_LOG_ONLINE_IDENTITY(Error, TEXT("Invalid user info response"));
		CancelAuthentication(LocalUserNum);
		return;
	}

	const FString ResponseStr = Response->GetContentAsString();
	if (ResponseStr.IsEmpty())
	{
		UE_LOG_ONLINE_IDENTITY(Error, TEXT("Empty user info response"));
		CancelAuthentication(LocalUserNum);
		return;
	}

	TSharedPtr<FJsonObject> JsonUserInfo;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ResponseStr);

	if (FJsonSerializer::Deserialize(JsonReader, JsonUserInfo)
		&& JsonUserInfo.IsValid()
		&& JsonUserInfo->HasTypedField<EJson::String>(TEXT("sub"))
		&& JsonUserInfo->HasTypedField<EJson::String>(TEXT("upn"))
		&& JsonUserInfo->HasTypedField<EJson::String>(TEXT("preferred_username"))
		&& JsonUserInfo->HasTypedField<EJson::String>(TEXT("wid")))
	{
		LocalUserMap[LocalUserNum] = CreateUniquePlayerId(JsonUserInfo->GetStringField(TEXT("sub")));

		if (LocalUserMap[LocalUserNum].IsValid())
		{
			const FString& UserName = JsonUserInfo->GetStringField(TEXT("upn"));
			const FString& DisplayName = JsonUserInfo->GetStringField(TEXT("preferred_username"));
			const FString& WalletId = JsonUserInfo->GetStringField(TEXT("wid"));

			TSharedPtr<FUserOnlineAccountUltra> OnlineAccount = MakeShared<FUserOnlineAccountUltra>(LocalUserMap[LocalUserNum].ToSharedRef(), ValidTokenCopy, UserName, DisplayName, WalletId);
			UltraOnlineAccounts.Add(LocalUserMap[LocalUserNum], OnlineAccount);

			TriggerOnLoginCompleteDelegates(LocalUserNum, true, *LocalUserMap[LocalUserNum], TEXT(""));
		}
		else
		{
			UE_LOG_ONLINE_IDENTITY(Error, TEXT("Cannot create unique id"));
			CancelAuthentication(LocalUserNum);
		}
	}
	else // Error
	{
		if (JsonUserInfo->HasTypedField<EJson::String>(TEXT("error")) && JsonUserInfo->HasTypedField<EJson::String>(TEXT("error_description")))
		{
			UE_LOG_ONLINE_IDENTITY(Error, TEXT("User info request failed with error: %s (%s)"), *JsonUserInfo->GetStringField(TEXT("error")), *JsonUserInfo->GetStringField(TEXT("error_description")));
			CancelAuthentication(LocalUserNum);
		}
		else
		{
			UE_LOG_ONLINE_IDENTITY(Error, TEXT("Invalid user info response json"));
			CancelAuthentication(LocalUserNum);
		}
	}
}

void FOnlineIdentityUltra::OpenVerificationUrl(const FString& Url)
{
	FString Prefix = bUseBrowser ? "" : (ApplicationProtocol + TEXT("://auth?url="));
	FString VerificationUrl = Prefix + Url;
	FPlatformProcess::LaunchURL(*VerificationUrl, nullptr, nullptr);
}

void FOnlineIdentityUltra::CancelTokenRequest(int32 LocalUserNum)
{
	PendingTokenRequests[LocalUserNum].Reset();
	TriggerOnLoginCompleteDelegates(LocalUserNum, false, *FUniqueNetIdUltra::EmptyId(), TEXT("Token request canceled"));
}

void FOnlineIdentityUltra::CancelAuthentication(int32 LocalUserNum)
{
	UltraOnlineAccounts.Remove(GetUniquePlayerId(LocalUserNum));
	LocalUserMap[LocalUserNum].Reset();
	CancelTokenRequest(LocalUserNum);
	TriggerOnLoginCompleteDelegates(LocalUserNum, false, *FUniqueNetIdUltra::EmptyId(), TEXT("Authentication canceled"));
}

TSharedPtr<FUserOnlineAccount> FOnlineIdentityUltra::GetUserAccount(const FUniqueNetId& UserId) const
{
	const TSharedPtr<FUserOnlineAccountUltra>* UserAccount = UltraOnlineAccounts.Find(UserId.AsShared());
	if (UserAccount)
	{
		return *UserAccount;
	}
	return nullptr;
}

TArray<TSharedPtr<FUserOnlineAccount> > FOnlineIdentityUltra::GetAllUserAccounts() const
{
	TArray<TSharedPtr<FUserOnlineAccount> > UserAccounts;
	for (auto& UserAccount : UltraOnlineAccounts)
	{
		UserAccounts.Add(UserAccount.Value);
	}
	return UserAccounts;
}

FUniqueNetIdPtr FOnlineIdentityUltra::GetUniquePlayerId(int32 LocalUserNum) const
{
	if (CheckLocalUserNum(LocalUserNum) && LocalUserMap[LocalUserNum].IsValid())
	{
		return LocalUserMap[LocalUserNum];
	}

	return nullptr;
}

FUniqueNetIdPtr FOnlineIdentityUltra::CreateUniquePlayerId(uint8* Bytes, int32 Size)
{
	return FUniqueNetIdUltra::Create(BytesToString(Bytes, Size));
}

FUniqueNetIdPtr FOnlineIdentityUltra::CreateUniquePlayerId(const FString& Str)
{
	return FUniqueNetIdUltra::Create(Str);
}

ELoginStatus::Type FOnlineIdentityUltra::GetLoginStatus(int32 LocalUserNum) const
{
	FUniqueNetIdPtr UniquePlayerId = GetUniquePlayerId(LocalUserNum);

	if (UniquePlayerId.IsValid())
	{
		return GetLoginStatus(*UniquePlayerId);
	}

	return ELoginStatus::NotLoggedIn;
}

ELoginStatus::Type FOnlineIdentityUltra::GetLoginStatus(const FUniqueNetId& UserId) const
{

	for (const auto& LocalUser: LocalUserMap)
	{
		if (LocalUser.IsValid() && *LocalUser == UserId)
		{
			return ELoginStatus::LoggedIn;
		}
	}

	return ELoginStatus::NotLoggedIn;
}

FString FOnlineIdentityUltra::GetPlayerNickname(int32 LocalUserNum) const
{
	FUniqueNetIdPtr UniquePlayerId = GetUniquePlayerId(LocalUserNum);
	if (UniquePlayerId.IsValid())
	{
		return GetPlayerNickname(*UniquePlayerId);
	}
	return FString();
}

FString FOnlineIdentityUltra::GetPlayerNickname(const FUniqueNetId& UserId) const
{
	TSharedPtr<FUserOnlineAccount> UserAccount = GetUserAccount(UserId);
	if (UserAccount.IsValid())
	{
		return UserAccount->GetDisplayName();
	}
	return FString();
}

FString FOnlineIdentityUltra::GetAuthToken(int32 LocalUserNum) const
{
	FUniqueNetIdPtr UniquePlayerId = GetUniquePlayerId(LocalUserNum);
	if (UniquePlayerId.IsValid())
	{
		TSharedPtr<FUserOnlineAccount> UserAccount = GetUserAccount(*UniquePlayerId);
		if (UserAccount.IsValid())
		{
			return UserAccount->GetAccessToken();
		}
	}
	return FString();
}

void FOnlineIdentityUltra::RevokeAuthToken(const FUniqueNetId& LocalUserId, const FOnRevokeAuthTokenCompleteDelegate& Delegate)
{
	Delegate.ExecuteIfBound(LocalUserId, FOnlineError(EOnlineErrorResult::NotImplemented));
}

void FOnlineIdentityUltra::GetUserPrivilege(const FUniqueNetId& LocalUserId, EUserPrivileges::Type Privilege, const FOnGetUserPrivilegeCompleteDelegate& Delegate)
{
	if (GetLoginStatus(LocalUserId) == ELoginStatus::LoggedIn)
	{
		Delegate.ExecuteIfBound(LocalUserId, Privilege, (uint32)EPrivilegeResults::NoFailures);
	}
	else
	{
		Delegate.ExecuteIfBound(LocalUserId, Privilege, (uint32)EPrivilegeResults::UserNotLoggedIn);
	}
}

FPlatformUserId FOnlineIdentityUltra::GetPlatformUserIdFromUniqueNetId(const FUniqueNetId& UniqueNetId) const
{
	unimplemented();
	return FPlatformUserId();
}

FString FOnlineIdentityUltra::GetAuthType() const
{
	FString AuthType;
	unimplemented();
	return AuthType;
}

void FOnlineIdentityUltra::Tick(float DeltaTime)
{
	const FDateTime CurrentDate = FDateTime::Now();
	for (int32 Index = 0; Index < MAX_LOCAL_PLAYERS; ++Index)
	{
		FUltraTokenRequest& RequestToken = PendingTokenRequests[Index];
		if (RequestToken.bAuthInProgress && !RequestToken.DeviceCode.IsEmpty())
		{
			if (CurrentDate > RequestToken.ExpiresDate)
			{
				CancelTokenRequest(Index);
			}
			else if (CurrentDate > RequestToken.NextRequestDate)
			{
				RequestToken.NextRequestDate = CurrentDate + FTimespan::FromSeconds(RequestToken.Interval);

				const FString AuthDeviceUrl(FString::Printf(TEXT("%s/token"), *AuthenticationUrl));
				const FString Content(FString::Printf(TEXT("device_code=%s&client_id=%s&scope=openid&grant_type=urn:ietf:params:oauth:grant-type:device_code"), *RequestToken.DeviceCode, *ClientId));

				TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
				HttpRequest->OnProcessRequestComplete().BindRaw(this, &FOnlineIdentityUltra::HandleTokenResponse, Index);
				HttpRequest->SetURL(AuthDeviceUrl);
				HttpRequest->SetContentAsString(Content);
				HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/x-www-form-urlencoded"));
				HttpRequest->SetVerb(TEXT("POST"));

				HttpRequest->ProcessRequest();
			}
		}
	}

	// TODO: Handle refresh tokens
}
