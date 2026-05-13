// Copyright Natali Caggiano. All Rights Reserved.

#include "MCPTool_Niagara.h"
#include "MCP/MCPParamValidator.h"
#include "UnrealClaudeModule.h"
#include "Editor.h"
#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraTypes.h"
#include "NiagaraRendererProperties.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraMeshRendererProperties.h"
#include "NiagaraRibbonRendererProperties.h"
#include "NiagaraLightRendererProperties.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/World.h"
#include "EngineUtils.h"

FMCPToolInfo FMCPTool_Niagara::GetInfo() const
{
	FMCPToolInfo Info;
	Info.Name = TEXT("niagara");
	Info.Description = TEXT(
		"Niagara particle system tool.\n\n"
		"READ OPERATIONS:\n"
		"- 'inspect': Full read of a Niagara system -- emitters, renderers, user parameters.\n"
		"  Params: asset_path (required)\n\n"
		"- 'list': Find Niagara system assets in a content folder.\n"
		"  Params: folder_path (optional, default /Game/), recursive (optional, default true)\n\n"
		"- 'get_parameters': Get user-exposed parameters for a system.\n"
		"  Params: asset_path (required)\n\n"
		"- 'get_emitter_info': Detailed info for a single emitter by name or index.\n"
		"  Params: asset_path (required), emitter_name (string) or emitter_index (number)\n\n"
		"WRITE OPERATIONS:\n"
		"- 'set_parameter': Set a user-exposed parameter value on a Niagara system asset.\n"
		"  Params: asset_path (required), parameter_name (required), value (required -- number, object {x,y,z}, object {r,g,b,a}, or boolean)\n\n"
		"- 'spawn_system': Spawn a Niagara system in the current level.\n"
		"  Params: asset_path (required), location (object {x,y,z}, optional, default origin), rotation (object {pitch,yaw,roll}, optional),\n"
		"          scale (object {x,y,z}, optional), auto_destroy (boolean, optional, default true), label (string, optional)"
	);
	Info.Parameters = {
		FMCPToolParameter(TEXT("operation"), TEXT("string"), TEXT("Operation: inspect, list, get_parameters, get_emitter_info, set_parameter, spawn_system"), true),
		FMCPToolParameter(TEXT("asset_path"), TEXT("string"), TEXT("Niagara system asset path")),
		FMCPToolParameter(TEXT("folder_path"), TEXT("string"), TEXT("Content folder path (for list)")),
		FMCPToolParameter(TEXT("recursive"), TEXT("boolean"), TEXT("Search subfolders (for list)"), false, TEXT("true")),
		FMCPToolParameter(TEXT("emitter_name"), TEXT("string"), TEXT("Emitter name (for get_emitter_info)")),
		FMCPToolParameter(TEXT("emitter_index"), TEXT("number"), TEXT("Emitter index (for get_emitter_info)")),
		FMCPToolParameter(TEXT("parameter_name"), TEXT("string"), TEXT("Name of the user parameter to set (for set_parameter)")),
		FMCPToolParameter(TEXT("value"), TEXT("string/number/object"), TEXT("Value to set (for set_parameter)")),
		FMCPToolParameter(TEXT("location"), TEXT("object"), TEXT("Spawn location {x,y,z} (for spawn_system)")),
		FMCPToolParameter(TEXT("rotation"), TEXT("object"), TEXT("Spawn rotation {pitch,yaw,roll} (for spawn_system)")),
		FMCPToolParameter(TEXT("scale"), TEXT("object"), TEXT("Spawn scale {x,y,z} (for spawn_system)")),
		FMCPToolParameter(TEXT("auto_destroy"), TEXT("boolean"), TEXT("Whether spawned system auto-destroys (for spawn_system)"), false, TEXT("true")),
		FMCPToolParameter(TEXT("label"), TEXT("string"), TEXT("Actor label for spawned system (for spawn_system)"))
	};
	Info.Annotations = FMCPToolAnnotations::Modifying();
	return Info;
}

FMCPToolResult FMCPTool_Niagara::Execute(const TSharedRef<FJsonObject>& Params)
{
	static const TMap<FString, FString> ParamAliases = {
		{TEXT("path"), TEXT("asset_path")},
		{TEXT("system_path"), TEXT("asset_path")}
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
		{TEXT("info"), TEXT("inspect")},
		{TEXT("get_info"), TEXT("inspect")},
		{TEXT("find"), TEXT("list")},
		{TEXT("search"), TEXT("list")},
		{TEXT("params"), TEXT("get_parameters")},
		{TEXT("get_params"), TEXT("get_parameters")},
		{TEXT("emitter"), TEXT("get_emitter_info")},
		{TEXT("emitter_info"), TEXT("get_emitter_info")},
		{TEXT("set_param"), TEXT("set_parameter")},
		{TEXT("spawn"), TEXT("spawn_system")}
	};
	Operation = ResolveOperationAlias(Operation, OpAliases);

	if (Operation == TEXT("inspect"))
	{
		return HandleInspect(Params);
	}
	else if (Operation == TEXT("list"))
	{
		return HandleList(Params);
	}
	else if (Operation == TEXT("get_parameters"))
	{
		return HandleGetParameters(Params);
	}
	else if (Operation == TEXT("get_emitter_info"))
	{
		return HandleGetEmitterInfo(Params);
	}
	else if (Operation == TEXT("set_parameter"))
	{
		return HandleSetParameter(Params);
	}
	else if (Operation == TEXT("spawn_system"))
	{
		return HandleSpawnSystem(Params);
	}

	return UnknownOperationError(Operation, {TEXT("inspect"), TEXT("list"), TEXT("get_parameters"), TEXT("get_emitter_info"), TEXT("set_parameter"), TEXT("spawn_system")});
}

// ============================================================================
// HandleInspect
// ============================================================================

FMCPToolResult FMCPTool_Niagara::HandleInspect(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath;
	TOptional<FMCPToolResult> ExtractError;
	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, ExtractError))
	{
		return ExtractError.GetValue();
	}

	UNiagaraSystem* NiagaraSystem = LoadObject<UNiagaraSystem>(nullptr, *AssetPath);
	if (!NiagaraSystem)
	{
		return FMCPToolResult::Error(FString::Printf(TEXT("Failed to load Niagara system: %s"), *AssetPath));
	}

	TSharedPtr<FJsonObject> ResultJson = BuildSystemInfoJson(NiagaraSystem);
	return FMCPToolResult::Success(TEXT("Niagara system inspected"), ResultJson);
}

// ============================================================================
// HandleList
// ============================================================================

FMCPToolResult FMCPTool_Niagara::HandleList(const TSharedRef<FJsonObject>& Params)
{
	FString FolderPath = ExtractOptionalString(Params, TEXT("folder_path"), TEXT("/Game/"));
	bool bRecursive = ExtractOptionalBool(Params, TEXT("recursive"), true);

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	FARFilter Filter;
	Filter.ClassPaths.Add(UNiagaraSystem::StaticClass()->GetClassPathName());
	Filter.PackagePaths.Add(FName(*FolderPath));
	Filter.bRecursivePaths = bRecursive;

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssets(Filter, AssetList);

	TArray<TSharedPtr<FJsonValue>> SystemsArray;
	for (const FAssetData& AssetData : AssetList)
	{
		TSharedPtr<FJsonObject> SystemObj = MakeShared<FJsonObject>();
		SystemObj->SetStringField(TEXT("name"), AssetData.AssetName.ToString());
		SystemObj->SetStringField(TEXT("path"), AssetData.GetObjectPathString());
		SystemsArray.Add(MakeShared<FJsonValueObject>(SystemObj));
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
	ResultJson->SetStringField(TEXT("folder"), FolderPath);
	ResultJson->SetNumberField(TEXT("count"), SystemsArray.Num());
	ResultJson->SetArrayField(TEXT("systems"), SystemsArray);

	return FMCPToolResult::Success(FString::Printf(TEXT("Found %d Niagara systems"), SystemsArray.Num()), ResultJson);
}

// ============================================================================
// HandleGetParameters
// ============================================================================

FMCPToolResult FMCPTool_Niagara::HandleGetParameters(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath;
	TOptional<FMCPToolResult> ExtractError;
	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, ExtractError))
	{
		return ExtractError.GetValue();
	}

	UNiagaraSystem* NiagaraSystem = LoadObject<UNiagaraSystem>(nullptr, *AssetPath);
	if (!NiagaraSystem)
	{
		return FMCPToolResult::Error(FString::Printf(TEXT("Failed to load Niagara system: %s"), *AssetPath));
	}

	const FNiagaraUserRedirectionParameterStore& ParamStore = NiagaraSystem->GetExposedParameters();
	TArray<FNiagaraVariable> UserParams;
	ParamStore.GetParameters(UserParams);

	TArray<TSharedPtr<FJsonValue>> ParamsArray;
	for (const FNiagaraVariable& Var : UserParams)
	{
		const uint8* ValueData = ParamStore.GetParameterData(Var);
		TSharedPtr<FJsonObject> ParamObj = BuildParameterInfoJson(Var, ValueData);
		ParamObj->SetNumberField(TEXT("size_bytes"), Var.GetSizeInBytes());
		ParamsArray.Add(MakeShared<FJsonValueObject>(ParamObj));
	}

	TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
	ResultJson->SetStringField(TEXT("system_name"), NiagaraSystem->GetName());
	ResultJson->SetStringField(TEXT("asset_path"), AssetPath);
	ResultJson->SetNumberField(TEXT("parameter_count"), ParamsArray.Num());
	ResultJson->SetArrayField(TEXT("parameters"), ParamsArray);

	return FMCPToolResult::Success(FString::Printf(TEXT("Found %d user parameters"), ParamsArray.Num()), ResultJson);
}

// ============================================================================
// HandleGetEmitterInfo
// ============================================================================

FMCPToolResult FMCPTool_Niagara::HandleGetEmitterInfo(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath;
	TOptional<FMCPToolResult> ExtractError;
	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, ExtractError))
	{
		return ExtractError.GetValue();
	}

	UNiagaraSystem* NiagaraSystem = LoadObject<UNiagaraSystem>(nullptr, *AssetPath);
	if (!NiagaraSystem)
	{
		return FMCPToolResult::Error(FString::Printf(TEXT("Failed to load Niagara system: %s"), *AssetPath));
	}

	FString EmitterName = ExtractOptionalString(Params, TEXT("emitter_name"));
	int32 EmitterIdx = ExtractOptionalNumber<int32>(Params, TEXT("emitter_index"), -1);

	const TArray<FNiagaraEmitterHandle>& EmitterHandles = NiagaraSystem->GetEmitterHandles();

	if (EmitterName.IsEmpty() && EmitterIdx < 0)
	{
		return FMCPToolResult::Error(TEXT("Must specify either emitter_name or emitter_index"));
	}

	const FNiagaraEmitterHandle* FoundHandle = nullptr;

	if (!EmitterName.IsEmpty())
	{
		for (const FNiagaraEmitterHandle& Handle : EmitterHandles)
		{
			if (Handle.GetName().ToString() == EmitterName)
			{
				FoundHandle = &Handle;
				break;
			}
		}
		if (!FoundHandle)
		{
			TArray<FString> AvailableNames;
			for (const FNiagaraEmitterHandle& Handle : EmitterHandles)
			{
				AvailableNames.Add(Handle.GetName().ToString());
			}
			return FMCPToolResult::Error(FString::Printf(TEXT("Emitter '%s' not found. Available: %s"),
				*EmitterName, *FString::Join(AvailableNames, TEXT(", "))));
		}
	}
	else if (EmitterIdx >= 0)
	{
		if (EmitterIdx >= EmitterHandles.Num())
		{
			return FMCPToolResult::Error(FString::Printf(TEXT("Emitter index %d out of range (0-%d)"),
				EmitterIdx, EmitterHandles.Num() - 1));
		}
		FoundHandle = &EmitterHandles[EmitterIdx];
	}

	TSharedPtr<FJsonObject> EmitterJson = BuildEmitterInfoJson(*FoundHandle);

	return FMCPToolResult::Success(TEXT("Emitter info retrieved"), EmitterJson);
}

// ============================================================================
// Helper: BuildSystemInfoJson
// ============================================================================

TSharedPtr<FJsonObject> FMCPTool_Niagara::BuildSystemInfoJson(UNiagaraSystem* NiagaraSystem)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
	ResultJson->SetStringField(TEXT("system_name"), NiagaraSystem->GetName());
	ResultJson->SetStringField(TEXT("asset_path"), NiagaraSystem->GetPathName());

	const TArray<FNiagaraEmitterHandle>& EmitterHandles = NiagaraSystem->GetEmitterHandles();
	ResultJson->SetNumberField(TEXT("emitter_count"), EmitterHandles.Num());

	TArray<TSharedPtr<FJsonValue>> EmittersArray;
	for (const FNiagaraEmitterHandle& Handle : EmitterHandles)
	{
		TSharedPtr<FJsonObject> EmitterObj = MakeShared<FJsonObject>();
		EmitterObj->SetStringField(TEXT("name"), Handle.GetName().ToString());
		EmitterObj->SetBoolField(TEXT("enabled"), Handle.GetIsEnabled());

		FVersionedNiagaraEmitterData* EmitterData = Handle.GetEmitterData();
		if (EmitterData)
		{
			EmitterObj->SetStringField(TEXT("sim_target"),
				EmitterData->SimTarget == ENiagaraSimTarget::CPUSim ? TEXT("CPU") : TEXT("GPU"));
			EmitterObj->SetBoolField(TEXT("local_space"), EmitterData->bLocalSpace);

			const TArray<UNiagaraRendererProperties*>& Renderers = EmitterData->GetRenderers();
			EmitterObj->SetNumberField(TEXT("renderer_count"), Renderers.Num());

			TArray<TSharedPtr<FJsonValue>> RendererTypes;
			for (const UNiagaraRendererProperties* Renderer : Renderers)
			{
				if (Renderer)
				{
					FString ClassName = Renderer->GetClass()->GetName();
					ClassName.RemoveFromStart(TEXT("Niagara"));
					ClassName.RemoveFromEnd(TEXT("RendererProperties"));
					RendererTypes.Add(MakeShared<FJsonValueString>(ClassName));
				}
			}
			EmitterObj->SetArrayField(TEXT("renderer_types"), RendererTypes);
		}

		EmittersArray.Add(MakeShared<FJsonValueObject>(EmitterObj));
	}
	ResultJson->SetArrayField(TEXT("emitters"), EmittersArray);

	const FNiagaraUserRedirectionParameterStore& ParamStore = NiagaraSystem->GetExposedParameters();
	TArray<FNiagaraVariable> UserParams;
	ParamStore.GetParameters(UserParams);

	TArray<TSharedPtr<FJsonValue>> UserParamsArray;
	for (const FNiagaraVariable& Var : UserParams)
	{
		const uint8* ValueData = ParamStore.GetParameterData(Var);
		TSharedPtr<FJsonObject> ParamObj = BuildParameterInfoJson(Var, ValueData);
		UserParamsArray.Add(MakeShared<FJsonValueObject>(ParamObj));
	}
	ResultJson->SetArrayField(TEXT("user_parameters"), UserParamsArray);

	return ResultJson;
}

// ============================================================================
// Helper: BuildEmitterInfoJson
// ============================================================================

TSharedPtr<FJsonObject> FMCPTool_Niagara::BuildEmitterInfoJson(const FNiagaraEmitterHandle& Handle)
{
	TSharedPtr<FJsonObject> EmitterJson = MakeShared<FJsonObject>();
	EmitterJson->SetStringField(TEXT("name"), Handle.GetName().ToString());
	EmitterJson->SetBoolField(TEXT("enabled"), Handle.GetIsEnabled());

	FVersionedNiagaraEmitterData* EmitterData = Handle.GetEmitterData();
	if (!EmitterData)
	{
		return EmitterJson;
	}

	EmitterJson->SetStringField(TEXT("sim_target"),
		EmitterData->SimTarget == ENiagaraSimTarget::CPUSim ? TEXT("CPU") : TEXT("GPU"));
	EmitterJson->SetBoolField(TEXT("local_space"), EmitterData->bLocalSpace);
	EmitterJson->SetBoolField(TEXT("deterministic"), EmitterData->bDeterminism);
	EmitterJson->SetNumberField(TEXT("random_seed"), EmitterData->RandomSeed);

	const TArray<UNiagaraRendererProperties*>& Renderers = EmitterData->GetRenderers();
	TArray<TSharedPtr<FJsonValue>> RenderersArray;
	for (const UNiagaraRendererProperties* Renderer : Renderers)
	{
		if (Renderer)
		{
			TSharedPtr<FJsonObject> RendererObj = BuildRendererInfoJson(Renderer);
			RenderersArray.Add(MakeShared<FJsonValueObject>(RendererObj));
		}
	}
	EmitterJson->SetArrayField(TEXT("renderers"), RenderersArray);

	return EmitterJson;
}

// ============================================================================
// Helper: BuildRendererInfoJson
// ============================================================================

TSharedPtr<FJsonObject> FMCPTool_Niagara::BuildRendererInfoJson(const UNiagaraRendererProperties* Renderer)
{
	TSharedPtr<FJsonObject> RendererObj = MakeShared<FJsonObject>();

	FString ClassName = Renderer->GetClass()->GetName();
	ClassName.RemoveFromStart(TEXT("Niagara"));
	ClassName.RemoveFromEnd(TEXT("RendererProperties"));
	RendererObj->SetStringField(TEXT("class_name"), ClassName);
	RendererObj->SetBoolField(TEXT("enabled"), Renderer->GetIsEnabled());

	if (const UNiagaraSpriteRendererProperties* SpriteRenderer = Cast<UNiagaraSpriteRendererProperties>(Renderer))
	{
		RendererObj->SetStringField(TEXT("alignment"), StaticEnum<ENiagaraSpriteAlignment>()->GetNameStringByValue(static_cast<int64>(SpriteRenderer->Alignment)));
		RendererObj->SetStringField(TEXT("facing_mode"), StaticEnum<ENiagaraSpriteFacingMode>()->GetNameStringByValue(static_cast<int64>(SpriteRenderer->FacingMode)));
		RendererObj->SetStringField(TEXT("sub_image_size"), FString::Printf(TEXT("%dx%d"),
			FMath::RoundToInt(SpriteRenderer->SubImageSize.X), FMath::RoundToInt(SpriteRenderer->SubImageSize.Y)));
	}
	else if (const UNiagaraMeshRendererProperties* MeshRenderer = Cast<UNiagaraMeshRendererProperties>(Renderer))
	{
		RendererObj->SetStringField(TEXT("facing_mode"), StaticEnum<ENiagaraMeshFacingMode>()->GetNameStringByValue(static_cast<int64>(MeshRenderer->FacingMode)));
		RendererObj->SetNumberField(TEXT("mesh_count"), MeshRenderer->Meshes.Num());
	}
	else if (const UNiagaraRibbonRendererProperties* RibbonRenderer = Cast<UNiagaraRibbonRendererProperties>(Renderer))
	{
		RendererObj->SetStringField(TEXT("facing_mode"), StaticEnum<ENiagaraRibbonFacingMode>()->GetNameStringByValue(static_cast<int64>(RibbonRenderer->FacingMode)));
		RendererObj->SetStringField(TEXT("shape_mode"), StaticEnum<ENiagaraRibbonShapeMode>()->GetNameStringByValue(static_cast<int64>(RibbonRenderer->Shape)));
		RendererObj->SetStringField(TEXT("draw_direction"), StaticEnum<ENiagaraRibbonDrawDirection>()->GetNameStringByValue(static_cast<int64>(RibbonRenderer->DrawDirection)));
	}
	else if (Cast<UNiagaraLightRendererProperties>(Renderer))
	{
		RendererObj->SetStringField(TEXT("type"), TEXT("Light"));
	}

	return RendererObj;
}

// ============================================================================
// Helper: BuildParameterInfoJson
// ============================================================================

TSharedPtr<FJsonObject> FMCPTool_Niagara::BuildParameterInfoJson(const FNiagaraVariable& Variable, const uint8* ValueData)
{
	TSharedPtr<FJsonObject> ParamObj = MakeShared<FJsonObject>();

	FString ParamName = Variable.GetName().ToString();
	ParamName.RemoveFromStart(TEXT("User."));
	ParamObj->SetStringField(TEXT("name"), ParamName);

	FNiagaraTypeDefinition TypeDef = Variable.GetType();
	ParamObj->SetStringField(TEXT("type"), NiagaraTypeToString(TypeDef));

	if (ValueData)
	{
		if (TypeDef == FNiagaraTypeDefinition::GetFloatDef())
		{
			float FloatVal = *reinterpret_cast<const float*>(ValueData);
			ParamObj->SetNumberField(TEXT("value"), FloatVal);
		}
		else if (TypeDef == FNiagaraTypeDefinition::GetIntDef())
		{
			int32 IntVal = *reinterpret_cast<const int32*>(ValueData);
			ParamObj->SetNumberField(TEXT("value"), IntVal);
		}
		else if (TypeDef == FNiagaraTypeDefinition::GetBoolDef())
		{
			const FNiagaraBool* BoolVal = reinterpret_cast<const FNiagaraBool*>(ValueData);
			ParamObj->SetBoolField(TEXT("value"), BoolVal->GetValue());
		}
		else if (TypeDef == FNiagaraTypeDefinition::GetVec3Def())
		{
			const FVector3f* VecVal = reinterpret_cast<const FVector3f*>(ValueData);
			ParamObj->SetStringField(TEXT("value"), FString::Printf(TEXT("(%g, %g, %g)"), VecVal->X, VecVal->Y, VecVal->Z));
		}
		else if (TypeDef == FNiagaraTypeDefinition::GetVec2Def())
		{
			const FVector2f* VecVal = reinterpret_cast<const FVector2f*>(ValueData);
			ParamObj->SetStringField(TEXT("value"), FString::Printf(TEXT("(%g, %g)"), VecVal->X, VecVal->Y));
		}
		else if (TypeDef == FNiagaraTypeDefinition::GetVec4Def())
		{
			const FVector4f* VecVal = reinterpret_cast<const FVector4f*>(ValueData);
			ParamObj->SetStringField(TEXT("value"), FString::Printf(TEXT("(%g, %g, %g, %g)"), VecVal->X, VecVal->Y, VecVal->Z, VecVal->W));
		}
		else if (TypeDef == FNiagaraTypeDefinition::GetColorDef())
		{
			const FLinearColor* ColorVal = reinterpret_cast<const FLinearColor*>(ValueData);
			ParamObj->SetStringField(TEXT("value"), FString::Printf(TEXT("(%g, %g, %g, %g)"), ColorVal->R, ColorVal->G, ColorVal->B, ColorVal->A));
		}
		else
		{
			ParamObj->SetStringField(TEXT("value"), FString::Printf(TEXT("<%s>"), *NiagaraTypeToString(TypeDef)));
		}
	}
	else
	{
		ParamObj->SetStringField(TEXT("value"), TEXT("<no data>"));
	}

	return ParamObj;
}

// ============================================================================
// Helper: NiagaraTypeToString
// ============================================================================

FString FMCPTool_Niagara::NiagaraTypeToString(const FNiagaraTypeDefinition& TypeDef) const
{
	if (TypeDef == FNiagaraTypeDefinition::GetFloatDef()) return TEXT("float");
	if (TypeDef == FNiagaraTypeDefinition::GetIntDef()) return TEXT("int");
	if (TypeDef == FNiagaraTypeDefinition::GetBoolDef()) return TEXT("bool");
	if (TypeDef == FNiagaraTypeDefinition::GetVec2Def()) return TEXT("vector2");
	if (TypeDef == FNiagaraTypeDefinition::GetVec3Def()) return TEXT("vector3");
	if (TypeDef == FNiagaraTypeDefinition::GetVec4Def()) return TEXT("vector4");
	if (TypeDef == FNiagaraTypeDefinition::GetColorDef()) return TEXT("color");
	if (TypeDef == FNiagaraTypeDefinition::GetQuatDef()) return TEXT("quaternion");
	if (TypeDef == FNiagaraTypeDefinition::GetMatrix4Def()) return TEXT("matrix");

	return TypeDef.GetFName().ToString();
}

// ============================================================================
// HandleSetParameter
// ============================================================================

FMCPToolResult FMCPTool_Niagara::HandleSetParameter(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath;
	TOptional<FMCPToolResult> ExtractError;
	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, ExtractError))
	{
		return ExtractError.GetValue();
	}

	FString ParamName;
	if (!ExtractRequiredString(Params, TEXT("parameter_name"), ParamName, ExtractError))
	{
		return ExtractError.GetValue();
	}

	UNiagaraSystem* NiagaraSystem = LoadObject<UNiagaraSystem>(nullptr, *AssetPath);
	if (!NiagaraSystem)
	{
		return FMCPToolResult::Error(FString::Printf(TEXT("Failed to load Niagara system: %s"), *AssetPath));
	}

	FNiagaraUserRedirectionParameterStore& ExposedParams =
		const_cast<FNiagaraUserRedirectionParameterStore&>(NiagaraSystem->GetExposedParameters());

	TArray<FNiagaraVariable> UserParams;
	ExposedParams.GetParameters(UserParams);

	const FNiagaraVariable* FoundParam = nullptr;
	for (const FNiagaraVariable& Var : UserParams)
	{
		FString VarName = Var.GetName().ToString();
		FString VarNameNoPrefix = VarName;
		VarNameNoPrefix.RemoveFromStart(TEXT("User."));

		if (VarNameNoPrefix == ParamName || VarName == ParamName)
		{
			FoundParam = &Var;
			break;
		}
	}

	if (!FoundParam)
	{
		TArray<FString> AvailableNames;
		for (const FNiagaraVariable& Var : UserParams)
		{
			FString VarName = Var.GetName().ToString();
			VarName.RemoveFromStart(TEXT("User."));
			AvailableNames.Add(VarName);
		}
		return FMCPToolResult::Error(FString::Printf(TEXT("Parameter '%s' not found on system '%s'. Available: %s"),
			*ParamName, *AssetPath, *FString::Join(AvailableNames, TEXT(", "))));
	}

	FNiagaraTypeDefinition TypeDef = FoundParam->GetType();
	FString TypeStr = NiagaraTypeToString(TypeDef);

	if (TypeDef == FNiagaraTypeDefinition::GetFloatDef())
	{
		double NumValue = 0.0;
		if (!Params->TryGetNumberField(TEXT("value"), NumValue))
		{
			return FMCPToolResult::Error(FString::Printf(TEXT("Parameter '%s' is float -- 'value' must be a number"), *ParamName));
		}
		float FloatVal = static_cast<float>(NumValue);
		ExposedParams.SetParameterData(reinterpret_cast<const uint8*>(&FloatVal), *FoundParam);
	}
	else if (TypeDef == FNiagaraTypeDefinition::GetIntDef())
	{
		double NumValue = 0.0;
		if (!Params->TryGetNumberField(TEXT("value"), NumValue))
		{
			return FMCPToolResult::Error(FString::Printf(TEXT("Parameter '%s' is int -- 'value' must be a number"), *ParamName));
		}
		int32 IntVal = FMath::RoundToInt32(NumValue);
		ExposedParams.SetParameterData(reinterpret_cast<const uint8*>(&IntVal), *FoundParam);
	}
	else if (TypeDef == FNiagaraTypeDefinition::GetBoolDef())
	{
		bool bBoolValue = false;
		if (!Params->TryGetBoolField(TEXT("value"), bBoolValue))
		{
			return FMCPToolResult::Error(FString::Printf(TEXT("Parameter '%s' is bool -- 'value' must be a boolean"), *ParamName));
		}
		FNiagaraBool NiagaraBoolVal;
		NiagaraBoolVal.SetValue(bBoolValue);
		ExposedParams.SetParameterData(reinterpret_cast<const uint8*>(&NiagaraBoolVal), *FoundParam);
	}
	else if (TypeDef == FNiagaraTypeDefinition::GetVec3Def())
	{
		const TSharedPtr<FJsonObject>* ValueObj = nullptr;
		if (!Params->TryGetObjectField(TEXT("value"), ValueObj) || !ValueObj)
		{
			return FMCPToolResult::Error(FString::Printf(TEXT("Parameter '%s' is vector3 -- 'value' must be an object {x,y,z}"), *ParamName));
		}
		FVector3f Vec3Val;
		Vec3Val.X = static_cast<float>((*ValueObj)->GetNumberField(TEXT("x")));
		Vec3Val.Y = static_cast<float>((*ValueObj)->GetNumberField(TEXT("y")));
		Vec3Val.Z = static_cast<float>((*ValueObj)->GetNumberField(TEXT("z")));
		ExposedParams.SetParameterData(reinterpret_cast<const uint8*>(&Vec3Val), *FoundParam);
	}
	else if (TypeDef == FNiagaraTypeDefinition::GetColorDef())
	{
		const TSharedPtr<FJsonObject>* ValueObj = nullptr;
		if (!Params->TryGetObjectField(TEXT("value"), ValueObj) || !ValueObj)
		{
			return FMCPToolResult::Error(FString::Printf(TEXT("Parameter '%s' is color -- 'value' must be an object {r,g,b,a}"), *ParamName));
		}
		FLinearColor ColorVal;
		ColorVal.R = static_cast<float>((*ValueObj)->GetNumberField(TEXT("r")));
		ColorVal.G = static_cast<float>((*ValueObj)->GetNumberField(TEXT("g")));
		ColorVal.B = static_cast<float>((*ValueObj)->GetNumberField(TEXT("b")));
		ColorVal.A = static_cast<float>((*ValueObj)->GetNumberField(TEXT("a")));
		ExposedParams.SetParameterData(reinterpret_cast<const uint8*>(&ColorVal), *FoundParam);
	}
	else if (TypeDef == FNiagaraTypeDefinition::GetVec2Def())
	{
		const TSharedPtr<FJsonObject>* ValueObj = nullptr;
		if (!Params->TryGetObjectField(TEXT("value"), ValueObj) || !ValueObj)
		{
			return FMCPToolResult::Error(FString::Printf(TEXT("Parameter '%s' is vector2 -- 'value' must be an object {x,y}"), *ParamName));
		}
		FVector2f Vec2Val;
		Vec2Val.X = static_cast<float>((*ValueObj)->GetNumberField(TEXT("x")));
		Vec2Val.Y = static_cast<float>((*ValueObj)->GetNumberField(TEXT("y")));
		ExposedParams.SetParameterData(reinterpret_cast<const uint8*>(&Vec2Val), *FoundParam);
	}
	else if (TypeDef == FNiagaraTypeDefinition::GetVec4Def())
	{
		const TSharedPtr<FJsonObject>* ValueObj = nullptr;
		if (!Params->TryGetObjectField(TEXT("value"), ValueObj) || !ValueObj)
		{
			return FMCPToolResult::Error(FString::Printf(TEXT("Parameter '%s' is vector4 -- 'value' must be an object {x,y,z,w}"), *ParamName));
		}
		FVector4f Vec4Val;
		Vec4Val.X = static_cast<float>((*ValueObj)->GetNumberField(TEXT("x")));
		Vec4Val.Y = static_cast<float>((*ValueObj)->GetNumberField(TEXT("y")));
		Vec4Val.Z = static_cast<float>((*ValueObj)->GetNumberField(TEXT("z")));
		Vec4Val.W = static_cast<float>((*ValueObj)->GetNumberField(TEXT("w")));
		ExposedParams.SetParameterData(reinterpret_cast<const uint8*>(&Vec4Val), *FoundParam);
	}
	else
	{
		return FMCPToolResult::Error(FString::Printf(TEXT("Unsupported parameter type '%s' for parameter '%s'"), *TypeStr, *ParamName));
	}

	NiagaraSystem->MarkPackageDirty();

	TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
	ResultJson->SetStringField(TEXT("parameter_name"), ParamName);
	ResultJson->SetStringField(TEXT("type"), TypeStr);
	ResultJson->SetStringField(TEXT("asset_path"), AssetPath);

	return FMCPToolResult::Success(FString::Printf(TEXT("Set parameter '%s' on '%s'"), *ParamName, *NiagaraSystem->GetName()), ResultJson);
}

// ============================================================================
// HandleSpawnSystem
// ============================================================================

FMCPToolResult FMCPTool_Niagara::HandleSpawnSystem(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath;
	TOptional<FMCPToolResult> ExtractError;
	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, ExtractError))
	{
		return ExtractError.GetValue();
	}

	UNiagaraSystem* NiagaraSystem = LoadObject<UNiagaraSystem>(nullptr, *AssetPath);
	if (!NiagaraSystem)
	{
		return FMCPToolResult::Error(FString::Printf(TEXT("Failed to load Niagara system: %s"), *AssetPath));
	}

	UWorld* OutWorld = nullptr;
	TOptional<FMCPToolResult> WorldError = ValidateEditorContext(OutWorld);
	if (WorldError.IsSet())
	{
		return WorldError.GetValue();
	}

	FVector SpawnLocation = ExtractVectorParam(Params, TEXT("location"), FVector::ZeroVector);
	FRotator SpawnRotation = ExtractRotatorParam(Params, TEXT("rotation"), FRotator::ZeroRotator);
	FVector NiagaraScale = ExtractScaleParam(Params, TEXT("scale"), FVector::OneVector);
	bool bAutoDestroy = ExtractOptionalBool(Params, TEXT("auto_destroy"), true);
	FString ActorLabel = ExtractOptionalString(Params, TEXT("label"));

	UNiagaraComponent* SpawnedComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		OutWorld, NiagaraSystem, SpawnLocation, SpawnRotation, NiagaraScale, bAutoDestroy, true);

	if (!SpawnedComponent)
	{
		return FMCPToolResult::Error(FString::Printf(TEXT("Failed to spawn Niagara system '%s' in the level"), *AssetPath));
	}

	AActor* OwnerActor = SpawnedComponent->GetOwner();
	if (!OwnerActor)
	{
		return FMCPToolResult::Error(TEXT("Niagara component spawned but has no owning actor"));
	}

	if (!ActorLabel.IsEmpty())
	{
		OwnerActor->SetActorLabel(ActorLabel);
	}

	OutWorld->MarkPackageDirty();

	TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
	ResultJson->SetStringField(TEXT("actor_name"), OwnerActor->GetName());
	ResultJson->SetStringField(TEXT("actor_label"), OwnerActor->GetActorLabel());
	ResultJson->SetStringField(TEXT("asset_path"), AssetPath);

	TSharedPtr<FJsonObject> LocationJson = MakeShared<FJsonObject>();
	LocationJson->SetNumberField(TEXT("x"), SpawnLocation.X);
	LocationJson->SetNumberField(TEXT("y"), SpawnLocation.Y);
	LocationJson->SetNumberField(TEXT("z"), SpawnLocation.Z);
	ResultJson->SetObjectField(TEXT("location"), LocationJson);

	TSharedPtr<FJsonObject> RotationJson = MakeShared<FJsonObject>();
	RotationJson->SetNumberField(TEXT("pitch"), SpawnRotation.Pitch);
	RotationJson->SetNumberField(TEXT("yaw"), SpawnRotation.Yaw);
	RotationJson->SetNumberField(TEXT("roll"), SpawnRotation.Roll);
	ResultJson->SetObjectField(TEXT("rotation"), RotationJson);

	TSharedPtr<FJsonObject> ScaleJson = MakeShared<FJsonObject>();
	ScaleJson->SetNumberField(TEXT("x"), NiagaraScale.X);
	ScaleJson->SetNumberField(TEXT("y"), NiagaraScale.Y);
	ScaleJson->SetNumberField(TEXT("z"), NiagaraScale.Z);
	ResultJson->SetObjectField(TEXT("scale"), ScaleJson);

	return FMCPToolResult::Success(FString::Printf(TEXT("Spawned Niagara system '%s' as '%s'"),
		*NiagaraSystem->GetName(), *OwnerActor->GetActorLabel()), ResultJson);
}
