// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Menu.generated.h"

/**
 *
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void MenuSetup(int32 NumberOfPublicConnections = 4, FString TypeOfMatch = FString(TEXT("FreeForAll")), FString PathToLobby =
		               TEXT("/Game/Maps/Lobby"));

protected:
	UFUNCTION()
	// It's like the constructor of the UUserWidget
	virtual bool Initialize() override;
	virtual void NativeDestruct() override;

	//
	// Teacher comment:
	// Callbacks for the custom delegates on the MultiplayerSessionsSubsystem
	// 
	// My comment: Dynamic Multicast Delegate
	//
	UFUNCTION()
	void OnCreateSession(bool bWasSuccessful);
	// Multicast Delegate (NON DYNAMIC)
	void OnFindSessions(const TArray<FOnlineSessionSearchResult>& SearchResults, bool bWasSuccessful);
	// Multicast Delegate (NON DYNAMIC)
	void OnJoinSession(const FString& Address, EOnJoinSessionCompleteResult::Type Result) const;
	// Dynamic Multicast Delegate
	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful);
	// Dynamic Multicast Delegate
	UFUNCTION()
	void OnStartSession(bool bWasSuccessful);

private:
	// That's one way to bind a button from Unreal to a UCLASS C++ variable.
	// Obs: Both the button and the variable must be of same name.
	UPROPERTY(meta = (BindWidget))
	class UButton* ButtonHost;

	// That's one way to bind a button from Unreal to a UCLASS C++ variable.
	// Obs: Both the button and the variable must be of same name.
	UPROPERTY(meta = (BindWidget))
	UButton* ButtonJoin;

	// That's one way to bind a button from Unreal to a UCLASS C++ variable.
	// Obs: Both the button and the variable must be of same name.
	UPROPERTY(meta = (BindWidget))
	UButton* ButtonStart;
	
	UFUNCTION()
	void ButtonHostClicked();

	UFUNCTION()
	void ButtonJoinClicked();

	UFUNCTION()
	void ButtonStartClicked();

	void MenuTearDown();

	// Teacher comment: The subsystem designed to handle all online session functionality
	class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;
	
	int32 NumPublicConnections{ 4 };
	FString MatchType{ TEXT("FreeForAll") };
	FString LobbyMap{ TEXT("") };
};
