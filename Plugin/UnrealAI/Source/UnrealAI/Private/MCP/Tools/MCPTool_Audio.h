// Copyright Natali Caggiano. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MCP/MCPToolBase.h"

/**
 * MCP Tool: Audio asset inspection and editing.
 *
 * Read Operations:
 *   - 'inspect': Detailed info about a sound asset (SoundWave, SoundCue, SoundAttenuation, SoundClass)
 *   - 'list_sounds': Find sound assets in a content folder
 *   - 'get_audio_components': List audio components on a Blueprint or world actor
 *
 * Write Operations:
 *   - 'add_audio_component': Add an audio component to a Blueprint
 *   - 'set_audio_properties': Modify audio component properties
 *   - 'create_sound_cue': Create a SoundCue from a SoundWave
 *   - 'create_attenuation': Create a SoundAttenuation asset
 *   - 'spawn_ambient_sound': Spawn an AmbientSound actor in the world
 */
class FMCPTool_Audio : public FMCPToolBase
{
public:
	virtual FMCPToolInfo GetInfo() const override
	{
		FMCPToolInfo Info;
		Info.Name = TEXT("audio");
		Info.Description = TEXT(
			"Audio asset inspection and editing tool.\n\n"
			"READ OPERATIONS:\n"
			"- 'inspect': Detailed info about a sound asset.\n"
			"  Params: asset_path (required)\n"
			"  Returns type-specific data for SoundWave (duration, sample_rate, num_channels, is_looping, sound_class, attenuation, subtitle),\n"
			"  SoundCue (nodes, volume/pitch multipliers, attenuation, sound_class),\n"
			"  SoundAttenuation (shape, radii, falloff, distance_algorithm, spatialization),\n"
			"  SoundClass (volume, pitch, properties).\n"
			"  Also recognizes MetaSoundSource assets (returns basic info + hint to use metasound tool).\n\n"
			"- 'list_sounds': Find sound assets in a content folder.\n"
			"  Params: folder_path (required), class_filter (optional: SoundWave, SoundCue, SoundAttenuation, SoundClass, MetaSoundSource), recursive (optional, default false)\n\n"
			"- 'get_audio_components': List audio components on a Blueprint or world actor.\n"
			"  Params: asset_path (required — Blueprint path or actor name/label)\n\n"
			"WRITE OPERATIONS:\n"
			"- 'add_audio_component': Add audio component to a Blueprint.\n"
			"  Params: blueprint_path, component_name, sound_asset_path\n\n"
			"- 'set_audio_properties': Set audio component properties.\n"
			"  Params: asset_path, component_name, properties (object with volume, pitch, auto_activate, etc.)\n\n"
			"- 'create_sound_cue': Create a SoundCue from a SoundWave.\n"
			"  Params: package_path, name, sound_wave_path\n\n"
			"- 'create_attenuation': Create a SoundAttenuation asset.\n"
			"  Params: package_path, name, plus shape/radius/falloff settings\n\n"
			"- 'spawn_ambient_sound': Spawn an AmbientSound actor.\n"
			"  Params: sound_asset_path, location (object {x,y,z}), label (optional)\n\n"
			"For MetaSound graph manipulation (nodes, connections, parameters), use the dedicated metasound tool."
		);
		Info.Parameters = {
			FMCPToolParameter(TEXT("operation"), TEXT("string"), TEXT("Operation: inspect, list_sounds, get_audio_components, add_audio_component, set_audio_properties, create_sound_cue, create_attenuation, spawn_ambient_sound"), true),
			FMCPToolParameter(TEXT("asset_path"), TEXT("string"), TEXT("Sound asset path or Blueprint/actor path")),
			FMCPToolParameter(TEXT("folder_path"), TEXT("string"), TEXT("Content folder path (for list_sounds)")),
			FMCPToolParameter(TEXT("class_filter"), TEXT("string"), TEXT("Class filter: SoundWave, SoundCue, SoundAttenuation, SoundClass, MetaSoundSource (for list_sounds)")),
			FMCPToolParameter(TEXT("recursive"), TEXT("boolean"), TEXT("Search subfolders (for list_sounds)"), false, TEXT("false")),
			FMCPToolParameter(TEXT("blueprint_path"), TEXT("string"), TEXT("Blueprint asset path (for add_audio_component)")),
			FMCPToolParameter(TEXT("component_name"), TEXT("string"), TEXT("Audio component name")),
			FMCPToolParameter(TEXT("sound_asset_path"), TEXT("string"), TEXT("Sound asset path (for add_audio_component, spawn_ambient_sound)")),
			FMCPToolParameter(TEXT("package_path"), TEXT("string"), TEXT("Package path for new asset")),
			FMCPToolParameter(TEXT("name"), TEXT("string"), TEXT("Asset name (for create operations)")),
			FMCPToolParameter(TEXT("sound_wave_path"), TEXT("string"), TEXT("SoundWave path (for create_sound_cue)")),
			FMCPToolParameter(TEXT("properties"), TEXT("object"), TEXT("Properties object for set_audio_properties")),
			FMCPToolParameter(TEXT("location"), TEXT("object"), TEXT("World location {x, y, z} (for spawn_ambient_sound)")),
			FMCPToolParameter(TEXT("label"), TEXT("string"), TEXT("Actor label (for spawn_ambient_sound)")),
			FMCPToolParameter(TEXT("attenuation_shape"), TEXT("string"), TEXT("Attenuation shape: Sphere, Capsule, Box, Cone (for create_attenuation)")),
			FMCPToolParameter(TEXT("inner_radius"), TEXT("number"), TEXT("Inner radius (for create_attenuation)")),
			FMCPToolParameter(TEXT("outer_radius"), TEXT("number"), TEXT("Outer radius (for create_attenuation)")),
			FMCPToolParameter(TEXT("falloff_distance"), TEXT("number"), TEXT("Falloff distance (for create_attenuation)")),
			FMCPToolParameter(TEXT("distance_algorithm"), TEXT("string"), TEXT("Distance algorithm: Linear, Logarithmic, NaturalSound, Custom, LogReverse (for create_attenuation)"))
		};
		Info.Annotations = FMCPToolAnnotations::Modifying();
		return Info;
	}

	virtual FMCPToolResult Execute(const TSharedRef<FJsonObject>& Params) override;

private:
	// Read operations
	FMCPToolResult HandleInspect(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleListSounds(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleGetAudioComponents(const TSharedRef<FJsonObject>& Params);

	// Write operations (stubs)
	FMCPToolResult HandleAddAudioComponent(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleSetAudioProperties(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleCreateSoundCue(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleCreateAttenuation(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleSpawnAmbientSound(const TSharedRef<FJsonObject>& Params);
};
