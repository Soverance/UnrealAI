// Copyright Natali Caggiano. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MCP/MCPToolBase.h"

class UNiagaraSystem;
struct FNiagaraVariable;
struct FNiagaraTypeDefinition;

/**
 * MCP Tool: Niagara particle system read operations.
 *
 * Read Operations:
 *   - 'inspect': Full read of a Niagara system — emitters, renderers, parameters
 *   - 'list': Find Niagara system assets in a content folder
 *   - 'get_parameters': Get user-exposed parameters for a system
 *   - 'get_emitter_info': Detailed info for a single emitter
 *
 * Write Operations:
 *   - 'set_parameter': Set a user-exposed parameter value on a Niagara system asset
 *   - 'spawn_system': Spawn a Niagara system in the current level
 */
class FMCPTool_Niagara : public FMCPToolBase
{
public:
	virtual FMCPToolInfo GetInfo() const override;
	virtual FMCPToolResult Execute(const TSharedRef<FJsonObject>& Params) override;

private:
	FMCPToolResult HandleInspect(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleList(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleGetParameters(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleGetEmitterInfo(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleSetParameter(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleSpawnSystem(const TSharedRef<FJsonObject>& Params);

	TSharedPtr<FJsonObject> BuildSystemInfoJson(UNiagaraSystem* NiagaraSystem);
	TSharedPtr<FJsonObject> BuildEmitterInfoJson(const struct FNiagaraEmitterHandle& Handle);
	TSharedPtr<FJsonObject> BuildRendererInfoJson(const class UNiagaraRendererProperties* Renderer);
	TSharedPtr<FJsonObject> BuildParameterInfoJson(const FNiagaraVariable& Variable, const uint8* ValueData);
	FString NiagaraTypeToString(const FNiagaraTypeDefinition& TypeDef) const;
};
