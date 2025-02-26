// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MCServerInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UMCServerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 *
 */
class IMCServerInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Server Sessions")
	void Host(const FString& ServerName, const FString& Password, bool bIsLocalServer);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Server Sessions")
	void Join(const FString& Address);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Server Sessions")
	void ShowLoadingScreen(float Duration);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Server Sessions")
	void OnPlayerConnected();
};
