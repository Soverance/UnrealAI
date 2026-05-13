// Copyright Natali Caggiano. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class UWorld;

class FLightingEditor
{
public:
	// ===== Read Operations =====

	static TSharedPtr<FJsonObject> InspectLight(UWorld* World, const FString& ActorNameOrLabel);
	static TSharedPtr<FJsonObject> ListLights(UWorld* World, const FString& TypeFilter);

	// ===== Write Operations (Light) =====

	static TSharedPtr<FJsonObject> SpawnLight(UWorld* World, const FString& LightType, const FVector& Location,
		const FRotator& Rotation, const FString& Label, const TSharedPtr<FJsonObject>& Properties);

	static TSharedPtr<FJsonObject> SetLightProperties(UWorld* World, const FString& ActorNameOrLabel,
		const TSharedPtr<FJsonObject>& Properties);

	static TSharedPtr<FJsonObject> SetSkyLight(UWorld* World, const FString& ActorNameOrLabel,
		const TSharedPtr<FJsonObject>& Properties);

	static TSharedPtr<FJsonObject> SetSkyAtmosphere(UWorld* World, const FString& ActorNameOrLabel,
		const TSharedPtr<FJsonObject>& Properties);

	static TSharedPtr<FJsonObject> SetFog(UWorld* World, const FString& ActorNameOrLabel,
		const TSharedPtr<FJsonObject>& Properties);

	// ===== Post-Process Operations =====

	static TSharedPtr<FJsonObject> InspectPostProcess(UWorld* World, const FString& ActorNameOrLabel);

	static TSharedPtr<FJsonObject> SetPostProcess(UWorld* World, const FString& ActorNameOrLabel,
		const TSharedPtr<FJsonObject>& Settings);

	static TSharedPtr<FJsonObject> SpawnPostProcessVolume(UWorld* World, const FVector& Location,
		const FVector& Extent, bool bInfinite, const FString& Label, const TSharedPtr<FJsonObject>& Settings);

private:
	static TSharedPtr<FJsonObject> SuccessResult(const FString& Message);
	static TSharedPtr<FJsonObject> ErrorResult(const FString& Message);
};
