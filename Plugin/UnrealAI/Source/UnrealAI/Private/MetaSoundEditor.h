// Copyright Natali Caggiano. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * Utility class for MetaSound asset inspection and editing.
 * Covers: inspect MetaSound sources, list registered nodes, get graph structure,
 * create/modify MetaSound graphs.
 *
 * All methods are static, return JSON, and contain no MCP/JSON dispatch logic.
 */
class FMetaSoundEditor
{
public:
	// ===== Read Operations =====

	/** Inspect a MetaSound source asset — metadata, I/O, node/edge counts. */
	static TSharedPtr<FJsonObject> InspectMetaSound(const FString& AssetPath);

	/** List registered MetaSound node classes, optionally filtered by name/category. */
	static TSharedPtr<FJsonObject> ListRegisteredNodes(const FString& NameFilter, const FString& CategoryFilter, int32 MaxResults);

	/** Get full graph structure of a MetaSound — nodes, edges, inputs, outputs. */
	static TSharedPtr<FJsonObject> GetMetaSoundGraph(const FString& AssetPath);

	// ===== Write Operations =====

	/** Create a new MetaSound source asset. */
	static TSharedPtr<FJsonObject> CreateMetaSound(const FString& PackagePath, const FString& Name, const FString& OutputFormat);

	/** Add a node to a MetaSound graph. */
	static TSharedPtr<FJsonObject> AddNode(const FString& AssetPath, const FString& NodeClassName, const FString& NodeNamespace);

	/** Remove a node from a MetaSound graph by GUID. */
	static TSharedPtr<FJsonObject> RemoveNode(const FString& AssetPath, const FString& NodeGuid);

	/** Connect two nodes in a MetaSound graph. */
	static TSharedPtr<FJsonObject> ConnectNodes(const FString& AssetPath, const FString& FromNodeGuid, const FString& FromPinName, const FString& ToNodeGuid, const FString& ToPinName);

	/** Disconnect two nodes in a MetaSound graph. */
	static TSharedPtr<FJsonObject> DisconnectNodes(const FString& AssetPath, const FString& FromNodeGuid, const FString& FromPinName, const FString& ToNodeGuid, const FString& ToPinName);

	/** Set a default value on a node input. */
	static TSharedPtr<FJsonObject> SetInputDefault(const FString& AssetPath, const FString& NodeGuid, const FString& InputName, const FString& Value, const FString& DataType);

	/** Add a graph-level input to a MetaSound. */
	static TSharedPtr<FJsonObject> AddGraphInput(const FString& AssetPath, const FString& InputName, const FString& DataType, const FString& DefaultValue);

	/** Add a graph-level output to a MetaSound. */
	static TSharedPtr<FJsonObject> AddGraphOutput(const FString& AssetPath, const FString& OutputName, const FString& DataType);

	/** Preview (play) a MetaSound in the editor. */
	static TSharedPtr<FJsonObject> PreviewMetaSound(const FString& AssetPath);

	/** Stop any currently playing MetaSound preview. */
	static TSharedPtr<FJsonObject> StopPreview();

private:
	static TSharedPtr<FJsonObject> SuccessResult(const FString& Message);
	static TSharedPtr<FJsonObject> ErrorResult(const FString& Message);
};
