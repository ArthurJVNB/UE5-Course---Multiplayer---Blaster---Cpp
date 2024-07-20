// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystem.h" // Who needs: IOnlineSubsystem
#include "OnlineSessionSettings.h" // Who needs: FOnlineSessionSettings
#include <Online/OnlineSessionNames.h> // Who needs: Macro SEARCH_PRESENCE (inside FindSessions)

UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem():
	CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete)),
	FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete)),
	JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete)),
	DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete)),
	StartSessionCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete))
{
	if (IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get())
		SessionInterface = OnlineSubsystem->GetSessionInterface();
}

void UMultiplayerSessionsSubsystem::CreateSession(const int32 NumPublicConnections, const FString& MatchType)
{
	if (!SessionInterface.IsValid()) return;

	auto ExistingSession = GetCurrentGameSession();
	if (ExistingSession) // should I use ExistingSession != nullptr?
	{
		GEngine->AddOnScreenDebugMessage(-1,15.f, FColor::Cyan, FString(TEXT("Destroying current session to create a new one")));
		bCreateSessionOnDestroy = true;
		LastNumPublicConnections = NumPublicConnections;
		LastMatchType = MatchType;
		MultiplayerOnCreateSessionComplete.Broadcast(false);
		DestroySession();
		return;
	}

	LastSessionSettings = MakeShareable(new FOnlineSessionSettings());
	LastSessionSettings->bIsLANMatch = ShouldBeLanMatch();
	LastSessionSettings->NumPublicConnections = NumPublicConnections;
	LastSessionSettings->bAllowJoinInProgress = true;
	LastSessionSettings->bAllowJoinViaPresence = true; // Must be set to true, so Steam can use their presence system (if false, the session won't work on Steam)
	LastSessionSettings->bShouldAdvertise = true; // Must be set to true, so Steam can show this session when we search for a session (if false, the session will still be available, but cannot be found in a normal session search... I think)
	LastSessionSettings->bUsesPresence = true; // Must be set to true, so Steam can use the user's presence to be used for the search results (if false, the session won't work on Steam)
	LastSessionSettings->bUseLobbiesIfAvailable = true; // Since UE5 Preview 2, we need to add this line for lobbies to work
	LastSessionSettings->Set(MatchTypeKey, MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing); // If we don't define the InType, it won't advertise this value to the online session search
	// LastSessionSettings->BuildUniqueId = 1; // Problem: For some reason, I can't join session with this thing.

	GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Cyan, FString::Printf(TEXT("MatchTypeKey: %s, MatchTypeValue: %s"), *MatchTypeKey.ToString(), *MatchType));

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	check(LocalPlayer);

	// Binding delegate and storing it, so we can remove it from the delegate list.
	// Teacher comment: Store the delegate in a FDelegateHandle, so we can later remove it from the delegate list.
	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	if (!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings))
	{
		// We clear the delegate by clearing our FDelegateHandle (and not the delegate itself).
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		
		// Teacher comment: Broadcast our own custom delegate
		MultiplayerOnCreateSessionComplete.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::FindSessions(const int32 MaxSearchResults)
{
	check(SessionInterface.IsValid());

	LastSessionSearch = MakeShareable(new FOnlineSessionSearch);
	LastSessionSearch->MaxSearchResults = MaxSearchResults;
	LastSessionSearch->bIsLanQuery = ShouldBeLanMatch();
	LastSessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

	FindSessionCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);
	if (!SessionInterface->FindSessions(GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionCompleteDelegateHandle);
		//MultiplayerOnFindSessionComplete.Broadcast(LastSessionSearch->SearchResults, false); // or pass empty array in the first param: TArray<FOnlineSessionSearchResult>()
		MultiplayerOnFindSessionComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
	}
}

void UMultiplayerSessionsSubsystem::JoinSession(const FOnlineSessionSearchResult& SessionResult)
{
	if (!SessionInterface.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		MultiplayerOnJoinSessionComplete.Broadcast( FString(), EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);
	if (!SessionInterface->JoinSession(GetPreferredUniqueNetId(), NAME_GameSession, SessionResult))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		MultiplayerOnJoinSessionComplete.Broadcast(FString(), EOnJoinSessionCompleteResult::UnknownError);
		checkNoEntry();
	}
}

void UMultiplayerSessionsSubsystem::DestroySession()
{
	if (!SessionInterface.IsValid())
	{
		MultiplayerOnDestroySessionComplete.Broadcast(false);
		return;
	}

	DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);
	if (!SessionInterface->DestroySession(NAME_GameSession))
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		MultiplayerOnDestroySessionComplete.Broadcast(false);
	}
}

void UMultiplayerSessionsSubsystem::StartSession()
{
	check(SessionInterface);
	if (const FNamedOnlineSession* CurrentSession = GetCurrentGameSession())
	{
		const ENetRole RemoteRole = GetWorld()->GetFirstLocalPlayerFromController()->PlayerController->GetLocalRole(); // GetRemoteRole() gave me ROLE_SimulatedProxy
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Silver, UEnum::GetValueAsString(RemoteRole));
		if (RemoteRole != ROLE_Authority)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString(TEXT("User does not have authority to start session")));
			return;
		}
		
		StartSessionCompleteDelegateHandle = SessionInterface->AddOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegate);
		if (!SessionInterface->StartSession(CurrentSession->SessionName))
		{
			SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
			MultiplayerOnStartSessionComplete.Broadcast(false);
		}
		
		// if (CurrentSession->bHosting)
		// {
		// 	GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Cyan, FString(TEXT("StartSession")));
		// 	SessionInterface->StartSession(CurrentSession->SessionName);
		// }
		// else
		// {
		// 	GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Orange, FString(TEXT("Cannot StartSession (you are not the session host)")));
		// }
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Orange, FString(TEXT("Cannot StartSession (no session created or joined)")));
	}
}

void UMultiplayerSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface)
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);

	MultiplayerOnCreateSessionComplete.Broadcast(bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	check(SessionInterface);
	SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionCompleteDelegateHandle);

	// Teacher preferred that this broadcast should broadcast as false if no results were found (even if the search was successful),
	// but I don't want that to happen.
	/*if (LastSessionSearch->SearchResults.Num() <= 0)
	{
		MultiplayerOnFindSessionComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}*/

	MultiplayerOnFindSessionComplete.Broadcast(LastSessionSearch->SearchResults, bWasSuccessful);
}

void UMultiplayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	check(SessionInterface);
	SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);

	FString Address;
	SessionInterface->GetResolvedConnectString(SessionName, Address);
	MultiplayerOnJoinSessionComplete.Broadcast(Address, Result);
}

void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	const FString LogSuccess = FString(TEXT("Session Destroyed"));
	const FString LogFailure = FString(TEXT("Failed to destroy session"));

	GEngine->AddOnScreenDebugMessage(-1, 15.f, bWasSuccessful ? FColor::Cyan : FColor::Red, bWasSuccessful ? LogSuccess : LogFailure);
	
	check(SessionInterface);
	SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	MultiplayerOnDestroySessionComplete.Broadcast(bWasSuccessful);
	
	if (bWasSuccessful && bCreateSessionOnDestroy)
	{
		bCreateSessionOnDestroy = false;
		CreateSession(LastNumPublicConnections, LastMatchType);
	}
}

void UMultiplayerSessionsSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
	const FString LogSuccess = FString(TEXT("Session started successfully!"));
	const FString LogFailure = FString(TEXT("Failed to start session"));
	
	GEngine->AddOnScreenDebugMessage(-1, 15.f, bWasSuccessful ? FColor::Cyan : FColor::Red, bWasSuccessful ? LogSuccess : LogFailure);

	SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
	MultiplayerOnStartSessionComplete.Broadcast(bWasSuccessful);
}

bool UMultiplayerSessionsSubsystem::ShouldBeLanMatch()
{
	return IOnlineSubsystem::Get()->GetSubsystemName() == "NULL" ? true : false;
}

const FUniqueNetId& UMultiplayerSessionsSubsystem::GetPreferredUniqueNetId() const
{
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	check(LocalPlayer);
	return *LocalPlayer->GetPreferredUniqueNetId();
}

FNamedOnlineSession* UMultiplayerSessionsSubsystem::GetCurrentGameSession() const
{
	check(SessionInterface);
	return SessionInterface->GetNamedSession(NAME_GameSession);
}
