// Copyright Natali Caggiano. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MCP/MCPToolBase.h"

class FMCPTool_GameFramework : public FMCPToolBase
{
public:
	virtual FMCPToolInfo GetInfo() const override
	{
		FMCPToolInfo Info;
		Info.Name = TEXT("game_framework");
		Info.Description = TEXT(
			"Game framework inspection and configuration tool.\n\n"
			"GAME MODE OPERATIONS:\n"
			"- 'inspect_game_mode': Get current game mode class and its default classes.\n"
			"  Returns: game_mode_class, default_pawn_class, player_controller_class, hud_class, spectator_class.\n\n"
			"- 'set_game_mode_defaults': Set default class overrides on the game mode CDO.\n"
			"  Params: default_pawn_class, player_controller_class, hud_class (all optional, empty = skip)\n\n"
			"PLAYER CONTROLLER OPERATIONS (require PIE):\n"
			"- 'set_input_mode': Set input mode. Params: input_mode ('game_only', 'ui_only', 'game_and_ui')\n"
			"- 'show_widget': Create and add a UUserWidget to viewport. Params: widget_class (BP class path), z_order (optional, default 0)\n"
			"- 'remove_widget': Remove all viewport widgets of a class. Params: widget_class (BP class path)\n"
			"- 'set_pause': Pause/unpause. Params: paused (boolean)\n"
			"- 'set_mouse_cursor': Show/hide mouse cursor. Params: show_cursor (boolean)\n\n"
			"DATATABLE OPERATIONS:\n"
			"- 'list_data_tables': List DataTable assets in a folder. Params: folder_path (optional, default '/Game')\n"
			"- 'inspect_data_table': Get DataTable schema (columns with types) and row names. Params: table_path\n"
			"- 'get_row': Get a single row's data as JSON. Params: table_path, row_name\n"
			"- 'set_row': Set/add a row (creates if new, updates if existing). Params: table_path, row_name, row_data (object with column:value pairs)\n"
			"- 'delete_row': Delete a row. Params: table_path, row_name"
		);
		Info.Parameters = {
			FMCPToolParameter(TEXT("operation"), TEXT("string"), TEXT("Operation: inspect_game_mode, set_game_mode_defaults, set_input_mode, show_widget, remove_widget, set_pause, set_mouse_cursor, inspect_data_table, list_data_tables, get_row, set_row, delete_row"), true),
			FMCPToolParameter(TEXT("default_pawn_class"), TEXT("string"), TEXT("Default pawn class path (for set_game_mode_defaults)")),
			FMCPToolParameter(TEXT("player_controller_class"), TEXT("string"), TEXT("Player controller class path (for set_game_mode_defaults)")),
			FMCPToolParameter(TEXT("hud_class"), TEXT("string"), TEXT("HUD class path (for set_game_mode_defaults)")),
			FMCPToolParameter(TEXT("input_mode"), TEXT("string"), TEXT("Input mode: game_only, ui_only, game_and_ui (for set_input_mode)")),
			FMCPToolParameter(TEXT("widget_class"), TEXT("string"), TEXT("Widget Blueprint class path (for show_widget, remove_widget)")),
			FMCPToolParameter(TEXT("z_order"), TEXT("number"), TEXT("Viewport Z-order for widget (for show_widget, default 0)")),
			FMCPToolParameter(TEXT("paused"), TEXT("boolean"), TEXT("Pause state (for set_pause)")),
			FMCPToolParameter(TEXT("show_cursor"), TEXT("boolean"), TEXT("Mouse cursor visibility (for set_mouse_cursor)")),
			FMCPToolParameter(TEXT("table_path"), TEXT("string"), TEXT("DataTable asset path")),
			FMCPToolParameter(TEXT("row_name"), TEXT("string"), TEXT("Row name (for get_row, set_row, delete_row)")),
			FMCPToolParameter(TEXT("row_data"), TEXT("object"), TEXT("Row data object (for set_row)")),
			FMCPToolParameter(TEXT("folder_path"), TEXT("string"), TEXT("Content folder path (for list_data_tables)"))
		};
		Info.Annotations = FMCPToolAnnotations::Modifying();
		return Info;
	}

	virtual FMCPToolResult Execute(const TSharedRef<FJsonObject>& Params) override;

private:
	FMCPToolResult HandleInspectGameMode(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleSetGameModeDefaults(const TSharedRef<FJsonObject>& Params);

	FMCPToolResult HandleSetInputMode(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleShowWidget(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleRemoveWidget(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleSetPause(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleSetMouseCursor(const TSharedRef<FJsonObject>& Params);

	FMCPToolResult HandleInspectDataTable(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleListDataTables(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleGetRow(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleSetRow(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleDeleteRow(const TSharedRef<FJsonObject>& Params);
};
