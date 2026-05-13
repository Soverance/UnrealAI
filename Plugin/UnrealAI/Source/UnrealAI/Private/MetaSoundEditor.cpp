// Copyright Natali Caggiano. All Rights Reserved.

#include "MetaSoundEditor.h"

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7

#include "MetasoundSource.h"
#include "MetasoundFrontendDocument.h"
#include "MetasoundFrontendDocumentBuilder.h"
#include "MetasoundFrontendRegistries.h"
#include "MetasoundFrontendSearchEngine.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/AudioComponent.h"
#include "Editor.h"
#include "Dom/JsonValue.h"

static TWeakObjectPtr<UAudioComponent> GMetaSoundPreviewComp;

// ============================================================================
// Private Helpers
// ============================================================================

TSharedPtr<FJsonObject> FMetaSoundEditor::SuccessResult(const FString& Message)
{
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("message"), Message);
	return Result;
}

TSharedPtr<FJsonObject> FMetaSoundEditor::ErrorResult(const FString& Message)
{
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), false);
	Result->SetStringField(TEXT("error"), Message);
	return Result;
}

// ============================================================================
// InspectMetaSound
// ============================================================================

TSharedPtr<FJsonObject> FMetaSoundEditor::InspectMetaSound(const FString& AssetPath)
{
	UObject* Loaded = StaticLoadObject(UObject::StaticClass(), nullptr, *AssetPath);
	if (!Loaded)
	{
		return ErrorResult(FString::Printf(TEXT("Failed to load asset: %s"), *AssetPath));
	}

	UMetaSoundSource* MetaSoundAsset = Cast<UMetaSoundSource>(Loaded);
	if (!MetaSoundAsset)
	{
		return ErrorResult(FString::Printf(TEXT("Not a MetaSound source: %s (is %s)"), *AssetPath, *Loaded->GetClass()->GetName()));
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("type"), TEXT("MetaSoundSource"));
	Result->SetStringField(TEXT("asset_path"), MetaSoundAsset->GetPathName());
	Result->SetStringField(TEXT("asset_name"), MetaSoundAsset->GetName());
	Result->SetNumberField(TEXT("duration"), MetaSoundAsset->GetDuration());

	const FMetasoundFrontendDocument& Doc = MetaSoundAsset->GetConstDocument();

	Result->SetNumberField(TEXT("node_count"), Doc.RootGraph.Graph.Nodes.Num());
	Result->SetNumberField(TEXT("edge_count"), Doc.RootGraph.Graph.Edges.Num());

	TArray<TSharedPtr<FJsonValue>> InputsArray;
	for (const FMetasoundFrontendClassInput& ClassInput : Doc.RootGraph.Interface.Inputs)
	{
		TSharedPtr<FJsonObject> InputJson = MakeShared<FJsonObject>();
		InputJson->SetStringField(TEXT("name"), ClassInput.Name.ToString());
		InputJson->SetStringField(TEXT("type"), ClassInput.TypeName.ToString());
		InputsArray.Add(MakeShared<FJsonValueObject>(InputJson));
	}
	Result->SetArrayField(TEXT("graph_inputs"), InputsArray);

	TArray<TSharedPtr<FJsonValue>> OutputsArray;
	for (const FMetasoundFrontendClassOutput& ClassOutput : Doc.RootGraph.Interface.Outputs)
	{
		TSharedPtr<FJsonObject> OutputJson = MakeShared<FJsonObject>();
		OutputJson->SetStringField(TEXT("name"), ClassOutput.Name.ToString());
		OutputJson->SetStringField(TEXT("type"), ClassOutput.TypeName.ToString());
		OutputsArray.Add(MakeShared<FJsonValueObject>(OutputJson));
	}
	Result->SetArrayField(TEXT("graph_outputs"), OutputsArray);

	TArray<TSharedPtr<FJsonValue>> InterfacesArray;
	for (const FMetasoundFrontendVersion& InterfaceVer : Doc.Interfaces)
	{
		TSharedPtr<FJsonObject> InterfaceJson = MakeShared<FJsonObject>();
		InterfaceJson->SetStringField(TEXT("name"), InterfaceVer.Name.ToString());
		InterfaceJson->SetNumberField(TEXT("major_version"), InterfaceVer.Number.Major);
		InterfaceJson->SetNumberField(TEXT("minor_version"), InterfaceVer.Number.Minor);
		InterfacesArray.Add(MakeShared<FJsonValueObject>(InterfaceJson));
	}
	Result->SetArrayField(TEXT("interfaces"), InterfacesArray);

	Result->SetStringField(TEXT("message"), FString::Printf(TEXT("MetaSound: %s (%d nodes, %d edges, %d inputs, %d outputs)"),
		*MetaSoundAsset->GetName(),
		Doc.RootGraph.Graph.Nodes.Num(), Doc.RootGraph.Graph.Edges.Num(),
		InputsArray.Num(), OutputsArray.Num()));

	return Result;
}

// ============================================================================
// ListRegisteredNodes
// ============================================================================

TSharedPtr<FJsonObject> FMetaSoundEditor::ListRegisteredNodes(const FString& NameFilter, const FString& CategoryFilter, int32 MaxResults)
{
	if (MaxResults <= 0)
	{
		MaxResults = 50;
	}

	TArray<TSharedPtr<FJsonValue>> NodesArray;
	int32 TotalMatched = 0;

#if WITH_EDITORONLY_DATA
	TArray<FMetasoundFrontendClass> AllClasses = Metasound::Frontend::ISearchEngine::Get().FindAllClasses(false);

	for (const FMetasoundFrontendClass& NodeClass : AllClasses)
	{
		const FMetasoundFrontendClassName& ClassName = NodeClass.Metadata.GetClassName();
		FString FullName = ClassName.ToString();
		FString NodeNamespace = ClassName.Namespace.ToString();
		FString NodeName = ClassName.Name.ToString();

		if (!NameFilter.IsEmpty() && !FullName.Contains(NameFilter))
		{
			continue;
		}

		if (!CategoryFilter.IsEmpty() && !NodeNamespace.Contains(CategoryFilter))
		{
			continue;
		}

		TotalMatched++;

		if (NodesArray.Num() >= MaxResults)
		{
			continue;
		}

		TSharedPtr<FJsonObject> NodeJson = MakeShared<FJsonObject>();
		NodeJson->SetStringField(TEXT("class_name"), NodeName);
		NodeJson->SetStringField(TEXT("namespace"), NodeNamespace);
		NodeJson->SetStringField(TEXT("full_name"), FullName);

		const FMetasoundFrontendVersionNumber& Ver = NodeClass.Metadata.GetVersion();
		NodeJson->SetStringField(TEXT("version"), FString::Printf(TEXT("%d.%d"), Ver.Major, Ver.Minor));

		NodesArray.Add(MakeShared<FJsonValueObject>(NodeJson));
	}
#endif

	TSharedPtr<FJsonObject> Result = SuccessResult(
		FString::Printf(TEXT("Found %d registered node classes (%d shown, max %d)"), TotalMatched, NodesArray.Num(), MaxResults));
	Result->SetArrayField(TEXT("nodes"), NodesArray);
	Result->SetNumberField(TEXT("count"), NodesArray.Num());
	Result->SetNumberField(TEXT("total_matches"), TotalMatched);
	return Result;
}

// ============================================================================
// GetMetaSoundGraph
// ============================================================================

TSharedPtr<FJsonObject> FMetaSoundEditor::GetMetaSoundGraph(const FString& AssetPath)
{
	UObject* Loaded = StaticLoadObject(UObject::StaticClass(), nullptr, *AssetPath);
	if (!Loaded)
	{
		return ErrorResult(FString::Printf(TEXT("Failed to load asset: %s"), *AssetPath));
	}

	UMetaSoundSource* MetaSoundAsset = Cast<UMetaSoundSource>(Loaded);
	if (!MetaSoundAsset)
	{
		return ErrorResult(FString::Printf(TEXT("Not a MetaSound source: %s (is %s)"), *AssetPath, *Loaded->GetClass()->GetName()));
	}

	const FMetasoundFrontendDocument& Doc = MetaSoundAsset->GetConstDocument();

	TMap<FGuid, const FMetasoundFrontendClass*> DepMap;
	for (const FMetasoundFrontendClass& Dep : Doc.Dependencies)
	{
		DepMap.Add(Dep.ID, &Dep);
	}

	TArray<TSharedPtr<FJsonValue>> NodesArray;
	TArray<TSharedPtr<FJsonValue>> EdgesArray;

	auto ProcessGraph = [&](const FMetasoundFrontendGraph& GraphData)
	{
		for (const FMetasoundFrontendNode& Node : GraphData.Nodes)
		{
			TSharedPtr<FJsonObject> NodeJson = MakeShared<FJsonObject>();
			NodeJson->SetStringField(TEXT("node_id"), Node.GetID().ToString());

			if (const FMetasoundFrontendClass* const* DepPtr = DepMap.Find(Node.ClassID))
			{
				const FMetasoundFrontendClassName& ClassName = (*DepPtr)->Metadata.GetClassName();
				NodeJson->SetStringField(TEXT("class_name"), ClassName.Name.ToString());
				NodeJson->SetStringField(TEXT("namespace"), ClassName.Namespace.ToString());
			}
			else
			{
				NodeJson->SetStringField(TEXT("class_name"), TEXT("unknown"));
				NodeJson->SetStringField(TEXT("namespace"), TEXT("unknown"));
			}

			TArray<TSharedPtr<FJsonValue>> NodeInputsArray;
			for (const FMetasoundFrontendVertex& InputVertex : Node.Interface.Inputs)
			{
				TSharedPtr<FJsonObject> InputJson = MakeShared<FJsonObject>();
				InputJson->SetStringField(TEXT("name"), InputVertex.Name.ToString());
				InputJson->SetStringField(TEXT("type"), InputVertex.TypeName.ToString());
				InputJson->SetStringField(TEXT("vertex_id"), InputVertex.VertexID.ToString());
				NodeInputsArray.Add(MakeShared<FJsonValueObject>(InputJson));
			}
			NodeJson->SetArrayField(TEXT("inputs"), NodeInputsArray);

			TArray<TSharedPtr<FJsonValue>> NodeOutputsArray;
			for (const FMetasoundFrontendVertex& OutputVertex : Node.Interface.Outputs)
			{
				TSharedPtr<FJsonObject> OutputJson = MakeShared<FJsonObject>();
				OutputJson->SetStringField(TEXT("name"), OutputVertex.Name.ToString());
				OutputJson->SetStringField(TEXT("type"), OutputVertex.TypeName.ToString());
				OutputJson->SetStringField(TEXT("vertex_id"), OutputVertex.VertexID.ToString());
				NodeOutputsArray.Add(MakeShared<FJsonValueObject>(OutputJson));
			}
			NodeJson->SetArrayField(TEXT("outputs"), NodeOutputsArray);

			NodesArray.Add(MakeShared<FJsonValueObject>(NodeJson));
		}

		for (const FMetasoundFrontendEdge& Edge : GraphData.Edges)
		{
			TSharedPtr<FJsonObject> EdgeJson = MakeShared<FJsonObject>();
			EdgeJson->SetStringField(TEXT("from_node_id"), Edge.FromNodeID.ToString());
			EdgeJson->SetStringField(TEXT("from_vertex_id"), Edge.FromVertexID.ToString());
			EdgeJson->SetStringField(TEXT("to_node_id"), Edge.ToNodeID.ToString());
			EdgeJson->SetStringField(TEXT("to_vertex_id"), Edge.ToVertexID.ToString());
			EdgesArray.Add(MakeShared<FJsonValueObject>(EdgeJson));
		}
	};

	ProcessGraph(Doc.RootGraph.Graph);

	TArray<TSharedPtr<FJsonValue>> GraphInputsArray;
	for (const FMetasoundFrontendClassInput& ClassInput : Doc.RootGraph.Interface.Inputs)
	{
		TSharedPtr<FJsonObject> InputJson = MakeShared<FJsonObject>();
		InputJson->SetStringField(TEXT("name"), ClassInput.Name.ToString());
		InputJson->SetStringField(TEXT("type"), ClassInput.TypeName.ToString());
		GraphInputsArray.Add(MakeShared<FJsonValueObject>(InputJson));
	}

	TArray<TSharedPtr<FJsonValue>> GraphOutputsArray;
	for (const FMetasoundFrontendClassOutput& ClassOutput : Doc.RootGraph.Interface.Outputs)
	{
		TSharedPtr<FJsonObject> OutputJson = MakeShared<FJsonObject>();
		OutputJson->SetStringField(TEXT("name"), ClassOutput.Name.ToString());
		OutputJson->SetStringField(TEXT("type"), ClassOutput.TypeName.ToString());
		GraphOutputsArray.Add(MakeShared<FJsonValueObject>(OutputJson));
	}

	TSharedPtr<FJsonObject> Result = SuccessResult(
		FString::Printf(TEXT("MetaSound graph: %d nodes, %d edges, %d inputs, %d outputs"),
			NodesArray.Num(), EdgesArray.Num(), GraphInputsArray.Num(), GraphOutputsArray.Num()));
	Result->SetArrayField(TEXT("nodes"), NodesArray);
	Result->SetArrayField(TEXT("edges"), EdgesArray);
	Result->SetArrayField(TEXT("graph_inputs"), GraphInputsArray);
	Result->SetArrayField(TEXT("graph_outputs"), GraphOutputsArray);
	return Result;
}

// ============================================================================
// Write Operations
// ============================================================================

TSharedPtr<FJsonObject> FMetaSoundEditor::CreateMetaSound(const FString& PackagePath, const FString& Name, const FString& OutputFormat)
{
	FString FullPackagePath = PackagePath / Name;
	UPackage* NewPackage = CreatePackage(*FullPackagePath);
	if (!NewPackage)
	{
		return ErrorResult(FString::Printf(TEXT("Failed to create package: %s"), *FullPackagePath));
	}

	UMetaSoundSource* NewMetaSound = NewObject<UMetaSoundSource>(NewPackage, FName(*Name), RF_Public | RF_Standalone);
	if (!NewMetaSound)
	{
		return ErrorResult(TEXT("Failed to create UMetaSoundSource object"));
	}

	TScriptInterface<IMetaSoundDocumentInterface> DocInterface(NewMetaSound);
	FMetaSoundFrontendDocumentBuilder DocBuilder(DocInterface);
	DocBuilder.InitDocument();

	FAssetRegistryModule::AssetCreated(NewMetaSound);
	NewMetaSound->MarkPackageDirty();

	TSharedPtr<FJsonObject> Result = SuccessResult(FString::Printf(TEXT("Created MetaSound: %s"), *NewMetaSound->GetPathName()));
	Result->SetStringField(TEXT("asset_path"), NewMetaSound->GetPathName());
	return Result;
}

// ============================================================================
// AddNode
// ============================================================================

TSharedPtr<FJsonObject> FMetaSoundEditor::AddNode(const FString& AssetPath, const FString& NodeClassName, const FString& NodeNamespace)
{
	UMetaSoundSource* MetaSoundAsset = LoadObject<UMetaSoundSource>(nullptr, *AssetPath);
	if (!MetaSoundAsset)
	{
		return ErrorResult(FString::Printf(TEXT("Failed to load MetaSound: %s"), *AssetPath));
	}

	TScriptInterface<IMetaSoundDocumentInterface> DocInterface(MetaSoundAsset);
	FMetaSoundFrontendDocumentBuilder DocBuilder(DocInterface);

	FName NsName(*NodeNamespace);
	FName ClsName(*NodeClassName);
	FMetasoundFrontendClassName ClassName(NsName, ClsName);
	const FMetasoundFrontendNode* NewNode = DocBuilder.AddNodeByClassName(ClassName, 1);
	if (!NewNode)
	{
		return ErrorResult(FString::Printf(TEXT("Failed to add node: %s::%s"), *NodeNamespace, *NodeClassName));
	}

	MetaSoundAsset->MarkPackageDirty();

	TSharedPtr<FJsonObject> Result = SuccessResult(FString::Printf(TEXT("Added node: %s::%s"), *NodeNamespace, *NodeClassName));
	Result->SetStringField(TEXT("node_id"), NewNode->GetID().ToString());
	return Result;
}

// ============================================================================
// RemoveNode
// ============================================================================

TSharedPtr<FJsonObject> FMetaSoundEditor::RemoveNode(const FString& AssetPath, const FString& NodeGuid)
{
	UMetaSoundSource* MetaSoundAsset = LoadObject<UMetaSoundSource>(nullptr, *AssetPath);
	if (!MetaSoundAsset)
	{
		return ErrorResult(FString::Printf(TEXT("Failed to load MetaSound: %s"), *AssetPath));
	}

	FGuid ParsedGuid;
	if (!FGuid::Parse(NodeGuid, ParsedGuid))
	{
		return ErrorResult(FString::Printf(TEXT("Invalid GUID: %s"), *NodeGuid));
	}

	TScriptInterface<IMetaSoundDocumentInterface> DocInterface(MetaSoundAsset);
	FMetaSoundFrontendDocumentBuilder DocBuilder(DocInterface);

	DocBuilder.RemoveEdges(ParsedGuid);
	bool bRemoved = DocBuilder.RemoveNode(ParsedGuid);
	if (!bRemoved)
	{
		return ErrorResult(FString::Printf(TEXT("Failed to remove node: %s"), *NodeGuid));
	}

	MetaSoundAsset->MarkPackageDirty();

	return SuccessResult(FString::Printf(TEXT("Removed node: %s"), *NodeGuid));
}

// ============================================================================
// ConnectNodes
// ============================================================================

TSharedPtr<FJsonObject> FMetaSoundEditor::ConnectNodes(const FString& AssetPath, const FString& FromNodeGuid, const FString& FromPinName, const FString& ToNodeGuid, const FString& ToPinName)
{
	UMetaSoundSource* MetaSoundAsset = LoadObject<UMetaSoundSource>(nullptr, *AssetPath);
	if (!MetaSoundAsset)
	{
		return ErrorResult(FString::Printf(TEXT("Failed to load MetaSound: %s"), *AssetPath));
	}

	FGuid FromGuid, ToGuid;
	if (!FGuid::Parse(FromNodeGuid, FromGuid))
	{
		return ErrorResult(FString::Printf(TEXT("Invalid from-node GUID: %s"), *FromNodeGuid));
	}
	if (!FGuid::Parse(ToNodeGuid, ToGuid))
	{
		return ErrorResult(FString::Printf(TEXT("Invalid to-node GUID: %s"), *ToNodeGuid));
	}

	TScriptInterface<IMetaSoundDocumentInterface> DocInterface(MetaSoundAsset);
	FMetaSoundFrontendDocumentBuilder DocBuilder(DocInterface);

	const FMetasoundFrontendVertex* OutputVertex = DocBuilder.FindNodeOutput(FromGuid, FName(*FromPinName));
	if (!OutputVertex)
	{
		return ErrorResult(FString::Printf(TEXT("Output pin not found: %s on node %s"), *FromPinName, *FromNodeGuid));
	}

	const FMetasoundFrontendVertex* InputVertex = DocBuilder.FindNodeInput(ToGuid, FName(*ToPinName));
	if (!InputVertex)
	{
		return ErrorResult(FString::Printf(TEXT("Input pin not found: %s on node %s"), *ToPinName, *ToNodeGuid));
	}

	FMetasoundFrontendEdge NewEdge;
	NewEdge.FromNodeID = FromGuid;
	NewEdge.FromVertexID = OutputVertex->VertexID;
	NewEdge.ToNodeID = ToGuid;
	NewEdge.ToVertexID = InputVertex->VertexID;

	DocBuilder.AddEdge(MoveTemp(NewEdge));

	MetaSoundAsset->MarkPackageDirty();

	TSharedPtr<FJsonObject> Result = SuccessResult(TEXT("Connected nodes"));
	Result->SetStringField(TEXT("from_node"), FromNodeGuid);
	Result->SetStringField(TEXT("from_pin"), FromPinName);
	Result->SetStringField(TEXT("to_node"), ToNodeGuid);
	Result->SetStringField(TEXT("to_pin"), ToPinName);
	return Result;
}

// ============================================================================
// DisconnectNodes
// ============================================================================

TSharedPtr<FJsonObject> FMetaSoundEditor::DisconnectNodes(const FString& AssetPath, const FString& FromNodeGuid, const FString& FromPinName, const FString& ToNodeGuid, const FString& ToPinName)
{
	UMetaSoundSource* MetaSoundAsset = LoadObject<UMetaSoundSource>(nullptr, *AssetPath);
	if (!MetaSoundAsset)
	{
		return ErrorResult(FString::Printf(TEXT("Failed to load MetaSound: %s"), *AssetPath));
	}

	FGuid FromGuid, ToGuid;
	if (!FGuid::Parse(FromNodeGuid, FromGuid))
	{
		return ErrorResult(FString::Printf(TEXT("Invalid from-node GUID: %s"), *FromNodeGuid));
	}
	if (!FGuid::Parse(ToNodeGuid, ToGuid))
	{
		return ErrorResult(FString::Printf(TEXT("Invalid to-node GUID: %s"), *ToNodeGuid));
	}

	TScriptInterface<IMetaSoundDocumentInterface> DocInterface(MetaSoundAsset);
	FMetaSoundFrontendDocumentBuilder DocBuilder(DocInterface);

	const FMetasoundFrontendVertex* OutputVertex = DocBuilder.FindNodeOutput(FromGuid, FName(*FromPinName));
	if (!OutputVertex)
	{
		return ErrorResult(FString::Printf(TEXT("Output pin not found: %s on node %s"), *FromPinName, *FromNodeGuid));
	}

	const FMetasoundFrontendVertex* InputVertex = DocBuilder.FindNodeInput(ToGuid, FName(*ToPinName));
	if (!InputVertex)
	{
		return ErrorResult(FString::Printf(TEXT("Input pin not found: %s on node %s"), *ToPinName, *ToNodeGuid));
	}

	FMetasoundFrontendEdge EdgeToRemove;
	EdgeToRemove.FromNodeID = FromGuid;
	EdgeToRemove.FromVertexID = OutputVertex->VertexID;
	EdgeToRemove.ToNodeID = ToGuid;
	EdgeToRemove.ToVertexID = InputVertex->VertexID;

	bool bRemoved = DocBuilder.RemoveEdge(EdgeToRemove);
	if (!bRemoved)
	{
		return ErrorResult(TEXT("Failed to remove edge - edge may not exist"));
	}

	MetaSoundAsset->MarkPackageDirty();

	return SuccessResult(TEXT("Disconnected nodes"));
}

// ============================================================================
// SetInputDefault
// ============================================================================

TSharedPtr<FJsonObject> FMetaSoundEditor::SetInputDefault(const FString& AssetPath, const FString& NodeGuid, const FString& InputName, const FString& Value, const FString& DataType)
{
	UMetaSoundSource* MetaSoundAsset = LoadObject<UMetaSoundSource>(nullptr, *AssetPath);
	if (!MetaSoundAsset)
	{
		return ErrorResult(FString::Printf(TEXT("Failed to load MetaSound: %s"), *AssetPath));
	}

	FGuid ParsedNodeGuid;
	if (!FGuid::Parse(NodeGuid, ParsedNodeGuid))
	{
		return ErrorResult(FString::Printf(TEXT("Invalid node GUID: %s"), *NodeGuid));
	}

	TScriptInterface<IMetaSoundDocumentInterface> DocInterface(MetaSoundAsset);
	FMetaSoundFrontendDocumentBuilder DocBuilder(DocInterface);

	const FMetasoundFrontendVertex* InputVertex = DocBuilder.FindNodeInput(ParsedNodeGuid, FName(*InputName));
	if (!InputVertex)
	{
		return ErrorResult(FString::Printf(TEXT("Input not found: %s on node %s"), *InputName, *NodeGuid));
	}

	FMetasoundFrontendLiteral LiteralValue;
	FString LowerDataType = DataType.ToLower();
	if (LowerDataType == TEXT("float"))
	{
		LiteralValue.Set(FCString::Atof(*Value));
	}
	else if (LowerDataType == TEXT("int32"))
	{
		LiteralValue.Set(FCString::Atoi(*Value));
	}
	else if (LowerDataType == TEXT("bool"))
	{
		LiteralValue.Set(Value.ToLower() == TEXT("true") || Value == TEXT("1"));
	}
	else
	{
		LiteralValue.Set(Value);
	}

	bool bSet = DocBuilder.SetNodeInputDefault(ParsedNodeGuid, InputVertex->VertexID, LiteralValue);
	if (!bSet)
	{
		return ErrorResult(FString::Printf(TEXT("Failed to set default on input '%s'"), *InputName));
	}

	MetaSoundAsset->MarkPackageDirty();

	return SuccessResult(FString::Printf(TEXT("Set default on input '%s' to '%s'"), *InputName, *Value));
}

// ============================================================================
// AddGraphInput
// ============================================================================

TSharedPtr<FJsonObject> FMetaSoundEditor::AddGraphInput(const FString& AssetPath, const FString& InputName, const FString& DataType, const FString& DefaultValue)
{
	UMetaSoundSource* MetaSoundAsset = LoadObject<UMetaSoundSource>(nullptr, *AssetPath);
	if (!MetaSoundAsset)
	{
		return ErrorResult(FString::Printf(TEXT("Failed to load MetaSound: %s"), *AssetPath));
	}

	TScriptInterface<IMetaSoundDocumentInterface> DocInterface(MetaSoundAsset);
	FMetaSoundFrontendDocumentBuilder DocBuilder(DocInterface);

	FMetasoundFrontendClassInput NewClassInput;
	NewClassInput.Name = FName(*InputName);
	NewClassInput.TypeName = FName(*DataType);
	NewClassInput.VertexID = FGuid::NewGuid();

#if WITH_EDITORONLY_DATA
	if (!DefaultValue.IsEmpty())
	{
		FString LowerType = DataType.ToLower();
		if (LowerType == TEXT("float"))
		{
			NewClassInput.DefaultLiteral.Set(FCString::Atof(*DefaultValue));
		}
		else if (LowerType == TEXT("int32"))
		{
			NewClassInput.DefaultLiteral.Set(FCString::Atoi(*DefaultValue));
		}
		else if (LowerType == TEXT("bool"))
		{
			NewClassInput.DefaultLiteral.Set(DefaultValue.ToLower() == TEXT("true") || DefaultValue == TEXT("1"));
		}
		else
		{
			NewClassInput.DefaultLiteral.Set(DefaultValue);
		}
	}
#endif

	const FMetasoundFrontendNode* InputNode = DocBuilder.AddGraphInput(MoveTemp(NewClassInput));
	if (!InputNode)
	{
		return ErrorResult(FString::Printf(TEXT("Failed to add graph input: %s"), *InputName));
	}

	MetaSoundAsset->MarkPackageDirty();

	TSharedPtr<FJsonObject> Result = SuccessResult(FString::Printf(TEXT("Added graph input: %s (%s)"), *InputName, *DataType));
	Result->SetStringField(TEXT("input_node_id"), InputNode->GetID().ToString());
	return Result;
}

// ============================================================================
// AddGraphOutput
// ============================================================================

TSharedPtr<FJsonObject> FMetaSoundEditor::AddGraphOutput(const FString& AssetPath, const FString& OutputName, const FString& DataType)
{
	UMetaSoundSource* MetaSoundAsset = LoadObject<UMetaSoundSource>(nullptr, *AssetPath);
	if (!MetaSoundAsset)
	{
		return ErrorResult(FString::Printf(TEXT("Failed to load MetaSound: %s"), *AssetPath));
	}

	TScriptInterface<IMetaSoundDocumentInterface> DocInterface(MetaSoundAsset);
	FMetaSoundFrontendDocumentBuilder DocBuilder(DocInterface);

	FMetasoundFrontendClassOutput NewClassOutput;
	NewClassOutput.Name = FName(*OutputName);
	NewClassOutput.TypeName = FName(*DataType);
	NewClassOutput.VertexID = FGuid::NewGuid();

	const FMetasoundFrontendNode* OutputNode = DocBuilder.AddGraphOutput(MoveTemp(NewClassOutput));
	if (!OutputNode)
	{
		return ErrorResult(FString::Printf(TEXT("Failed to add graph output: %s"), *OutputName));
	}

	MetaSoundAsset->MarkPackageDirty();

	TSharedPtr<FJsonObject> Result = SuccessResult(FString::Printf(TEXT("Added graph output: %s (%s)"), *OutputName, *DataType));
	Result->SetStringField(TEXT("output_node_id"), OutputNode->GetID().ToString());
	return Result;
}

// ============================================================================
// Preview Operations
// ============================================================================

TSharedPtr<FJsonObject> FMetaSoundEditor::PreviewMetaSound(const FString& AssetPath)
{
	UMetaSoundSource* MetaSound = LoadObject<UMetaSoundSource>(nullptr, *AssetPath);
	if (!MetaSound)
	{
		return ErrorResult(FString::Printf(TEXT("Failed to load MetaSound: %s"), *AssetPath));
	}

	if (!GEditor)
	{
		return ErrorResult(TEXT("Editor not available"));
	}

	if (GMetaSoundPreviewComp.IsValid())
	{
		GMetaSoundPreviewComp->Stop();
		GMetaSoundPreviewComp->DestroyComponent();
		GMetaSoundPreviewComp = nullptr;
	}

	UAudioComponent* NewComp = NewObject<UAudioComponent>(GetTransientPackage());
	NewComp->SetSound(MetaSound);
	NewComp->bAutoDestroy = false;
	NewComp->bIsPreviewSound = true;
	NewComp->bAllowSpatialization = false;
	NewComp->SetComponentTickEnabled(true);
	NewComp->RegisterComponentWithWorld(GEditor->GetEditorWorldContext().World());
	NewComp->Play();
	GMetaSoundPreviewComp = NewComp;

	TSharedPtr<FJsonObject> Result = SuccessResult(FString::Printf(TEXT("Playing MetaSound: %s"), *MetaSound->GetName()));
	Result->SetStringField(TEXT("asset_path"), AssetPath);
	Result->SetStringField(TEXT("status"), TEXT("playing"));
	return Result;
}

TSharedPtr<FJsonObject> FMetaSoundEditor::StopPreview()
{
	if (GMetaSoundPreviewComp.IsValid())
	{
		GMetaSoundPreviewComp->Stop();
		GMetaSoundPreviewComp->DestroyComponent();
		GMetaSoundPreviewComp = nullptr;
		return SuccessResult(TEXT("Preview stopped"));
	}

	return SuccessResult(TEXT("No preview was playing"));
}

#endif // ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
