// Copyright Natali Caggiano. All Rights Reserved.

#include "MCPTool_GameFramework.h"
#include "GameFrameworkEditor.h"

static FMCPToolResult GameFrameworkJsonToToolResult(const TSharedPtr<FJsonObject>& Result, const FString& SuccessContext)
{
	bool bSuccess = false;
	Result->TryGetBoolField(TEXT("success"), bSuccess);

	if (bSuccess)
	{
		FString Message;
		Result->TryGetStringField(TEXT("message"), Message);
		if (Message.IsEmpty())
		{
			Message = SuccessContext;
		}
		return FMCPToolResult::Success(Message, Result);
	}
	else
	{
		FString ErrorMsg;
		Result->TryGetStringField(TEXT("error"), ErrorMsg);
		return FMCPToolResult::Error(ErrorMsg.IsEmpty() ? TEXT("Unknown error") : ErrorMsg);
	}
}

FMCPToolResult FMCPTool_GameFramework::Execute(const TSharedRef<FJsonObject>& Params)
{
	FString Operation;
	TOptional<FMCPToolResult> Error;
	if (!ExtractRequiredString(Params, TEXT("operation"), Operation, Error))
	{
		return Error.GetValue();
	}

	Operation = Operation.ToLower();

	if (Operation == TEXT("inspect") || Operation == TEXT("info") || Operation == TEXT("get_info"))
	{
		Operation = TEXT("inspect_game_mode");
	}
	else if (Operation == TEXT("set_defaults"))
	{
		Operation = TEXT("set_game_mode_defaults");
	}
	else if (Operation == TEXT("add_widget"))
	{
		Operation = TEXT("show_widget");
	}
	else if (Operation == TEXT("remove_all_widgets"))
	{
		Operation = TEXT("remove_widget");
	}
	else if (Operation == TEXT("pause") || Operation == TEXT("unpause"))
	{
		Operation = TEXT("set_pause");
	}
	else if (Operation == TEXT("cursor") || Operation == TEXT("mouse_cursor"))
	{
		Operation = TEXT("set_mouse_cursor");
	}
	else if (Operation == TEXT("list") || Operation == TEXT("list_tables"))
	{
		Operation = TEXT("list_data_tables");
	}
	else if (Operation == TEXT("inspect_table") || Operation == TEXT("table_info"))
	{
		Operation = TEXT("inspect_data_table");
	}
	else if (Operation == TEXT("get") || Operation == TEXT("get_row_data"))
	{
		Operation = TEXT("get_row");
	}

	if (Operation == TEXT("inspect_game_mode"))
	{
		return HandleInspectGameMode(Params);
	}
	else if (Operation == TEXT("set_game_mode_defaults"))
	{
		return HandleSetGameModeDefaults(Params);
	}
	else if (Operation == TEXT("set_input_mode"))
	{
		return HandleSetInputMode(Params);
	}
	else if (Operation == TEXT("show_widget"))
	{
		return HandleShowWidget(Params);
	}
	else if (Operation == TEXT("remove_widget"))
	{
		return HandleRemoveWidget(Params);
	}
	else if (Operation == TEXT("set_pause"))
	{
		return HandleSetPause(Params);
	}
	else if (Operation == TEXT("set_mouse_cursor"))
	{
		return HandleSetMouseCursor(Params);
	}
	else if (Operation == TEXT("inspect_data_table"))
	{
		return HandleInspectDataTable(Params);
	}
	else if (Operation == TEXT("list_data_tables"))
	{
		return HandleListDataTables(Params);
	}
	else if (Operation == TEXT("get_row"))
	{
		return HandleGetRow(Params);
	}
	else if (Operation == TEXT("set_row"))
	{
		return HandleSetRow(Params);
	}
	else if (Operation == TEXT("delete_row"))
	{
		return HandleDeleteRow(Params);
	}

	return FMCPToolResult::Error(FString::Printf(
		TEXT("Unknown operation: '%s'. Valid: inspect_game_mode, set_game_mode_defaults, set_input_mode, show_widget, remove_widget, set_pause, set_mouse_cursor, inspect_data_table, list_data_tables, get_row, set_row, delete_row"),
		*Operation));
}

FMCPToolResult FMCPTool_GameFramework::HandleInspectGameMode(const TSharedRef<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Result = FGameFrameworkEditor::InspectGameMode();
	return GameFrameworkJsonToToolResult(Result, TEXT("Game mode inspected"));
}

FMCPToolResult FMCPTool_GameFramework::HandleSetGameModeDefaults(const TSharedRef<FJsonObject>& Params)
{
	FString DefaultPawnClass = ExtractOptionalString(Params, TEXT("default_pawn_class"));
	FString PlayerControllerClass = ExtractOptionalString(Params, TEXT("player_controller_class"));
	FString HUDClass = ExtractOptionalString(Params, TEXT("hud_class"));

	TSharedPtr<FJsonObject> Result = FGameFrameworkEditor::SetGameModeDefaults(DefaultPawnClass, PlayerControllerClass, HUDClass);
	return GameFrameworkJsonToToolResult(Result, TEXT("Game mode defaults updated"));
}

FMCPToolResult FMCPTool_GameFramework::HandleSetInputMode(const TSharedRef<FJsonObject>& Params)
{
	FString InputMode;
	TOptional<FMCPToolResult> Error;
	if (!ExtractRequiredString(Params, TEXT("input_mode"), InputMode, Error))
	{
		return Error.GetValue();
	}

	InputMode = InputMode.ToLower();
	TSharedPtr<FJsonObject> Result = FGameFrameworkEditor::SetInputMode(InputMode);
	return GameFrameworkJsonToToolResult(Result, TEXT("Input mode set"));
}

FMCPToolResult FMCPTool_GameFramework::HandleShowWidget(const TSharedRef<FJsonObject>& Params)
{
	FString WidgetClassPath;
	TOptional<FMCPToolResult> Error;
	if (!ExtractRequiredString(Params, TEXT("widget_class"), WidgetClassPath, Error))
	{
		return Error.GetValue();
	}

	int32 ZOrder = ExtractOptionalNumber<int32>(Params, TEXT("z_order"), 0);

	TSharedPtr<FJsonObject> Result = FGameFrameworkEditor::ShowWidget(WidgetClassPath, ZOrder);
	return GameFrameworkJsonToToolResult(Result, TEXT("Widget shown"));
}

FMCPToolResult FMCPTool_GameFramework::HandleRemoveWidget(const TSharedRef<FJsonObject>& Params)
{
	FString WidgetClassPath;
	TOptional<FMCPToolResult> Error;
	if (!ExtractRequiredString(Params, TEXT("widget_class"), WidgetClassPath, Error))
	{
		return Error.GetValue();
	}

	TSharedPtr<FJsonObject> Result = FGameFrameworkEditor::RemoveWidget(WidgetClassPath);
	return GameFrameworkJsonToToolResult(Result, TEXT("Widgets removed"));
}

FMCPToolResult FMCPTool_GameFramework::HandleSetPause(const TSharedRef<FJsonObject>& Params)
{
	bool bPaused = ExtractOptionalBool(Params, TEXT("paused"), true);

	TSharedPtr<FJsonObject> Result = FGameFrameworkEditor::SetPause(bPaused);
	return GameFrameworkJsonToToolResult(Result, TEXT("Pause state updated"));
}

FMCPToolResult FMCPTool_GameFramework::HandleSetMouseCursor(const TSharedRef<FJsonObject>& Params)
{
	bool bShowCursor = ExtractOptionalBool(Params, TEXT("show_cursor"), true);

	TSharedPtr<FJsonObject> Result = FGameFrameworkEditor::SetMouseCursor(bShowCursor);
	return GameFrameworkJsonToToolResult(Result, TEXT("Mouse cursor updated"));
}

FMCPToolResult FMCPTool_GameFramework::HandleListDataTables(const TSharedRef<FJsonObject>& Params)
{
	FString FolderPath = ExtractOptionalString(Params, TEXT("folder_path"));
	if (FolderPath.IsEmpty())
	{
		FolderPath = TEXT("/Game");
	}

	TSharedPtr<FJsonObject> Result = FGameFrameworkEditor::ListDataTables(FolderPath);
	return GameFrameworkJsonToToolResult(Result, TEXT("DataTables listed"));
}

FMCPToolResult FMCPTool_GameFramework::HandleInspectDataTable(const TSharedRef<FJsonObject>& Params)
{
	FString TablePath;
	TOptional<FMCPToolResult> Error;
	if (!ExtractRequiredString(Params, TEXT("table_path"), TablePath, Error))
	{
		return Error.GetValue();
	}

	TSharedPtr<FJsonObject> Result = FGameFrameworkEditor::InspectDataTable(TablePath);
	return GameFrameworkJsonToToolResult(Result, TEXT("DataTable inspected"));
}

FMCPToolResult FMCPTool_GameFramework::HandleGetRow(const TSharedRef<FJsonObject>& Params)
{
	FString TablePath;
	TOptional<FMCPToolResult> Error;
	if (!ExtractRequiredString(Params, TEXT("table_path"), TablePath, Error))
	{
		return Error.GetValue();
	}

	FString RowName;
	if (!ExtractRequiredString(Params, TEXT("row_name"), RowName, Error))
	{
		return Error.GetValue();
	}

	TSharedPtr<FJsonObject> Result = FGameFrameworkEditor::GetRow(TablePath, RowName);
	return GameFrameworkJsonToToolResult(Result, TEXT("Row retrieved"));
}

FMCPToolResult FMCPTool_GameFramework::HandleSetRow(const TSharedRef<FJsonObject>& Params)
{
	FString TablePath;
	TOptional<FMCPToolResult> Error;
	if (!ExtractRequiredString(Params, TEXT("table_path"), TablePath, Error))
	{
		return Error.GetValue();
	}

	FString RowName;
	if (!ExtractRequiredString(Params, TEXT("row_name"), RowName, Error))
	{
		return Error.GetValue();
	}

	const TSharedPtr<FJsonObject>* RowDataPtr = nullptr;
	if (!Params->TryGetObjectField(TEXT("row_data"), RowDataPtr) || !RowDataPtr || !(*RowDataPtr).IsValid())
	{
		return FMCPToolResult::Error(TEXT("Missing required parameter: 'row_data' (must be a JSON object with column:value pairs)"));
	}

	TSharedPtr<FJsonObject> Result = FGameFrameworkEditor::SetRow(TablePath, RowName, *RowDataPtr);
	return GameFrameworkJsonToToolResult(Result, TEXT("Row updated"));
}

FMCPToolResult FMCPTool_GameFramework::HandleDeleteRow(const TSharedRef<FJsonObject>& Params)
{
	FString TablePath;
	TOptional<FMCPToolResult> Error;
	if (!ExtractRequiredString(Params, TEXT("table_path"), TablePath, Error))
	{
		return Error.GetValue();
	}

	FString RowName;
	if (!ExtractRequiredString(Params, TEXT("row_name"), RowName, Error))
	{
		return Error.GetValue();
	}

	TSharedPtr<FJsonObject> Result = FGameFrameworkEditor::DeleteRow(TablePath, RowName);
	return GameFrameworkJsonToToolResult(Result, TEXT("Row deleted"));
}
