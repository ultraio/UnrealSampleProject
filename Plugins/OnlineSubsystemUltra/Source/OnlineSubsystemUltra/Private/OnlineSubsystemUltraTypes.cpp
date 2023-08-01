#include "OnlineSubsystemUltraTypes.h"

FUserOnlineAccountUltra::FUserOnlineAccountUltra(FUniqueNetIdRef InUserId, const FUltraValidToken& InValidToken, const FString& UserName, const FString& UserPreferredName, const FString& Wallet)
	: UserId(InUserId)
	, ValidToken(InValidToken)
	, RealName(UserName)
	, DisplayName(UserPreferredName)
{
	UserAttributes.Add(TEXT("wallet"), Wallet);
}

FUniqueNetIdRef FUserOnlineAccountUltra::GetUserId() const 
{ 
	return UserId; 
}

FString FUserOnlineAccountUltra::GetRealName() const 
{ 
	return RealName;
}

FString FUserOnlineAccountUltra::GetDisplayName(const FString& Platform) const
{
	return DisplayName;
}

bool FUserOnlineAccountUltra::GetUserAttribute(const FString& AttrName, FString& OutAttrValue) const
{
	const FString* Attribute = UserAttributes.Find(AttrName);
	if (Attribute)
	{
		OutAttrValue = *Attribute;
		return true;
	}

	return false;
}

bool FUserOnlineAccountUltra::SetUserAttribute(const FString& AttrName, const FString& AttrValue)
{
	unimplemented();
	return false;
}

FString FUserOnlineAccountUltra::GetAccessToken() const
{
	return ValidToken.AccessToken;
}

bool FUserOnlineAccountUltra::GetAuthAttribute(const FString & AttrName, FString & OutAttrValue) const
{
	unimplemented();
	return false;
}
