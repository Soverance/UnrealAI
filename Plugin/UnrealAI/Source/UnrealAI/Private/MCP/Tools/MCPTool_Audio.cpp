// Copyright Natali Caggiano. All Rights Reserved.

#include "MCPTool_Audio.h"
#include "AudioEditor.h"

static FMCPToolResult AudioJsonToToolResult(const TSharedPtr<FJsonObject>& Result, const FString& SuccessContext)
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

// ============================================================================
// Main Dispatch
// ============================================================================

FMCPToolResult FMCPTool_Audio::Execute(const TSharedRef<FJsonObject>& Params)
{
	FString Operation;
	TOptional<FMCPToolResult> Error;
	if (!ExtractRequiredString(Params, TEXT("operation"), Operation, Error))
	{
		return Error.GetValue();
	}

	Operation = Operation.ToLower();

	// Read operations
	if (Operation == TEXT("inspect"))
	{
		return HandleInspect(Params);
	}
	else if (Operation == TEXT("list_sounds"))
	{
		return HandleListSounds(Params);
	}
	else if (Operation == TEXT("get_audio_components"))
	{
		return HandleGetAudioComponents(Params);
	}
	// Write operations
	else if (Operation == TEXT("add_audio_component"))
	{
		return HandleAddAudioComponent(Params);
	}
	else if (Operation == TEXT("set_audio_properties"))
	{
		return HandleSetAudioProperties(Params);
	}
	else if (Operation == TEXT("create_sound_cue"))
	{
		return HandleCreateSoundCue(Params);
	}
	else if (Operation == TEXT("create_attenuation"))
	{
		return HandleCreateAttenuation(Params);
	}
	else if (Operation == TEXT("spawn_ambient_sound"))
	{
		return HandleSpawnAmbientSound(Params);
	}

	return FMCPToolResult::Error(FString::Printf(
		TEXT("Unknown operation: '%s'. Valid: inspect, list_sounds, get_audio_components, add_audio_component, set_audio_properties, create_sound_cue, create_attenuation, spawn_ambient_sound"),
		*Operation));
}

// ============================================================================
// Read Handlers
// ============================================================================

FMCPToolResult FMCPTool_Audio::HandleInspect(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath;
	TOptional<FMCPToolResult> Error;
	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}

	TSharedPtr<FJsonObject> Result = FAudioEditor::InspectSound(AssetPath);
	return AudioJsonToToolResult(Result, TEXT("Sound inspected"));
}

FMCPToolResult FMCPTool_Audio::HandleListSounds(const TSharedRef<FJsonObject>& Params)
{
	FString FolderPath;
	TOptional<FMCPToolResult> Error;
	if (!ExtractRequiredString(Params, TEXT("folder_path"), FolderPath, Error))
	{
		return Error.GetValue();
	}

	FString ClassFilter = ExtractOptionalString(Params, TEXT("class_filter"));
	bool bRecursive = ExtractOptionalBool(Params, TEXT("recursive"), false);

	TSharedPtr<FJsonObject> Result = FAudioEditor::ListSounds(FolderPath, ClassFilter, bRecursive);
	return AudioJsonToToolResult(Result, TEXT("Sounds listed"));
}

FMCPToolResult FMCPTool_Audio::HandleGetAudioComponents(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath;
	TOptional<FMCPToolResult> Error;
	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}

	TSharedPtr<FJsonObject> Result = FAudioEditor::GetAudioComponents(AssetPath);
	return AudioJsonToToolResult(Result, TEXT("Audio components retrieved"));
}

// ============================================================================
// Write Handlers (stubs — call FAudioEditor stubs)
// ============================================================================

FMCPToolResult FMCPTool_Audio::HandleAddAudioComponent(const TSharedRef<FJsonObject>& Params)
{
	FString BlueprintPath;
	TOptional<FMCPToolResult> ExtractError;

	if (!ExtractRequiredString(Params, TEXT("blueprint_path"), BlueprintPath, ExtractError))
	{
		return ExtractError.GetValue();
	}

	FString ComponentName = ExtractOptionalString(Params, TEXT("component_name"), TEXT("AudioComponent"));
	FString SoundAssetPath = ExtractOptionalString(Params, TEXT("sound_asset_path"));

	TSharedPtr<FJsonObject> Result = FAudioEditor::AddAudioComponent(BlueprintPath, ComponentName, SoundAssetPath);
	return AudioJsonToToolResult(Result, TEXT("Audio component added"));
}

FMCPToolResult FMCPTool_Audio::HandleSetAudioProperties(const TSharedRef<FJsonObject>& Params)
{
	FString TargetPath;
	TOptional<FMCPToolResult> ExtractError;

	if (!ExtractRequiredString(Params, TEXT("target_path"), TargetPath, ExtractError))
	{
		return ExtractError.GetValue();
	}

	FString ComponentName = ExtractOptionalString(Params, TEXT("component_name"));

	TSharedPtr<FJsonObject> Properties = MakeShared<FJsonObject>();
	const TSharedPtr<FJsonObject>* PropertiesObj = nullptr;
	if (Params->TryGetObjectField(TEXT("properties"), PropertiesObj) && PropertiesObj && *PropertiesObj)
	{
		Properties->Values = (*PropertiesObj)->Values;
	}

	double NumVal;
	if (Params->TryGetNumberField(TEXT("volume"), NumVal))
	{
		Properties->SetNumberField(TEXT("volume"), NumVal);
	}
	if (Params->TryGetNumberField(TEXT("pitch"), NumVal))
	{
		Properties->SetNumberField(TEXT("pitch"), NumVal);
	}
	bool BoolVal;
	if (Params->TryGetBoolField(TEXT("auto_activate"), BoolVal))
	{
		Properties->SetBoolField(TEXT("auto_activate"), BoolVal);
	}
	if (Params->TryGetBoolField(TEXT("is_spatialized"), BoolVal))
	{
		Properties->SetBoolField(TEXT("is_spatialized"), BoolVal);
	}
	FString StrVal;
	if (Params->TryGetStringField(TEXT("sound"), StrVal))
	{
		Properties->SetStringField(TEXT("sound"), StrVal);
	}
	if (Params->TryGetStringField(TEXT("attenuation_path"), StrVal))
	{
		Properties->SetStringField(TEXT("attenuation_path"), StrVal);
	}

	TSharedPtr<FJsonObject> Result = FAudioEditor::SetAudioProperties(TargetPath, ComponentName, Properties);
	return AudioJsonToToolResult(Result, TEXT("Audio properties set"));
}

FMCPToolResult FMCPTool_Audio::HandleCreateSoundCue(const TSharedRef<FJsonObject>& Params)
{
	FString PackagePath, Name, SoundWavePath;
	TOptional<FMCPToolResult> Error;

	if (!ExtractRequiredString(Params, TEXT("package_path"), PackagePath, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("name"), Name, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("sound_wave_path"), SoundWavePath, Error))
	{
		return Error.GetValue();
	}

	TSharedPtr<FJsonObject> Result = FAudioEditor::CreateSoundCue(PackagePath, Name, SoundWavePath);
	return AudioJsonToToolResult(Result, TEXT("SoundCue created"));
}

FMCPToolResult FMCPTool_Audio::HandleCreateAttenuation(const TSharedRef<FJsonObject>& Params)
{
	FString PackagePath, Name;
	TOptional<FMCPToolResult> Error;

	if (!ExtractRequiredString(Params, TEXT("package_path"), PackagePath, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("name"), Name, Error))
	{
		return Error.GetValue();
	}

	TSharedPtr<FJsonObject> Settings = MakeShared<FJsonObject>();

	const TSharedPtr<FJsonObject>* SettingsObj = nullptr;
	if (Params->TryGetObjectField(TEXT("settings"), SettingsObj) && SettingsObj && *SettingsObj)
	{
		Settings->Values = (*SettingsObj)->Values;
	}

	FString StrVal;
	double NumVal;
	bool BoolVal;

	if (Params->TryGetStringField(TEXT("attenuation_shape"), StrVal))
	{
		Settings->SetStringField(TEXT("attenuation_shape"), StrVal);
	}
	if (Params->TryGetNumberField(TEXT("inner_radius"), NumVal))
	{
		Settings->SetNumberField(TEXT("inner_radius"), NumVal);
	}
	if (Params->TryGetNumberField(TEXT("outer_radius"), NumVal))
	{
		Settings->SetNumberField(TEXT("outer_radius"), NumVal);
	}
	if (Params->TryGetNumberField(TEXT("falloff_distance"), NumVal))
	{
		Settings->SetNumberField(TEXT("falloff_distance"), NumVal);
	}
	if (Params->TryGetStringField(TEXT("distance_algorithm"), StrVal))
	{
		Settings->SetStringField(TEXT("distance_algorithm"), StrVal);
	}
	if (Params->TryGetNumberField(TEXT("db_attenuation_at_max"), NumVal))
	{
		Settings->SetNumberField(TEXT("db_attenuation_at_max"), NumVal);
	}
	if (Params->TryGetBoolField(TEXT("is_spatialized"), BoolVal))
	{
		Settings->SetBoolField(TEXT("is_spatialized"), BoolVal);
	}
	if (Params->TryGetStringField(TEXT("spatialization_algorithm"), StrVal))
	{
		Settings->SetStringField(TEXT("spatialization_algorithm"), StrVal);
	}

	TSharedPtr<FJsonObject> Result = FAudioEditor::CreateAttenuation(PackagePath, Name, Settings);
	return AudioJsonToToolResult(Result, TEXT("SoundAttenuation created"));
}

FMCPToolResult FMCPTool_Audio::HandleSpawnAmbientSound(const TSharedRef<FJsonObject>& Params)
{
	FString SoundAssetPath;
	TOptional<FMCPToolResult> Error;

	if (!ExtractRequiredString(Params, TEXT("sound_asset_path"), SoundAssetPath, Error))
	{
		return Error.GetValue();
	}

	FVector Location = ExtractVectorParam(Params, TEXT("location"), FVector::ZeroVector);
	FString Label = ExtractOptionalString(Params, TEXT("label"));

	UWorld* World = nullptr;
	TOptional<FMCPToolResult> WorldError = ValidateEditorContext(World);
	if (WorldError.IsSet())
	{
		return WorldError.GetValue();
	}

	TSharedPtr<FJsonObject> Result = FAudioEditor::SpawnAmbientSound(World, SoundAssetPath, Location, Label);
	return AudioJsonToToolResult(Result, TEXT("AmbientSound spawned"));
}
