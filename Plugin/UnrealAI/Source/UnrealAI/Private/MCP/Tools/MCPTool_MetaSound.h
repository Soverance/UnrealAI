// Copyright Natali Caggiano. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MCP/MCPToolBase.h"

/**
 * MCP Tool: MetaSound graph inspection and editing.
 *
 * Read Operations:
 *   - 'inspect': Inspect a MetaSound source asset
 *   - 'list_nodes': List available MetaSound node types from the registry
 *   - 'get_graph': Get complete graph structure (nodes, edges, inputs, outputs)
 *
 * Write Operations:
 *   - 'create': Create a new MetaSound source asset
 *   - 'add_node': Add a node to the graph
 *   - 'remove_node': Remove a node and its connections
 *   - 'connect': Connect two node pins
 *   - 'disconnect': Remove a connection
 *   - 'set_input_default': Set a default value on a node input
 *   - 'add_graph_input': Add an input to the MetaSound interface
 *   - 'add_graph_output': Add an output to the MetaSound interface
 *   - 'preview': Play a MetaSound in the editor
 *   - 'stop_preview': Stop any playing MetaSound preview
 */
class FMCPTool_MetaSound : public FMCPToolBase
{
public:
	virtual FMCPToolInfo GetInfo() const override
	{
		FMCPToolInfo Info;
		Info.Name = TEXT("metasound");
		Info.Description = TEXT(
			"MetaSound graph inspection and editing tool.\n\n"
			"READ OPERATIONS:\n"
			"- 'inspect': Inspect a MetaSound source asset.\n"
			"  Params: asset_path (required)\n"
			"  Returns: type, duration, output_format, node_count, edge_count, graph_inputs, graph_outputs, interfaces.\n\n"
			"- 'list_nodes': List available MetaSound node types from the registry.\n"
			"  Params: name_filter (optional), category_filter (optional), max_results (optional, default 50)\n"
			"  Returns: array of available node classes.\n\n"
			"- 'get_graph': Get complete graph structure.\n"
			"  Params: asset_path (required)\n"
			"  Returns: nodes (with IDs, classes, inputs, outputs, defaults), edges (connections), graph_inputs, graph_outputs.\n\n"
			"WRITE OPERATIONS:\n"
			"- 'create': Create a new MetaSound source asset.\n"
			"  Params: package_path (required), name (required), output_format (optional: Mono, Stereo)\n"
			"  Returns: asset_path.\n\n"
			"- 'add_node': Add a node to the graph.\n"
			"  Params: asset_path (required), node_class_name (required), node_namespace (optional)\n"
			"  Returns: node_id.\n\n"
			"- 'remove_node': Remove a node and its connections.\n"
			"  Params: asset_path (required), node_id (required, GUID string)\n"
			"  Returns: success.\n\n"
			"- 'connect': Connect two node pins.\n"
			"  Params: asset_path (required), from_node_id (required), from_pin (required), to_node_id (required), to_pin (required)\n"
			"  Returns: success.\n\n"
			"- 'disconnect': Remove a connection.\n"
			"  Params: asset_path (required), from_node_id (required), from_pin (required), to_node_id (required), to_pin (required)\n"
			"  Returns: success.\n\n"
			"- 'set_input_default': Set a default value on a node input.\n"
			"  Params: asset_path (required), node_id (required), input_name (required), value (required), data_type (optional: float, int32, bool, string)\n"
			"  Returns: success.\n\n"
			"- 'add_graph_input': Add an input to the MetaSound interface.\n"
			"  Params: asset_path (required), input_name (required), data_type (required), default_value (optional)\n"
			"  Returns: node_id of the new input node.\n\n"
			"- 'add_graph_output': Add an output to the MetaSound interface.\n"
			"  Params: asset_path (required), output_name (required), data_type (required)\n"
			"  Returns: node_id of the new output node.\n\n"
			"- 'preview': Play a MetaSound in the editor.\n"
			"  Params: asset_path (required)\n"
			"  Returns: success.\n\n"
			"- 'stop_preview': Stop any playing MetaSound preview.\n"
			"  Returns: success."
		);
		Info.Parameters = {
			FMCPToolParameter(TEXT("operation"), TEXT("string"), TEXT("Operation: inspect, list_nodes, get_graph, create, add_node, remove_node, connect, disconnect, set_input_default, add_graph_input, add_graph_output, preview, stop_preview"), true),
			FMCPToolParameter(TEXT("asset_path"), TEXT("string"), TEXT("MetaSound source asset path")),
			FMCPToolParameter(TEXT("package_path"), TEXT("string"), TEXT("Package path for create operation")),
			FMCPToolParameter(TEXT("name"), TEXT("string"), TEXT("Asset name for create")),
			FMCPToolParameter(TEXT("output_format"), TEXT("string"), TEXT("Output format: Mono, Stereo (for create)")),
			FMCPToolParameter(TEXT("node_class_name"), TEXT("string"), TEXT("Node class to add")),
			FMCPToolParameter(TEXT("node_namespace"), TEXT("string"), TEXT("Node namespace")),
			FMCPToolParameter(TEXT("node_id"), TEXT("string"), TEXT("Node GUID")),
			FMCPToolParameter(TEXT("from_node_id"), TEXT("string"), TEXT("Source node GUID")),
			FMCPToolParameter(TEXT("from_pin"), TEXT("string"), TEXT("Source pin name")),
			FMCPToolParameter(TEXT("to_node_id"), TEXT("string"), TEXT("Target node GUID")),
			FMCPToolParameter(TEXT("to_pin"), TEXT("string"), TEXT("Target pin name")),
			FMCPToolParameter(TEXT("input_name"), TEXT("string"), TEXT("Input pin name")),
			FMCPToolParameter(TEXT("output_name"), TEXT("string"), TEXT("Output pin name")),
			FMCPToolParameter(TEXT("value"), TEXT("string"), TEXT("Default value")),
			FMCPToolParameter(TEXT("data_type"), TEXT("string"), TEXT("Data type name (float, int32, bool, string, etc.)")),
			FMCPToolParameter(TEXT("default_value"), TEXT("string"), TEXT("Default value for graph input")),
			FMCPToolParameter(TEXT("name_filter"), TEXT("string"), TEXT("Filter for list_nodes")),
			FMCPToolParameter(TEXT("category_filter"), TEXT("string"), TEXT("Category filter for list_nodes")),
			FMCPToolParameter(TEXT("max_results"), TEXT("integer"), TEXT("Max results for list_nodes"), false, TEXT("50"))
		};
		Info.Annotations = FMCPToolAnnotations::Modifying();
		return Info;
	}

	virtual FMCPToolResult Execute(const TSharedRef<FJsonObject>& Params) override;

private:
	// Read operations
	FMCPToolResult HandleInspect(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleListNodes(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleGetGraph(const TSharedRef<FJsonObject>& Params);

	// Write operations
	FMCPToolResult HandleCreate(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleAddNode(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleRemoveNode(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleConnect(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleDisconnect(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleSetInputDefault(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleAddGraphInput(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleAddGraphOutput(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandlePreview(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleStopPreview(const TSharedRef<FJsonObject>& Params);
};
