// Copyright 2019 Othereum. All Rights Reserved.

#include "Saucewich.h"

#include "GameLiftServerSDK.h"

#include "EngineUtils.h"
#include "Modules/ModuleManager.h"
#include "Http.h"
#include "Json.h"
#include "JsonObjectConverter.h"
#include "HttpManager.h"
#include "GameFramework/InputSettings.h"

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, Saucewich, "Saucewich")

DEFINE_LOG_CATEGORY(LogGameLift)
DEFINE_LOG_CATEGORY(LogSaucewich)

FGameLiftServerSDKModule& USaucewich::GetGameLift()
{
	static auto&& Module = FModuleManager::GetModuleChecked<FGameLiftServerSDKModule>(TEXT("GameLiftServerSDK"));
	return Module;
}

bool USaucewich::CheckInputAction(const FName ActionName, const FKeyEvent& KeyEvent)
{
	const auto PressedKey = KeyEvent.GetKey();
	TArray<FInputActionKeyMapping> Mappings;
	GetDefault<UInputSettings>()->GetActionMappingByName(ActionName, Mappings);

	for (auto&& Mapping : Mappings)
		if (Mapping.Key == PressedKey)
			return true;
	
	return false;
}

void USaucewich::SearchSession(const FSearchSessionResponse& Callback)
{
	Request(TEXT("GET"), TEXT("http://localhost:6974/session"), FOnHttpResponse::CreateLambda(
		[=](const int32 ResponseCode, const FJsonObject& Content)
		{
			if (ResponseCode == 200)
			{
				const auto ServerIP = Content.GetStringField(TEXT("server-ip"));
				const auto PlayerID = Content.GetStringField(TEXT("player-id"));
				if (!PlayerID.IsEmpty() && !ServerIP.IsEmpty())
				{
					(void)Callback.ExecuteIfBound(true, ServerIP, PlayerID);
					return;
				}
			}
			(void)Callback.ExecuteIfBound(false, {}, {});
		}
	));
}

ENameValidity USaucewich::IsValidPlayerName(const FString& PlayerName)
{
	if (PlayerName.Len() < GetPlayerNameMinLen() || PlayerName.Len() > GetPlayerNameMaxLen())
		return ENameValidity::Length;

	for (const auto c : PlayerName)
		if (!FChar::IsIdentifier(c))
			return ENameValidity::Character;

	return ENameValidity::Valid;
}

int32 USaucewich::GetPlayerNameMinLen()
{
	return 2;
}

int32 USaucewich::GetPlayerNameMaxLen()
{
	return 16;
}

static TSharedRef<IHttpRequest> CreateRequest(const FString& Verb, const FString& URL, const FOnHttpResponse& OnResponse)
{
	const auto Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb(Verb);
	Request->SetURL(URL);

	Request->OnProcessRequestComplete().BindLambda(
		[=](const FHttpRequestPtr&, const FHttpResponsePtr& Response, const bool bConnectedSuccessfully)
		{
			if (bConnectedSuccessfully && Response)
			{
				const auto Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
				TSharedPtr<FJsonObject> JsonObject;
				FJsonSerializer::Deserialize(Reader, JsonObject);
				(void)OnResponse.ExecuteIfBound(Response->GetResponseCode(), JsonObject ? *JsonObject : FJsonObject{});
			}
			else
			{
				(void)OnResponse.ExecuteIfBound(0, {});
			}
		}
	);

	return Request;
}

void USaucewich::Request(const FString& Verb, const FString& URL, const FOnHttpResponse& OnResponse)
{
	const auto Request = CreateRequest(Verb, URL, OnResponse);
	if (!Request->ProcessRequest()) OnResponse.ExecuteIfBound(0, {});
}

void USaucewich::Request(const FString& Verb, const FString& URL, const TSharedRef<FJsonObject>& Content, const FOnHttpResponse& OnResponse)
{
	const auto Request = CreateRequest(Verb, URL, OnResponse);
	
	FString Serialized;
	const auto Writer = TJsonWriterFactory<>::Create(&Serialized);
	if (FJsonSerializer::Serialize(Content, Writer))
	{
		Request->SetContentAsString(Serialized);
	}

	if (!Request->ProcessRequest()) OnResponse.ExecuteIfBound(0, {});
}
