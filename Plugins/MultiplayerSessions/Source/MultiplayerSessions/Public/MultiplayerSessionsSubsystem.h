// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h" // To use IOnlineSessionPtr
#include "FindSessionsCallbackProxy.h" // FBlueprintSessionResult
#include "MultiplayerSessionsSubsystem.generated.h"

//
// Teacher comment:
// Declaring our own custom delegate for the Menu class to bind callbacks to
// 
// My comments:
// 
// "Dynamic" means that it can be serialized, and it can be saved and loaded from a Blueprint event graph.
// In Blueprint, they are called "Event Dispatchers".
// 
// "Multicast" means that multiple classes can bind their functions to this delegate.
// 
// Because it is Dynamic, any function that wants to bind to this delegate must use the UFUNCTION macro.
// If not, that function won't successfully bind to this delegate. If it wasn't Dynamic, you wouldn't need
// UFUNCTION macro.
//
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnCreateSessionComplete, bool, bWasSuccessful);

//
// This delegate cannot use DYNAMIC because FOnlineSessionSearchResult is not a USTRUCT. Dynamic delegates
// are compatible with Blueprints, and blueprints can only receive UFUNCTION, and not native C++ structs.
// If we want for this delegate to be compatible with Blueprints, we need to create a USTRUCT that encompasses
// the FOnlineSessionSearchResult
//
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnFindSessionComplete, const TArray<FOnlineSessionSearchResult>& SearchResults, bool bWasSuccessful); // I can change to FBlueprintSessionResult (must include "FindSessionsCallbackProxy.h")

//
// This delegate cannot use DYNAMIC because EOnJoinSessionCompleteResult::Type is not compatible with Blueprints,
// because EOnJoinSessionCompleteResult::Type is not a UENUM. Remember: DYNAMIC only works with U macros to maintain
// compatibility with blueprints.
//
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnJoinSessionComplete, const FString& Address, EOnJoinSessionCompleteResult::Type Result);

//
// Callback of DestroySession.
// Note: bool is a value type, which is compatible with Blueprints, so we can use DYNAMIC.
//
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnDestroySessionComplete, bool, bWasSuccessful);

//
// Callback of StartSession.
// Note: bool is a value type, which is compatible with Blueprints, so we can use DYNAMIC.
//
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnStartSessionComplete, bool, bWasSuccessful);

/**
 *
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UMultiplayerSessionsSubsystem();

	// 
	// Teacher comment:
	// To handle session functionality. The Menu class will call these.
	//
	UFUNCTION(BlueprintCallable)
	void CreateSession(const int32 NumPublicConnections, const FString& MatchType);
	UFUNCTION(BlueprintCallable)
	void FindSessions(const int32 MaxSearchResults);
	void JoinSession(const FOnlineSessionSearchResult& SessionResult);
	UFUNCTION(BlueprintCallable)
	void DestroySession();
	UFUNCTION(BlueprintCallable)
	void StartSession();

	//
	// Teacher comment:
	// Our own custom delegates for the Menu class to bind callbacks to
	// 
	// My comment: Dynamic Multicast Delegate
	//
	FMultiplayerOnCreateSessionComplete MultiplayerOnCreateSessionComplete;
	// Not Dynamic Multicast Delegate
	FMultiplayerOnFindSessionComplete MultiplayerOnFindSessionComplete;
	// Not Dynamic Multicast Delegate
	FMultiplayerOnJoinSessionComplete MultiplayerOnJoinSessionComplete;
	// Dynamic Multicast Delegate
	FMultiplayerOnDestroySessionComplete MultiplayerOnDestroySessionComplete;
	// Dynamic Multicast Delegate
	FMultiplayerOnStartSessionComplete MultiplayerOnStartSessionComplete;

	// Extra: const key of MatchType
	// const FName MatchTypeKey = FName("MatchType");
	// 
	// Some info about list initialization:
	// https://stackoverflow.com/questions/47861532/use-curly-braces-or-equal-sign-when-initialize-a-variable
	// https://stackoverflow.com/questions/18222926/what-are-the-advantages-of-list-initialization-using-curly-braces
	// https://herbsutter.com/2013/08/12/gotw-94-solution-aaa-style-almost-always-auto/
	const FName MatchTypeKey{ TEXT("MatchType") };

protected:

	//
	// Teacher comment:
	// Internal callbacks for the delegates we'll add to the Online Session Interface delegate list.
	// This does not need to be called outside this class.
	//
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);

private:
	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;

	// Extra: Helper function to define if the creation of a session
	// should be LAN or online.
	static bool ShouldBeLanMatch();

	// Extra: Get Preferred Unique Net User Id
	const FUniqueNetId& GetPreferredUniqueNetId() const;

	// Extra: Gets current session, if there's one
	FNamedOnlineSession* GetCurrentGameSession() const;
	
	//
	// Teacher comment:
	// To add to the Online Session Interface delegate list.
	// We'll bind our MultiplayerSessionSubsystem internal callback to these.
	//
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FDelegateHandle FindSessionCompleteDelegateHandle;
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	FOnStartSessionCompleteDelegate StartSessionCompleteDelegate;
	FDelegateHandle StartSessionCompleteDelegateHandle;

	bool bCreateSessionOnDestroy;
	int32 LastNumPublicConnections;
	FString LastMatchType;
};
