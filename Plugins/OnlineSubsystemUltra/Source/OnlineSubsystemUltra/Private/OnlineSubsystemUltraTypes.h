#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystemTypes.h"

#include "OnlineSubsystemUltraPrivate.h"

struct FUltraValidToken
{
	// OAuth fields
	FString AccessToken;
	FString IdToken;
	FString RefreshToken;
	FString TokenType;
	FDateTime ExpiresDate;
	FDateTime RefreshExpiresDate;
};

typedef TSharedPtr<const class FUniqueNetIdUltra> FUniqueNetIdUltraPtr;
typedef TSharedRef<const class FUniqueNetIdUltra> FUniqueNetIdUltraRef;

class FUniqueNetIdUltra :
	public FUniqueNetId
{
public:

	template<typename... TArgs>
	static FUniqueNetIdUltraRef Create(TArgs&&... Args)
	{
		return MakeShareable(new FUniqueNetIdUltra(Forward<TArgs>(Args)...));
	}

	static const FUniqueNetIdUltra& Cast(const FUniqueNetId& NetId)
	{
		return *static_cast<const FUniqueNetIdUltra*>(&NetId);
	}

	FUniqueNetIdUltraRef AsShared() const
	{
		return StaticCastSharedRef<const FUniqueNetIdUltra>(FUniqueNetId::AsShared());
	}

	static const FUniqueNetIdUltraRef& EmptyId()
	{
		static const FUniqueNetIdUltraRef EmptyId(Create());
		return EmptyId;
	}

	virtual FName GetType() const override
	{
		static FName ULTRA_Type(TEXT("ULTRA"));
		return ULTRA_Type;
	}

	virtual const uint8* GetBytes() const override
	{
		return reinterpret_cast<const uint8*>(UserGuid.GetCharArray().GetData());
	}

	virtual int32 GetSize() const override
	{
		const auto& CharArray = UserGuid.GetCharArray();
		return CharArray.GetTypeSize() * CharArray.Num();
	}

	virtual bool IsValid() const override
	{
		return !UserGuid.IsEmpty();
	}

	virtual uint32 GetTypeHash() const override
	{
		return ::GetTypeHash(StaticCast<const void*>(UserGuid.GetCharArray().GetData()));
	}

	virtual FString ToString() const override
	{
		return UserGuid;
	}

	virtual FString ToDebugString() const override
	{
		return UserGuid;
	}

private:

	FString UserGuid;

	FUniqueNetIdUltra() = default;

	explicit FUniqueNetIdUltra(const FString& InUniqueNetId)
		: UserGuid(InUniqueNetId)
	{
	}

	explicit FUniqueNetIdUltra(FString&& InUniqueNetId)
		: UserGuid(MoveTemp(InUniqueNetId))
	{
	}

	explicit FUniqueNetIdUltra(const FUniqueNetId& Src)
		: UserGuid(Src.ToString())
	{
	}
};

class FUserOnlineAccountUltra :
	public FUserOnlineAccount,
	public TSharedFromThis<FUserOnlineAccountUltra>
{
public:

	FUserOnlineAccountUltra(FUniqueNetIdRef InUserId, const FUltraValidToken& InValidToken, const FString& UserName, const FString& UserPreferredName, const FString& Wallet);
	virtual ~FUserOnlineAccountUltra() = default;

	virtual FUniqueNetIdRef GetUserId() const override;
	virtual FString GetRealName() const override;
	virtual FString GetDisplayName(const FString& Platform = FString()) const override;
	virtual bool GetUserAttribute(const FString& AttrName, FString& OutAttrValue) const override;
	virtual bool SetUserAttribute(const FString& AttrName, const FString& AttrValue) override;

	virtual FString GetAccessToken() const override;
	virtual bool GetAuthAttribute(const FString& AttrName, FString& OutAttrValue) const override;

private:

	FUniqueNetIdRef UserId;
	FUltraValidToken ValidToken;
	FString RealName;
	FString DisplayName;

	TMap<FString, FString> UserAttributes;
};
