// Copyright Natali Caggiano. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class UWorld;

/**
 * Utility class for audio asset inspection and editing.
 * Covers: inspect sounds, list sounds, get audio components, create/modify audio.
 *
 * All methods are static, return JSON, and contain no MCP/JSON dispatch logic.
 * Read operations implemented in Phase 1; write operations stubbed for Phase 2+.
 */
class FAudioEditor
{
public:
	// ===== Read Operations =====

	/** Inspect a sound asset (SoundWave, SoundCue, SoundAttenuation, SoundClass). */
	static TSharedPtr<FJsonObject> InspectSound(const FString& AssetPath);

	/** List sound assets in a folder, optionally filtered by class. */
	static TSharedPtr<FJsonObject> ListSounds(const FString& FolderPath, const FString& ClassFilter, bool bRecursive);

	/** Get audio components from a Blueprint or world actor. */
	static TSharedPtr<FJsonObject> GetAudioComponents(const FString& TargetPath);

	// ===== Write Operations (Phase 2+) =====

	/** Add an audio component to a Blueprint. */
	static TSharedPtr<FJsonObject> AddAudioComponent(const FString& BlueprintPath, const FString& ComponentName, const FString& SoundAssetPath);

	/** Set properties on an audio component. */
	static TSharedPtr<FJsonObject> SetAudioProperties(const FString& TargetPath, const FString& ComponentName, const TSharedPtr<FJsonObject>& Properties);

	/** Create a new SoundCue asset from a SoundWave. */
	static TSharedPtr<FJsonObject> CreateSoundCue(const FString& PackagePath, const FString& Name, const FString& SoundWavePath);

	/** Create a new SoundAttenuation asset. */
	static TSharedPtr<FJsonObject> CreateAttenuation(const FString& PackagePath, const FString& Name, const TSharedPtr<FJsonObject>& Settings);

	/** Spawn an AmbientSound actor in the world. */
	static TSharedPtr<FJsonObject> SpawnAmbientSound(UWorld* World, const FString& SoundAssetPath, const FVector& Location, const FString& Label);

private:
	static TSharedPtr<FJsonObject> SuccessResult(const FString& Message);
	static TSharedPtr<FJsonObject> ErrorResult(const FString& Message);
};
