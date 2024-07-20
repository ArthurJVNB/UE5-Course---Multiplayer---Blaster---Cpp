// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"

void UMenu::MenuSetup(const int32 NumberOfPublicConnections, FString TypeOfMatch, FString PathToLobby)
{
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);
	//bIsFocusable = true;

	NumPublicConnections = NumberOfPublicConnections;
	MatchType = TypeOfMatch;
	LobbyMap = FString::Printf(TEXT("%s?listen"), *PathToLobby);

	if (const UWorld* World = GetWorld())
	{
		if (APlayerController* PlayerController = World->GetFirstPlayerController())
		{
			FInputModeUIOnly InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputModeData);
		}
	}

	const UGameInstance* GameInstance = GetGameInstance();
	check(GameInstance);
	MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();

	check(MultiplayerSessionsSubsystem);
	MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreateSession);
	MultiplayerSessionsSubsystem->MultiplayerOnFindSessionComplete.AddUObject(this, &ThisClass::OnFindSessions);
	MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::OnJoinSession);
	MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &ThisClass::OnDestroySession);
	MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ThisClass::OnStartSession);
}

// It's like the constructor of the UUserWidget
bool UMenu::Initialize()
{
	if (!Super::Initialize()) return false;

	if (ButtonHost)
		ButtonHost->OnClicked.AddDynamic(this, &ThisClass::ButtonHostClicked);

	if (ButtonJoin)
		ButtonJoin->OnClicked.AddDynamic(this, &ThisClass::ButtonJoinClicked);

	if (ButtonStart)
		ButtonStart->OnClicked.AddDynamic(this, &ThisClass::ButtonStartClicked);
	
	return true;
}

void UMenu::NativeDestruct()
{
	MenuTearDown();
	Super::NativeDestruct();
}

void UMenu::OnCreateSession(const bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, FString(TEXT("Session created successfully!")));
		GetWorld()->ServerTravel(LobbyMap);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString(TEXT("Failed to create session!")));
		ButtonHost->SetIsEnabled(true);
	}
}

void UMenu::OnFindSessions(const TArray<FOnlineSessionSearchResult>& SearchResults, bool bWasSuccessful)
{
	check(MultiplayerSessionsSubsystem);

	// Debug messages
	if (bWasSuccessful) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, FString(TEXT("Completed finding sessions")));
	else GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString(TEXT("Error trying to find sessions")));
	// --------------

	if (bWasSuccessful)
	{
		if (SearchResults.IsEmpty())
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Orange, FString(TEXT("Seach Results was empty")));
			ButtonJoin->SetIsEnabled(true);
			return;
		}

		for (auto& Result : SearchResults)
		{
			FString ResultMatchType;
			Result.Session.SessionSettings.Get(MultiplayerSessionsSubsystem->MatchTypeKey, ResultMatchType);
			if (ResultMatchType == MatchType)
			{
				// FOUNDS SESSION AND TRIES TO JOIN IT
				GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, FString(TEXT("Joining session...")));
				MultiplayerSessionsSubsystem->JoinSession(Result);
				return;
			}
		}

		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Orange, FString(TEXT("Could not find any session")));
	}
	
	ButtonJoin->SetIsEnabled(true);
}

void UMenu::OnJoinSession(const FString& Address, const EOnJoinSessionCompleteResult::Type Result) const
{
	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		check(Address != FString());
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, FString::Printf(TEXT("Success joining session! Address %s"), *Address));
		GetWorld()->GetFirstPlayerController()->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString("Failed to join session!"));
		ButtonJoin->SetIsEnabled(true);
	}
}

void UMenu::OnDestroySession(bool bWasSuccessful)
{
}

void UMenu::OnStartSession(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, FString("Started Session"));
		GetWorld()->ServerTravel("/Game/Maps/Level");
	}
	else
	{
		ButtonStart->SetIsEnabled(true);
	}
}

void UMenu::ButtonHostClicked()
{
	check(MultiplayerSessionsSubsystem);
	if (!MultiplayerSessionsSubsystem) return;
	ButtonHost->SetIsEnabled(false);
	MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
}

void UMenu::ButtonJoinClicked()
{
	check(MultiplayerSessionsSubsystem);
	if (!MultiplayerSessionsSubsystem) return;
	ButtonJoin->SetIsEnabled(false);
	MultiplayerSessionsSubsystem->FindSessions(1000000000);
}

void UMenu::ButtonStartClicked()
{
	check(MultiplayerSessionsSubsystem);
	if (!MultiplayerSessionsSubsystem) return;
	ButtonStart->SetIsEnabled(false);
	MultiplayerSessionsSubsystem->StartSession();
}

void UMenu::MenuTearDown()
{
	//RemoveFromViewport(); // Old method from Viewport. I should use RemoveFromParent() (that's the same thing, because the parent of a UUserWidget is the Viewport).
	RemoveFromParent();

	const UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!PlayerController) return;

	const FInputModeGameOnly InputModeData;
	PlayerController->SetInputMode(InputModeData);
	PlayerController->SetShowMouseCursor(false);
}
