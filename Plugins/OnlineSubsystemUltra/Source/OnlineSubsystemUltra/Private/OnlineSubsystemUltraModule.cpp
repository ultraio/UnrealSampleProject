// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemUltraModule.h"
#include "Modules/ModuleManager.h"
#include "OnlineSubsystemModule.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUltra.h"

#define LOCTEXT_NAMESPACE "FOnlineSubsystemUltraModule"

class FOnlineFactoryUltra : public IOnlineFactory
{
public:

	FOnlineFactoryUltra() = default;
	virtual ~FOnlineFactoryUltra() override = default;

	virtual IOnlineSubsystemPtr CreateSubsystem(FName InstanceName) override
	{
		FOnlineSubsystemUltraPtr UltraSubsystem = MakeShared<FOnlineSubsystemUltra, ESPMode::ThreadSafe>(InstanceName);
		if (UltraSubsystem->Init())
		{
			return UltraSubsystem;
		}

		UltraSubsystem->Shutdown();
		return nullptr;
	}
};

void FOnlineSubsystemUltraModule::StartupModule()
{
	UltraSubsystemFactory = new FOnlineFactoryUltra;

	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.RegisterPlatformService(TEXT("ULTRA"), UltraSubsystemFactory);
}

void FOnlineSubsystemUltraModule::ShutdownModule()
{
	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.UnregisterPlatformService(TEXT("ULTRA"));

	delete UltraSubsystemFactory;
	UltraSubsystemFactory = nullptr;
}

bool FOnlineSubsystemUltraModule::SupportsDynamicReloading()
{
	return false;
}

bool FOnlineSubsystemUltraModule::SupportsAutomaticShutdown()
{
	return false;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FOnlineSubsystemUltraModule, OnlineSubsystemUltra)
