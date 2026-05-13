// Copyright Natali Caggiano. All Rights Reserved.

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7

#include "MCPTool_MetaSound.h"
#include "MetaSoundEditor.h"

static FMCPToolResult MetaSoundJsonToToolResult(const TSharedPtr<FJsonObject>& Result, const FString& SuccessContext)
{
	if (!Result.IsValid())
	{
		return FMCPToolResult::Error(TEXT("MetaSound operation returned null result"));
	}

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

FMCPToolResult FMCPTool_MetaSound::Execute(const TSharedRef<FJsonObject>& Params)
{
	FString Operation;
	TOptional<FMCPToolResult> Error;
	if (!ExtractRequiredString(Params, TEXT("operation"), Operation, Error))
	{
		return Error.GetValue();
	}

	Operation = Operation.ToLower();

	// Operation aliases
	if (Operation == TEXT("info"))
	{
		Operation = TEXT("inspect");
	}
	else if (Operation == TEXT("graph"))
	{
		Operation = TEXT("get_graph");
	}
	else if (Operation == TEXT("create_metasound"))
	{
		Operation = TEXT("create");
	}

	// Read operations
	if (Operation == TEXT("inspect"))
	{
		return HandleInspect(Params);
	}
	else if (Operation == TEXT("list_nodes"))
	{
		return HandleListNodes(Params);
	}
	else if (Operation == TEXT("get_graph"))
	{
		return HandleGetGraph(Params);
	}
	// Write operations
	else if (Operation == TEXT("create"))
	{
		return HandleCreate(Params);
	}
	else if (Operation == TEXT("add_node"))
	{
		return HandleAddNode(Params);
	}
	else if (Operation == TEXT("remove_node"))
	{
		return HandleRemoveNode(Params);
	}
	else if (Operation == TEXT("connect"))
	{
		return HandleConnect(Params);
	}
	else if (Operation == TEXT("disconnect"))
	{
		return HandleDisconnect(Params);
	}
	else if (Operation == TEXT("set_input_default"))
	{
		return HandleSetInputDefault(Params);
	}
	else if (Operation == TEXT("add_graph_input"))
	{
		return HandleAddGraphInput(Params);
	}
	else if (Operation == TEXT("add_graph_output"))
	{
		return HandleAddGraphOutput(Params);
	}
	else if (Operation == TEXT("preview"))
	{
		return HandlePreview(Params);
	}
	else if (Operation == TEXT("stop_preview"))
	{
		return HandleStopPreview(Params);
	}

	return FMCPToolResult::Error(FString::Printf(
		TEXT("Unknown operation: '%s'. Valid: inspect, list_nodes, get_graph, create, add_node, remove_node, connect, disconnect, set_input_default, add_graph_input, add_graph_output, preview, stop_preview"),
		*Operation));
}

// ============================================================================
// Read Handlers
// ============================================================================

FMCPToolResult FMCPTool_MetaSound::HandleInspect(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath;
	TOptional<FMCPToolResult> Error;
	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}

	TSharedPtr<FJsonObject> Result = FMetaSoundEditor::InspectMetaSound(AssetPath);
	return MetaSoundJsonToToolResult(Result, TEXT("MetaSound inspected"));
}

FMCPToolResult FMCPTool_MetaSound::HandleListNodes(const TSharedRef<FJsonObject>& Params)
{
	FString NameFilter = ExtractOptionalString(Params, TEXT("name_filter"));
	FString CategoryFilter = ExtractOptionalString(Params, TEXT("category_filter"));
	int32 MaxResults = static_cast<int32>(ExtractOptionalNumber(Params, TEXT("max_results"), 50.0));

	TSharedPtr<FJsonObject> Result = FMetaSoundEditor::ListRegisteredNodes(NameFilter, CategoryFilter, MaxResults);
	return MetaSoundJsonToToolResult(Result, TEXT("Node types listed"));
}

FMCPToolResult FMCPTool_MetaSound::HandleGetGraph(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath;
	TOptional<FMCPToolResult> Error;
	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}

	TSharedPtr<FJsonObject> Result = FMetaSoundEditor::GetMetaSoundGraph(AssetPath);
	return MetaSoundJsonToToolResult(Result, TEXT("Graph structure retrieved"));
}

// ============================================================================
// Write Handlers
// ============================================================================

FMCPToolResult FMCPTool_MetaSound::HandleCreate(const TSharedRef<FJsonObject>& Params)
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

	FString OutputFormat = ExtractOptionalString(Params, TEXT("output_format"), TEXT("Mono"));

	TSharedPtr<FJsonObject> Result = FMetaSoundEditor::CreateMetaSound(PackagePath, Name, OutputFormat);
	return MetaSoundJsonToToolResult(Result, TEXT("MetaSound created"));
}

FMCPToolResult FMCPTool_MetaSound::HandleAddNode(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath, NodeClassName;
	TOptional<FMCPToolResult> Error;

	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("node_class_name"), NodeClassName, Error))
	{
		return Error.GetValue();
	}

	FString NodeNamespace = ExtractOptionalString(Params, TEXT("node_namespace"));

	TSharedPtr<FJsonObject> Result = FMetaSoundEditor::AddNode(AssetPath, NodeClassName, NodeNamespace);
	return MetaSoundJsonToToolResult(Result, TEXT("Node added"));
}

FMCPToolResult FMCPTool_MetaSound::HandleRemoveNode(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath, NodeId;
	TOptional<FMCPToolResult> Error;

	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("node_id"), NodeId, Error))
	{
		return Error.GetValue();
	}

	TSharedPtr<FJsonObject> Result = FMetaSoundEditor::RemoveNode(AssetPath, NodeId);
	return MetaSoundJsonToToolResult(Result, TEXT("Node removed"));
}

FMCPToolResult FMCPTool_MetaSound::HandleConnect(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath, FromNodeId, FromPin, ToNodeId, ToPin;
	TOptional<FMCPToolResult> Error;

	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("from_node_id"), FromNodeId, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("from_pin"), FromPin, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("to_node_id"), ToNodeId, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("to_pin"), ToPin, Error))
	{
		return Error.GetValue();
	}

	TSharedPtr<FJsonObject> Result = FMetaSoundEditor::ConnectNodes(AssetPath, FromNodeId, FromPin, ToNodeId, ToPin);
	return MetaSoundJsonToToolResult(Result, TEXT("Nodes connected"));
}

FMCPToolResult FMCPTool_MetaSound::HandleDisconnect(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath, FromNodeId, FromPin, ToNodeId, ToPin;
	TOptional<FMCPToolResult> Error;

	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("from_node_id"), FromNodeId, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("from_pin"), FromPin, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("to_node_id"), ToNodeId, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("to_pin"), ToPin, Error))
	{
		return Error.GetValue();
	}

	TSharedPtr<FJsonObject> Result = FMetaSoundEditor::DisconnectNodes(AssetPath, FromNodeId, FromPin, ToNodeId, ToPin);
	return MetaSoundJsonToToolResult(Result, TEXT("Nodes disconnected"));
}

FMCPToolResult FMCPTool_MetaSound::HandleSetInputDefault(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath, NodeId, InputName, Value;
	TOptional<FMCPToolResult> Error;

	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("node_id"), NodeId, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("input_name"), InputName, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("value"), Value, Error))
	{
		return Error.GetValue();
	}

	FString DataType = ExtractOptionalString(Params, TEXT("data_type"), TEXT("float"));

	TSharedPtr<FJsonObject> Result = FMetaSoundEditor::SetInputDefault(AssetPath, NodeId, InputName, Value, DataType);
	return MetaSoundJsonToToolResult(Result, TEXT("Input default set"));
}

FMCPToolResult FMCPTool_MetaSound::HandleAddGraphInput(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath, InputName, DataType;
	TOptional<FMCPToolResult> Error;

	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("input_name"), InputName, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("data_type"), DataType, Error))
	{
		return Error.GetValue();
	}

	FString DefaultValue = ExtractOptionalString(Params, TEXT("default_value"));

	TSharedPtr<FJsonObject> Result = FMetaSoundEditor::AddGraphInput(AssetPath, InputName, DataType, DefaultValue);
	return MetaSoundJsonToToolResult(Result, TEXT("Graph input added"));
}

FMCPToolResult FMCPTool_MetaSound::HandleAddGraphOutput(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath, OutputName, DataType;
	TOptional<FMCPToolResult> Error;

	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("output_name"), OutputName, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("data_type"), DataType, Error))
	{
		return Error.GetValue();
	}

	TSharedPtr<FJsonObject> Result = FMetaSoundEditor::AddGraphOutput(AssetPath, OutputName, DataType);
	return MetaSoundJsonToToolResult(Result, TEXT("Graph output added"));
}

FMCPToolResult FMCPTool_MetaSound::HandlePreview(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath;
	TOptional<FMCPToolResult> Error;
	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}

	TSharedPtr<FJsonObject> Result = FMetaSoundEditor::PreviewMetaSound(AssetPath);
	return MetaSoundJsonToToolResult(Result, TEXT("MetaSound preview started"));
}

FMCPToolResult FMCPTool_MetaSound::HandleStopPreview(const TSharedRef<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Result = FMetaSoundEditor::StopPreview();
	return MetaSoundJsonToToolResult(Result, TEXT("MetaSound preview stopped"));
}

#endif // ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
