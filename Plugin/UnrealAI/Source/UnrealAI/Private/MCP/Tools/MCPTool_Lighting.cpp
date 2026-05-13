// Copyright Natali Caggiano. All Rights Reserved.

#include "MCPTool_Lighting.h"
#include "LightingEditor.h"
#include "UnrealClaudeModule.h"
#include "Editor.h"
#include "Engine/World.h"

static FMCPToolResult LightingJsonToToolResult(const TSharedPtr<FJsonObject>& ResultJson)
{
	if (!ResultJson.IsValid())
	{
		return FMCPToolResult::Error(TEXT("Internal error: null result from LightingEditor"));
	}

	bool bSuccess = false;
	ResultJson->TryGetBoolField(TEXT("success"), bSuccess);

	if (bSuccess)
	{
		FString Msg;
		ResultJson->TryGetStringField(TEXT("message"), Msg);
		return FMCPToolResult::Success(Msg, ResultJson);
	}
	else
	{
		FString ErrMsg;
		ResultJson->TryGetStringField(TEXT("error"), ErrMsg);
		return FMCPToolResult::Error(ErrMsg);
	}
}

FMCPToolInfo FMCPTool_Lighting::GetInfo() const
{
	FMCPToolInfo Info;
	Info.Name = TEXT("lighting");
	Info.Description = TEXT(
		"Lighting, atmosphere, and post-processing tool.\n\n"
		"READ OPERATIONS:\n"
		"- 'inspect_light': Inspect a light, sky atmosphere, fog, or post-process volume actor.\n"
		"  Params: actor_name (required -- name or label of the actor)\n\n"
		"- 'list_lights': List all lighting-related actors in the current level.\n"
		"  Params: type_filter (optional -- e.g. 'PointLight', 'SpotLight', 'DirectionalLight', 'RectLight',\n"
		"          'SkyLight', 'SkyAtmosphere', 'ExponentialHeightFog', 'PostProcessVolume')\n\n"
		"WRITE OPERATIONS (Light):\n"
		"- 'spawn_light': Spawn a new light actor.\n"
		"  Params: light_type (required -- 'PointLight', 'SpotLight', 'DirectionalLight', 'RectLight'),\n"
		"          location (object {x,y,z}, optional), rotation (object {pitch,yaw,roll}, optional),\n"
		"          label (string, optional), properties (object, optional)\n\n"
		"- 'set_light_properties': Set properties on a standard light (point/spot/directional/rect).\n"
		"  Params: actor_name (required), properties (required -- object with property values)\n\n"
		"- 'set_sky_light': Set properties on a sky light.\n"
		"  Params: actor_name (required), properties (required)\n\n"
		"- 'set_sky_atmosphere': Set properties on a sky atmosphere.\n"
		"  Params: actor_name (required), properties (required)\n\n"
		"- 'set_fog': Set properties on an exponential height fog.\n"
		"  Params: actor_name (required), properties (required)\n\n"
		"POST-PROCESS OPERATIONS:\n"
		"- 'inspect_post_process': Inspect post-process volume settings.\n"
		"  Params: actor_name (required)\n\n"
		"- 'set_post_process': Set post-process volume settings.\n"
		"  Params: actor_name (required), settings (required -- object with setting values)\n\n"
		"- 'spawn_post_process_volume': Spawn a new post-process volume.\n"
		"  Params: location (object {x,y,z}, optional), extent (object {x,y,z}, optional),\n"
		"          infinite (boolean, optional, default false), label (string, optional),\n"
		"          settings (object, optional)"
	);
	Info.Parameters = {
		FMCPToolParameter(TEXT("operation"), TEXT("string"), TEXT("Operation: inspect_light, list_lights, spawn_light, set_light_properties, set_sky_light, set_sky_atmosphere, set_fog, inspect_post_process, set_post_process, spawn_post_process_volume"), true),
		FMCPToolParameter(TEXT("actor_name"), TEXT("string"), TEXT("Actor name or label")),
		FMCPToolParameter(TEXT("type_filter"), TEXT("string"), TEXT("Type filter for list_lights")),
		FMCPToolParameter(TEXT("light_type"), TEXT("string"), TEXT("Light type for spawn_light (PointLight, SpotLight, DirectionalLight, RectLight)")),
		FMCPToolParameter(TEXT("location"), TEXT("object"), TEXT("Location {x,y,z}")),
		FMCPToolParameter(TEXT("rotation"), TEXT("object"), TEXT("Rotation {pitch,yaw,roll}")),
		FMCPToolParameter(TEXT("label"), TEXT("string"), TEXT("Actor label")),
		FMCPToolParameter(TEXT("properties"), TEXT("object"), TEXT("Light properties to set")),
		FMCPToolParameter(TEXT("settings"), TEXT("object"), TEXT("Post-process settings to set")),
		FMCPToolParameter(TEXT("extent"), TEXT("object"), TEXT("Volume extent {x,y,z} for spawn_post_process_volume")),
		FMCPToolParameter(TEXT("infinite"), TEXT("boolean"), TEXT("Infinite extent for post-process volume"), false, TEXT("false"))
	};
	Info.Annotations = FMCPToolAnnotations::Modifying();
	return Info;
}

FMCPToolResult FMCPTool_Lighting::Execute(const TSharedRef<FJsonObject>& Params)
{
	static const TMap<FString, FString> ParamAliases = {
		{TEXT("name"), TEXT("actor_name")},
		{TEXT("actor_label"), TEXT("actor_name")}
	};
	ResolveParamAliases(Params, ParamAliases);

	FString Operation;
	TOptional<FMCPToolResult> ExtractError;
	if (!ExtractRequiredString(Params, TEXT("operation"), Operation, ExtractError))
	{
		return ExtractError.GetValue();
	}

	Operation = Operation.ToLower();

	static const TMap<FString, FString> OpAliases = {
		{TEXT("inspect"), TEXT("inspect_light")},
		{TEXT("info"), TEXT("inspect_light")},
		{TEXT("get_info"), TEXT("inspect_light")},
		{TEXT("list"), TEXT("list_lights")},
		{TEXT("find"), TEXT("list_lights")},
		{TEXT("spawn"), TEXT("spawn_light")},
		{TEXT("set_properties"), TEXT("set_light_properties")},
		{TEXT("set_pp"), TEXT("set_post_process")},
		{TEXT("inspect_pp"), TEXT("inspect_post_process")},
		{TEXT("spawn_pp"), TEXT("spawn_post_process_volume")},
		{TEXT("spawn_ppv"), TEXT("spawn_post_process_volume")}
	};
	Operation = ResolveOperationAlias(Operation, OpAliases);

	if (Operation == TEXT("inspect_light"))
	{
		return HandleInspectLight(Params);
	}
	else if (Operation == TEXT("list_lights"))
	{
		return HandleListLights(Params);
	}
	else if (Operation == TEXT("spawn_light"))
	{
		return HandleSpawnLight(Params);
	}
	else if (Operation == TEXT("set_light_properties"))
	{
		return HandleSetLightProperties(Params);
	}
	else if (Operation == TEXT("set_sky_light"))
	{
		return HandleSetSkyLight(Params);
	}
	else if (Operation == TEXT("set_sky_atmosphere"))
	{
		return HandleSetSkyAtmosphere(Params);
	}
	else if (Operation == TEXT("set_fog"))
	{
		return HandleSetFog(Params);
	}
	else if (Operation == TEXT("inspect_post_process"))
	{
		return HandleInspectPostProcess(Params);
	}
	else if (Operation == TEXT("set_post_process"))
	{
		return HandleSetPostProcess(Params);
	}
	else if (Operation == TEXT("spawn_post_process_volume"))
	{
		return HandleSpawnPostProcessVolume(Params);
	}

	return UnknownOperationError(Operation, {
		TEXT("inspect_light"), TEXT("list_lights"),
		TEXT("spawn_light"), TEXT("set_light_properties"),
		TEXT("set_sky_light"), TEXT("set_sky_atmosphere"), TEXT("set_fog"),
		TEXT("inspect_post_process"), TEXT("set_post_process"), TEXT("spawn_post_process_volume")
	});
}

// ============================================================================
// HandleInspectLight
// ============================================================================

FMCPToolResult FMCPTool_Lighting::HandleInspectLight(const TSharedRef<FJsonObject>& Params)
{
	UWorld* LightWorld = nullptr;
	TOptional<FMCPToolResult> WorldError = ValidateEditorContext(LightWorld);
	if (WorldError.IsSet())
	{
		return WorldError.GetValue();
	}

	FString ActorName;
	TOptional<FMCPToolResult> ExtractError;
	if (!ExtractRequiredString(Params, TEXT("actor_name"), ActorName, ExtractError))
	{
		return ExtractError.GetValue();
	}

	TSharedPtr<FJsonObject> ResultJson = FLightingEditor::InspectLight(LightWorld, ActorName);
	return LightingJsonToToolResult(ResultJson);
}

// ============================================================================
// HandleListLights
// ============================================================================

FMCPToolResult FMCPTool_Lighting::HandleListLights(const TSharedRef<FJsonObject>& Params)
{
	UWorld* LightWorld = nullptr;
	TOptional<FMCPToolResult> WorldError = ValidateEditorContext(LightWorld);
	if (WorldError.IsSet())
	{
		return WorldError.GetValue();
	}

	FString TypeFilterStr = ExtractOptionalString(Params, TEXT("type_filter"));
	TSharedPtr<FJsonObject> ResultJson = FLightingEditor::ListLights(LightWorld, TypeFilterStr);
	return LightingJsonToToolResult(ResultJson);
}

// ============================================================================
// HandleSpawnLight
// ============================================================================

FMCPToolResult FMCPTool_Lighting::HandleSpawnLight(const TSharedRef<FJsonObject>& Params)
{
	UWorld* LightWorld = nullptr;
	TOptional<FMCPToolResult> WorldError = ValidateEditorContext(LightWorld);
	if (WorldError.IsSet())
	{
		return WorldError.GetValue();
	}

	FString LightType;
	TOptional<FMCPToolResult> ExtractError;
	if (!ExtractRequiredString(Params, TEXT("light_type"), LightType, ExtractError))
	{
		return ExtractError.GetValue();
	}

	FVector SpawnLoc = ExtractVectorParam(Params, TEXT("location"), FVector::ZeroVector);
	FRotator SpawnRot = ExtractRotatorParam(Params, TEXT("rotation"), FRotator::ZeroRotator);
	FString LabelStr = ExtractOptionalString(Params, TEXT("label"));

	const TSharedPtr<FJsonObject>* PropsObj = nullptr;
	TSharedPtr<FJsonObject> PropsJson;
	if (Params->TryGetObjectField(TEXT("properties"), PropsObj) && PropsObj)
	{
		PropsJson = *PropsObj;
	}

	TSharedPtr<FJsonObject> ResultJson = FLightingEditor::SpawnLight(LightWorld, LightType, SpawnLoc, SpawnRot, LabelStr, PropsJson);
	return LightingJsonToToolResult(ResultJson);
}

// ============================================================================
// HandleSetLightProperties
// ============================================================================

FMCPToolResult FMCPTool_Lighting::HandleSetLightProperties(const TSharedRef<FJsonObject>& Params)
{
	UWorld* LightWorld = nullptr;
	TOptional<FMCPToolResult> WorldError = ValidateEditorContext(LightWorld);
	if (WorldError.IsSet())
	{
		return WorldError.GetValue();
	}

	FString ActorName;
	TOptional<FMCPToolResult> ExtractError;
	if (!ExtractRequiredString(Params, TEXT("actor_name"), ActorName, ExtractError))
	{
		return ExtractError.GetValue();
	}

	const TSharedPtr<FJsonObject>* PropsObj = nullptr;
	TSharedPtr<FJsonObject> PropsJson;
	if (Params->TryGetObjectField(TEXT("properties"), PropsObj) && PropsObj)
	{
		PropsJson = *PropsObj;
	}

	TSharedPtr<FJsonObject> ResultJson = FLightingEditor::SetLightProperties(LightWorld, ActorName, PropsJson);
	return LightingJsonToToolResult(ResultJson);
}

// ============================================================================
// HandleSetSkyLight
// ============================================================================

FMCPToolResult FMCPTool_Lighting::HandleSetSkyLight(const TSharedRef<FJsonObject>& Params)
{
	UWorld* LightWorld = nullptr;
	TOptional<FMCPToolResult> WorldError = ValidateEditorContext(LightWorld);
	if (WorldError.IsSet())
	{
		return WorldError.GetValue();
	}

	FString ActorName;
	TOptional<FMCPToolResult> ExtractError;
	if (!ExtractRequiredString(Params, TEXT("actor_name"), ActorName, ExtractError))
	{
		return ExtractError.GetValue();
	}

	const TSharedPtr<FJsonObject>* PropsObj = nullptr;
	TSharedPtr<FJsonObject> PropsJson;
	if (Params->TryGetObjectField(TEXT("properties"), PropsObj) && PropsObj)
	{
		PropsJson = *PropsObj;
	}

	TSharedPtr<FJsonObject> ResultJson = FLightingEditor::SetSkyLight(LightWorld, ActorName, PropsJson);
	return LightingJsonToToolResult(ResultJson);
}

// ============================================================================
// HandleSetSkyAtmosphere
// ============================================================================

FMCPToolResult FMCPTool_Lighting::HandleSetSkyAtmosphere(const TSharedRef<FJsonObject>& Params)
{
	UWorld* LightWorld = nullptr;
	TOptional<FMCPToolResult> WorldError = ValidateEditorContext(LightWorld);
	if (WorldError.IsSet())
	{
		return WorldError.GetValue();
	}

	FString ActorName;
	TOptional<FMCPToolResult> ExtractError;
	if (!ExtractRequiredString(Params, TEXT("actor_name"), ActorName, ExtractError))
	{
		return ExtractError.GetValue();
	}

	const TSharedPtr<FJsonObject>* PropsObj = nullptr;
	TSharedPtr<FJsonObject> PropsJson;
	if (Params->TryGetObjectField(TEXT("properties"), PropsObj) && PropsObj)
	{
		PropsJson = *PropsObj;
	}

	TSharedPtr<FJsonObject> ResultJson = FLightingEditor::SetSkyAtmosphere(LightWorld, ActorName, PropsJson);
	return LightingJsonToToolResult(ResultJson);
}

// ============================================================================
// HandleSetFog
// ============================================================================

FMCPToolResult FMCPTool_Lighting::HandleSetFog(const TSharedRef<FJsonObject>& Params)
{
	UWorld* LightWorld = nullptr;
	TOptional<FMCPToolResult> WorldError = ValidateEditorContext(LightWorld);
	if (WorldError.IsSet())
	{
		return WorldError.GetValue();
	}

	FString ActorName;
	TOptional<FMCPToolResult> ExtractError;
	if (!ExtractRequiredString(Params, TEXT("actor_name"), ActorName, ExtractError))
	{
		return ExtractError.GetValue();
	}

	const TSharedPtr<FJsonObject>* PropsObj = nullptr;
	TSharedPtr<FJsonObject> PropsJson;
	if (Params->TryGetObjectField(TEXT("properties"), PropsObj) && PropsObj)
	{
		PropsJson = *PropsObj;
	}

	TSharedPtr<FJsonObject> ResultJson = FLightingEditor::SetFog(LightWorld, ActorName, PropsJson);
	return LightingJsonToToolResult(ResultJson);
}

// ============================================================================
// HandleInspectPostProcess
// ============================================================================

FMCPToolResult FMCPTool_Lighting::HandleInspectPostProcess(const TSharedRef<FJsonObject>& Params)
{
	UWorld* LightWorld = nullptr;
	TOptional<FMCPToolResult> WorldError = ValidateEditorContext(LightWorld);
	if (WorldError.IsSet())
	{
		return WorldError.GetValue();
	}

	FString ActorName;
	TOptional<FMCPToolResult> ExtractError;
	if (!ExtractRequiredString(Params, TEXT("actor_name"), ActorName, ExtractError))
	{
		return ExtractError.GetValue();
	}

	TSharedPtr<FJsonObject> ResultJson = FLightingEditor::InspectPostProcess(LightWorld, ActorName);
	return LightingJsonToToolResult(ResultJson);
}

// ============================================================================
// HandleSetPostProcess
// ============================================================================

FMCPToolResult FMCPTool_Lighting::HandleSetPostProcess(const TSharedRef<FJsonObject>& Params)
{
	UWorld* LightWorld = nullptr;
	TOptional<FMCPToolResult> WorldError = ValidateEditorContext(LightWorld);
	if (WorldError.IsSet())
	{
		return WorldError.GetValue();
	}

	FString ActorName;
	TOptional<FMCPToolResult> ExtractError;
	if (!ExtractRequiredString(Params, TEXT("actor_name"), ActorName, ExtractError))
	{
		return ExtractError.GetValue();
	}

	const TSharedPtr<FJsonObject>* SettingsObj = nullptr;
	TSharedPtr<FJsonObject> SettingsJson;
	if (Params->TryGetObjectField(TEXT("settings"), SettingsObj) && SettingsObj)
	{
		SettingsJson = *SettingsObj;
	}

	TSharedPtr<FJsonObject> ResultJson = FLightingEditor::SetPostProcess(LightWorld, ActorName, SettingsJson);
	return LightingJsonToToolResult(ResultJson);
}

// ============================================================================
// HandleSpawnPostProcessVolume
// ============================================================================

FMCPToolResult FMCPTool_Lighting::HandleSpawnPostProcessVolume(const TSharedRef<FJsonObject>& Params)
{
	UWorld* LightWorld = nullptr;
	TOptional<FMCPToolResult> WorldError = ValidateEditorContext(LightWorld);
	if (WorldError.IsSet())
	{
		return WorldError.GetValue();
	}

	FVector SpawnLoc = ExtractVectorParam(Params, TEXT("location"), FVector::ZeroVector);
	FVector VolumeExtent = ExtractVectorParam(Params, TEXT("extent"), FVector(200.0, 200.0, 200.0));
	bool bInfinite = ExtractOptionalBool(Params, TEXT("infinite"), false);
	FString LabelStr = ExtractOptionalString(Params, TEXT("label"));

	const TSharedPtr<FJsonObject>* SettingsObj = nullptr;
	TSharedPtr<FJsonObject> SettingsJson;
	if (Params->TryGetObjectField(TEXT("settings"), SettingsObj) && SettingsObj)
	{
		SettingsJson = *SettingsObj;
	}

	TSharedPtr<FJsonObject> ResultJson = FLightingEditor::SpawnPostProcessVolume(LightWorld, SpawnLoc, VolumeExtent, bInfinite, LabelStr, SettingsJson);
	return LightingJsonToToolResult(ResultJson);
}
