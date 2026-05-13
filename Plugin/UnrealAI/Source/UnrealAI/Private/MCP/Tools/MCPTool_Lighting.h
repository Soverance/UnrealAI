// Copyright Natali Caggiano. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MCP/MCPToolBase.h"

/**
 * MCP Tool: Lighting, atmosphere, and post-processing operations.
 *
 * Read Operations:
 *   - 'inspect_light': Inspect a light/atmosphere/post-process actor properties
 *   - 'list_lights': List all lighting actors in the current level
 *
 * Write Operations (Light):
 *   - 'spawn_light': Spawn a new light actor in the level
 *   - 'set_light_properties': Set properties on a standard light actor
 *   - 'set_sky_light': Set properties on a sky light actor
 *   - 'set_sky_atmosphere': Set properties on a sky atmosphere actor
 *   - 'set_fog': Set properties on an exponential height fog actor
 *
 * Post-Process Operations:
 *   - 'inspect_post_process': Inspect post-process volume settings
 *   - 'set_post_process': Set post-process volume settings
 *   - 'spawn_post_process_volume': Spawn a new post-process volume
 */
class FMCPTool_Lighting : public FMCPToolBase
{
public:
	virtual FMCPToolInfo GetInfo() const override;
	virtual FMCPToolResult Execute(const TSharedRef<FJsonObject>& Params) override;

private:
	FMCPToolResult HandleInspectLight(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleListLights(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleSpawnLight(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleSetLightProperties(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleSetSkyLight(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleSetSkyAtmosphere(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleSetFog(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleInspectPostProcess(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleSetPostProcess(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleSpawnPostProcessVolume(const TSharedRef<FJsonObject>& Params);
};
