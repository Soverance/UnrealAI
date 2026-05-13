// Copyright Natali Caggiano. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

class FGameFrameworkEditor
{
public:
	// ===== Game Mode Operations =====

	static TSharedPtr<FJsonObject> InspectGameMode();

	static TSharedPtr<FJsonObject> SetGameModeDefaults(const FString& DefaultPawnClass, const FString& PlayerControllerClass, const FString& HUDClass);

	// ===== Player Controller Operations =====

	static TSharedPtr<FJsonObject> SetInputMode(const FString& InputMode);

	static TSharedPtr<FJsonObject> ShowWidget(const FString& WidgetClassPath, int32 ZOrder = 0);

	static TSharedPtr<FJsonObject> RemoveWidget(const FString& WidgetClassPath);

	static TSharedPtr<FJsonObject> SetPause(bool bPaused);

	static TSharedPtr<FJsonObject> SetMouseCursor(bool bShowCursor);

	// ===== DataTable Operations =====

	static TSharedPtr<FJsonObject> ListDataTables(const FString& FolderPath);

	static TSharedPtr<FJsonObject> InspectDataTable(const FString& TablePath);

	static TSharedPtr<FJsonObject> GetRow(const FString& TablePath, const FString& RowName);

	static TSharedPtr<FJsonObject> SetRow(const FString& TablePath, const FString& RowName, const TSharedPtr<FJsonObject>& RowData);

	static TSharedPtr<FJsonObject> DeleteRow(const FString& TablePath, const FString& RowName);

private:
	static TSharedPtr<FJsonObject> SuccessResult(const FString& Message);
	static TSharedPtr<FJsonObject> ErrorResult(const FString& Message);

	static TSharedPtr<FJsonValue> PropertyToJson(FProperty* Property, const void* ValuePtr);
	static bool JsonToProperty(FProperty* Property, void* ValuePtr, const TSharedPtr<FJsonValue>& JsonValue, FString& OutError);

	static UDataTable* LoadDataTable(const FString& TablePath, FString& OutError);
	static TSharedPtr<FJsonObject> SerializeRow(const UScriptStruct* RowStruct, const void* RowData);
};
