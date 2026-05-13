// Copyright Natali Caggiano. All Rights Reserved.

#include "AudioEditor.h"

#include "MetasoundSource.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundNodeWavePlayer.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundClass.h"
#include "Sound/AmbientSound.h"
#include "Components/AudioComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/World.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "GameFramework/Actor.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Editor.h"
#include "EngineUtils.h"

// ============================================================================
// Private Helpers
// ============================================================================

TSharedPtr<FJsonObject> FAudioEditor::SuccessResult(const FString& Message)
{
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("message"), Message);
	return Result;
}

TSharedPtr<FJsonObject> FAudioEditor::ErrorResult(const FString& Message)
{
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), false);
	Result->SetStringField(TEXT("error"), Message);
	return Result;
}

// ============================================================================
// InspectSound
// ============================================================================

TSharedPtr<FJsonObject> FAudioEditor::InspectSound(const FString& AssetPath)
{
	UObject* Loaded = StaticLoadObject(UObject::StaticClass(), nullptr, *AssetPath);
	if (!Loaded)
	{
		return ErrorResult(FString::Printf(TEXT("Failed to load asset: %s"), *AssetPath));
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("asset_path"), Loaded->GetPathName());
	Result->SetStringField(TEXT("asset_name"), Loaded->GetName());

	// --- USoundWave ---
	if (USoundWave* SoundWave = Cast<USoundWave>(Loaded))
	{
		Result->SetStringField(TEXT("type"), TEXT("SoundWave"));
		Result->SetNumberField(TEXT("duration"), SoundWave->Duration);
		Result->SetNumberField(TEXT("sample_rate"), static_cast<double>(SoundWave->GetSampleRateForCurrentPlatform()));
		Result->SetNumberField(TEXT("num_channels"), SoundWave->NumChannels);
		Result->SetBoolField(TEXT("is_looping"), SoundWave->bLooping);

		if (SoundWave->SoundClassObject)
		{
			Result->SetStringField(TEXT("sound_class"), SoundWave->SoundClassObject->GetPathName());
		}

		if (SoundWave->AttenuationSettings)
		{
			Result->SetStringField(TEXT("attenuation_settings"), SoundWave->AttenuationSettings->GetPathName());
		}

		Result->SetStringField(TEXT("message"), FString::Printf(TEXT("SoundWave: %s (%.2fs, %dch, %dHz)"),
			*SoundWave->GetName(), SoundWave->Duration, SoundWave->NumChannels,
			static_cast<int32>(SoundWave->GetSampleRateForCurrentPlatform())));
		return Result;
	}

	// --- USoundCue ---
	if (USoundCue* SoundCue = Cast<USoundCue>(Loaded))
	{
		Result->SetStringField(TEXT("type"), TEXT("SoundCue"));
		Result->SetNumberField(TEXT("duration"), SoundCue->GetDuration());
		Result->SetNumberField(TEXT("volume_multiplier"), SoundCue->VolumeMultiplier);
		Result->SetNumberField(TEXT("pitch_multiplier"), SoundCue->PitchMultiplier);

		if (SoundCue->SoundClassObject)
		{
			Result->SetStringField(TEXT("sound_class"), SoundCue->SoundClassObject->GetPathName());
		}

		if (SoundCue->AttenuationSettings)
		{
			Result->SetStringField(TEXT("attenuation_settings"), SoundCue->AttenuationSettings->GetPathName());
		}

		if (SoundCue->FirstNode)
		{
			Result->SetStringField(TEXT("first_node"), SoundCue->FirstNode->GetClass()->GetName());
		}

		TArray<TSharedPtr<FJsonValue>> NodesArray;
		for (USoundNode* Node : SoundCue->AllNodes)
		{
			if (!Node) continue;

			TSharedPtr<FJsonObject> NodeJson = MakeShared<FJsonObject>();
			NodeJson->SetStringField(TEXT("class"), Node->GetClass()->GetName());

			USoundNodeWavePlayer* WavePlayer = Cast<USoundNodeWavePlayer>(Node);
			if (WavePlayer && WavePlayer->GetSoundWave())
			{
				NodeJson->SetStringField(TEXT("sound_wave"), WavePlayer->GetSoundWave()->GetPathName());
			}

			NodesArray.Add(MakeShared<FJsonValueObject>(NodeJson));
		}
		Result->SetArrayField(TEXT("nodes"), NodesArray);
		Result->SetNumberField(TEXT("node_count"), SoundCue->AllNodes.Num());

		Result->SetStringField(TEXT("message"), FString::Printf(TEXT("SoundCue: %s (%d nodes, vol=%.2f, pitch=%.2f)"),
			*SoundCue->GetName(), SoundCue->AllNodes.Num(), SoundCue->VolumeMultiplier, SoundCue->PitchMultiplier));
		return Result;
	}

	// --- USoundAttenuation ---
	if (USoundAttenuation* SoundAtten = Cast<USoundAttenuation>(Loaded))
	{
		Result->SetStringField(TEXT("type"), TEXT("SoundAttenuation"));

		const FSoundAttenuationSettings& Settings = SoundAtten->Attenuation;

		FString ShapeName;
		switch (Settings.AttenuationShape)
		{
		case EAttenuationShape::Sphere:  ShapeName = TEXT("Sphere"); break;
		case EAttenuationShape::Capsule: ShapeName = TEXT("Capsule"); break;
		case EAttenuationShape::Box:     ShapeName = TEXT("Box"); break;
		case EAttenuationShape::Cone:    ShapeName = TEXT("Cone"); break;
		default: ShapeName = TEXT("Unknown"); break;
		}
		Result->SetStringField(TEXT("shape"), ShapeName);

		Result->SetNumberField(TEXT("inner_radius"), Settings.AttenuationShapeExtents.X);
		Result->SetNumberField(TEXT("outer_radius"), Settings.AttenuationShapeExtents.X + Settings.FalloffDistance);
		Result->SetNumberField(TEXT("falloff_distance"), Settings.FalloffDistance);

		FString AlgoName;
		switch (Settings.DistanceAlgorithm)
		{
		case EAttenuationDistanceModel::Linear:       AlgoName = TEXT("Linear"); break;
		case EAttenuationDistanceModel::Logarithmic:  AlgoName = TEXT("Logarithmic"); break;
		case EAttenuationDistanceModel::Inverse:       AlgoName = TEXT("Inverse"); break;
		case EAttenuationDistanceModel::LogReverse:    AlgoName = TEXT("LogReverse"); break;
		case EAttenuationDistanceModel::NaturalSound:  AlgoName = TEXT("NaturalSound"); break;
		case EAttenuationDistanceModel::Custom:        AlgoName = TEXT("Custom"); break;
		default: AlgoName = TEXT("Unknown"); break;
		}
		Result->SetStringField(TEXT("distance_algorithm"), AlgoName);
		Result->SetNumberField(TEXT("db_attenuation_at_max"), Settings.dBAttenuationAtMax);
		Result->SetBoolField(TEXT("is_spatialized"), Settings.bSpatialize);

		FString SpatAlgo;
		switch (Settings.SpatializationAlgorithm)
		{
		case SPATIALIZATION_Default: SpatAlgo = TEXT("Default"); break;
		case SPATIALIZATION_HRTF:    SpatAlgo = TEXT("HRTF"); break;
		default: SpatAlgo = TEXT("Unknown"); break;
		}
		Result->SetStringField(TEXT("spatialization_algorithm"), SpatAlgo);

		Result->SetStringField(TEXT("message"), FString::Printf(TEXT("SoundAttenuation: %s (%s, falloff=%.0f)"),
			*SoundAtten->GetName(), *ShapeName, Settings.FalloffDistance));
		return Result;
	}

	// --- USoundClass ---
	if (USoundClass* SoundClass = Cast<USoundClass>(Loaded))
	{
		Result->SetStringField(TEXT("type"), TEXT("SoundClass"));
		Result->SetNumberField(TEXT("volume"), SoundClass->Properties.Volume);
		Result->SetNumberField(TEXT("pitch"), SoundClass->Properties.Pitch);

		TSharedPtr<FJsonObject> PropsJson = MakeShared<FJsonObject>();
		PropsJson->SetNumberField(TEXT("volume"), SoundClass->Properties.Volume);
		PropsJson->SetNumberField(TEXT("pitch"), SoundClass->Properties.Pitch);
		PropsJson->SetNumberField(TEXT("low_pass_filter_frequency"), SoundClass->Properties.LowPassFilterFrequency);
		PropsJson->SetNumberField(TEXT("attenuation_distance_scale"), SoundClass->Properties.AttenuationDistanceScale);
		Result->SetObjectField(TEXT("properties"), PropsJson);

		Result->SetStringField(TEXT("message"), FString::Printf(TEXT("SoundClass: %s (vol=%.2f, pitch=%.2f)"),
			*SoundClass->GetName(), SoundClass->Properties.Volume, SoundClass->Properties.Pitch));
		return Result;
	}

	// --- UMetaSoundSource ---
	if (UMetaSoundSource* MetaSoundSrc = Cast<UMetaSoundSource>(Loaded))
	{
		Result->SetStringField(TEXT("type"), TEXT("MetaSoundSource"));
		Result->SetNumberField(TEXT("duration"), MetaSoundSrc->GetDuration());
		Result->SetStringField(TEXT("message"), FString::Printf(TEXT("MetaSoundSource: %s (use 'metasound' tool for graph details)"),
			*MetaSoundSrc->GetName()));
		Result->SetStringField(TEXT("hint"), TEXT("Use the 'metasound' tool with operation 'inspect' or 'get_graph' for detailed MetaSound graph information."));
		return Result;
	}

	return ErrorResult(FString::Printf(TEXT("Asset is not a recognized sound type: %s (is %s)"),
		*AssetPath, *Loaded->GetClass()->GetName()));
}

// ============================================================================
// ListSounds
// ============================================================================

TSharedPtr<FJsonObject> FAudioEditor::ListSounds(const FString& FolderPath, const FString& ClassFilter, bool bRecursive)
{
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(*FolderPath));
	Filter.bRecursivePaths = bRecursive;

	if (!ClassFilter.IsEmpty())
	{
		if (ClassFilter.Equals(TEXT("SoundWave"), ESearchCase::IgnoreCase))
		{
			Filter.ClassPaths.Add(USoundWave::StaticClass()->GetClassPathName());
		}
		else if (ClassFilter.Equals(TEXT("SoundCue"), ESearchCase::IgnoreCase))
		{
			Filter.ClassPaths.Add(USoundCue::StaticClass()->GetClassPathName());
		}
		else if (ClassFilter.Equals(TEXT("SoundAttenuation"), ESearchCase::IgnoreCase))
		{
			Filter.ClassPaths.Add(USoundAttenuation::StaticClass()->GetClassPathName());
		}
		else if (ClassFilter.Equals(TEXT("SoundClass"), ESearchCase::IgnoreCase))
		{
			Filter.ClassPaths.Add(USoundClass::StaticClass()->GetClassPathName());
		}
		else if (ClassFilter.Equals(TEXT("MetaSoundSource"), ESearchCase::IgnoreCase))
		{
			Filter.ClassPaths.Add(UMetaSoundSource::StaticClass()->GetClassPathName());
		}
		else
		{
			return ErrorResult(FString::Printf(TEXT("Unknown class filter: '%s'. Valid: SoundWave, SoundCue, SoundAttenuation, SoundClass, MetaSoundSource"), *ClassFilter));
		}
	}
	else
	{
		Filter.ClassPaths.Add(USoundWave::StaticClass()->GetClassPathName());
		Filter.ClassPaths.Add(USoundCue::StaticClass()->GetClassPathName());
		Filter.ClassPaths.Add(USoundAttenuation::StaticClass()->GetClassPathName());
		Filter.ClassPaths.Add(USoundClass::StaticClass()->GetClassPathName());
		Filter.ClassPaths.Add(UMetaSoundSource::StaticClass()->GetClassPathName());
	}

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssets(Filter, AssetList);

	TArray<TSharedPtr<FJsonValue>> AssetsArray;
	for (const FAssetData& Asset : AssetList)
	{
		TSharedPtr<FJsonObject> AssetJson = MakeShared<FJsonObject>();
		AssetJson->SetStringField(TEXT("name"), Asset.AssetName.ToString());
		AssetJson->SetStringField(TEXT("path"), Asset.GetObjectPathString());
		AssetJson->SetStringField(TEXT("class"), Asset.AssetClassPath.GetAssetName().ToString());

		if (Asset.AssetClassPath == USoundWave::StaticClass()->GetClassPathName())
		{
			USoundWave* Wave = Cast<USoundWave>(Asset.GetAsset());
			if (Wave)
			{
				AssetJson->SetNumberField(TEXT("duration"), Wave->Duration);
			}
		}

		AssetsArray.Add(MakeShared<FJsonValueObject>(AssetJson));
	}

	TSharedPtr<FJsonObject> Result = SuccessResult(
		FString::Printf(TEXT("Found %d sound assets in %s"), AssetsArray.Num(), *FolderPath));
	Result->SetArrayField(TEXT("assets"), AssetsArray);
	Result->SetNumberField(TEXT("count"), AssetsArray.Num());
	return Result;
}

// ============================================================================
// GetAudioComponents
// ============================================================================

TSharedPtr<FJsonObject> FAudioEditor::GetAudioComponents(const FString& TargetPath)
{
	TArray<TSharedPtr<FJsonValue>> ComponentsArray;

	// Try as Blueprint path first
	if (TargetPath.StartsWith(TEXT("/Game/")))
	{
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *TargetPath);
		if (Blueprint && Blueprint->SimpleConstructionScript)
		{
			for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
			{
				if (!Node || !Node->ComponentClass || !Node->ComponentClass->IsChildOf(UAudioComponent::StaticClass()))
				{
					continue;
				}

				UAudioComponent* AudioComp = Cast<UAudioComponent>(Node->ComponentTemplate);
				if (!AudioComp) continue;

				TSharedPtr<FJsonObject> CompJson = MakeShared<FJsonObject>();
				CompJson->SetStringField(TEXT("component_name"), Node->GetVariableName().ToString());

				if (AudioComp->Sound)
				{
					CompJson->SetStringField(TEXT("sound_asset"), AudioComp->Sound->GetPathName());
				}

				CompJson->SetNumberField(TEXT("volume"), AudioComp->VolumeMultiplier);
				CompJson->SetNumberField(TEXT("pitch"), AudioComp->PitchMultiplier);
				CompJson->SetBoolField(TEXT("is_auto_activate"), AudioComp->bAutoActivate);

				if (AudioComp->AttenuationSettings)
				{
					CompJson->SetStringField(TEXT("attenuation_settings_path"), AudioComp->AttenuationSettings->GetPathName());
				}

				CompJson->SetBoolField(TEXT("is_spatialized"), AudioComp->bAllowSpatialization);

				ComponentsArray.Add(MakeShared<FJsonValueObject>(CompJson));
			}

			TSharedPtr<FJsonObject> Result = SuccessResult(
				FString::Printf(TEXT("Found %d audio components in Blueprint %s"), ComponentsArray.Num(), *TargetPath));
			Result->SetArrayField(TEXT("components"), ComponentsArray);
			Result->SetNumberField(TEXT("count"), ComponentsArray.Num());
			Result->SetStringField(TEXT("source"), TEXT("blueprint"));
			return Result;
		}
	}

	// Try as actor in world
	if (!GEditor)
	{
		return ErrorResult(TEXT("Editor not available"));
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return ErrorResult(TEXT("No editor world available"));
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor) continue;

		if (Actor->GetActorLabel() == TargetPath || Actor->GetName() == TargetPath)
		{
			TArray<UAudioComponent*> AudioComps;
			Actor->GetComponents<UAudioComponent>(AudioComps);

			for (UAudioComponent* AudioComp : AudioComps)
			{
				if (!AudioComp) continue;

				TSharedPtr<FJsonObject> CompJson = MakeShared<FJsonObject>();
				CompJson->SetStringField(TEXT("component_name"), AudioComp->GetName());

				if (AudioComp->Sound)
				{
					CompJson->SetStringField(TEXT("sound_asset"), AudioComp->Sound->GetPathName());
				}

				CompJson->SetNumberField(TEXT("volume"), AudioComp->VolumeMultiplier);
				CompJson->SetNumberField(TEXT("pitch"), AudioComp->PitchMultiplier);
				CompJson->SetBoolField(TEXT("is_auto_activate"), AudioComp->bAutoActivate);

				if (AudioComp->AttenuationSettings)
				{
					CompJson->SetStringField(TEXT("attenuation_settings_path"), AudioComp->AttenuationSettings->GetPathName());
				}

				CompJson->SetBoolField(TEXT("is_spatialized"), AudioComp->bAllowSpatialization);

				ComponentsArray.Add(MakeShared<FJsonValueObject>(CompJson));
			}

			TSharedPtr<FJsonObject> Result = SuccessResult(
				FString::Printf(TEXT("Found %d audio components on actor %s"), ComponentsArray.Num(), *TargetPath));
			Result->SetArrayField(TEXT("components"), ComponentsArray);
			Result->SetNumberField(TEXT("count"), ComponentsArray.Num());
			Result->SetStringField(TEXT("source"), TEXT("world_actor"));
			return Result;
		}
	}

	return ErrorResult(FString::Printf(TEXT("Could not find Blueprint or actor: %s"), *TargetPath));
}

// ============================================================================
// Write Operation Stubs (Phase 2+)
// ============================================================================

TSharedPtr<FJsonObject> FAudioEditor::AddAudioComponent(const FString& BlueprintPath, const FString& ComponentName, const FString& SoundAssetPath)
{
	UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
	if (!BP)
	{
		return ErrorResult(FString::Printf(TEXT("Failed to load Blueprint: %s"), *BlueprintPath));
	}

	if (!BP->SimpleConstructionScript)
	{
		return ErrorResult(FString::Printf(TEXT("Blueprint has no SimpleConstructionScript: %s"), *BlueprintPath));
	}

	USoundBase* SoundAsset = nullptr;
	if (!SoundAssetPath.IsEmpty())
	{
		SoundAsset = LoadObject<USoundBase>(nullptr, *SoundAssetPath);
		if (!SoundAsset)
		{
			return ErrorResult(FString::Printf(TEXT("Failed to load sound asset: %s"), *SoundAssetPath));
		}
	}

	USCS_Node* NewNode = BP->SimpleConstructionScript->CreateNode(UAudioComponent::StaticClass(), *ComponentName);
	if (!NewNode)
	{
		return ErrorResult(TEXT("Failed to create SCS node for AudioComponent"));
	}

	BP->SimpleConstructionScript->AddNode(NewNode);

	if (SoundAsset)
	{
		UAudioComponent* AudioComp = Cast<UAudioComponent>(NewNode->ComponentTemplate);
		if (AudioComp)
		{
			AudioComp->SetSound(SoundAsset);
		}
	}

	FKismetEditorUtilities::CompileBlueprint(BP);
	BP->MarkPackageDirty();

	TSharedPtr<FJsonObject> Result = SuccessResult(
		FString::Printf(TEXT("Added audio component '%s' to %s"), *ComponentName, *BlueprintPath));
	Result->SetStringField(TEXT("component_name"), ComponentName);
	Result->SetStringField(TEXT("blueprint_path"), BlueprintPath);
	if (SoundAsset)
	{
		Result->SetStringField(TEXT("sound_asset"), SoundAsset->GetPathName());
	}
	return Result;
}

TSharedPtr<FJsonObject> FAudioEditor::SetAudioProperties(const FString& TargetPath, const FString& ComponentName, const TSharedPtr<FJsonObject>& Properties)
{
	if (!Properties.IsValid())
	{
		return ErrorResult(TEXT("Properties object is required"));
	}

	UAudioComponent* AudioComp = nullptr;
	UBlueprint* BP = nullptr;

	if (TargetPath.StartsWith(TEXT("/Game/")))
	{
		BP = LoadObject<UBlueprint>(nullptr, *TargetPath);
		if (BP && BP->SimpleConstructionScript)
		{
			for (USCS_Node* Node : BP->SimpleConstructionScript->GetAllNodes())
			{
				if (!Node || !Node->ComponentClass || !Node->ComponentClass->IsChildOf(UAudioComponent::StaticClass()))
				{
					continue;
				}

				if (ComponentName.IsEmpty() || Node->GetVariableName().ToString() == ComponentName)
				{
					AudioComp = Cast<UAudioComponent>(Node->ComponentTemplate);
					break;
				}
			}

			if (!AudioComp)
			{
				return ErrorResult(FString::Printf(TEXT("No audio component '%s' found in Blueprint: %s"),
					*ComponentName, *TargetPath));
			}
		}
		else
		{
			return ErrorResult(FString::Printf(TEXT("Failed to load Blueprint or no SCS: %s"), *TargetPath));
		}
	}
	else
	{
		if (!GEditor)
		{
			return ErrorResult(TEXT("Editor not available"));
		}

		UWorld* World = GEditor->GetEditorWorldContext().World();
		if (!World)
		{
			return ErrorResult(TEXT("No editor world available"));
		}

		AActor* FoundActor = nullptr;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!Actor) continue;

			if (Actor->GetActorLabel() == TargetPath || Actor->GetName() == TargetPath)
			{
				FoundActor = Actor;
				break;
			}
		}

		if (!FoundActor)
		{
			return ErrorResult(FString::Printf(TEXT("Could not find actor: %s"), *TargetPath));
		}

		if (ComponentName.IsEmpty())
		{
			AudioComp = FoundActor->FindComponentByClass<UAudioComponent>();
		}
		else
		{
			TArray<UAudioComponent*> AudioComps;
			FoundActor->GetComponents<UAudioComponent>(AudioComps);
			for (UAudioComponent* Comp : AudioComps)
			{
				if (Comp && Comp->GetName() == ComponentName)
				{
					AudioComp = Comp;
					break;
				}
			}
		}

		if (!AudioComp)
		{
			return ErrorResult(FString::Printf(TEXT("No audio component '%s' found on actor: %s"),
				*ComponentName, *TargetPath));
		}
	}

	TArray<FString> PropertiesSet;
	double NumValue;

	if (Properties->TryGetNumberField(TEXT("volume"), NumValue))
	{
		AudioComp->VolumeMultiplier = static_cast<float>(NumValue);
		PropertiesSet.Add(FString::Printf(TEXT("volume=%.2f"), NumValue));
	}

	if (Properties->TryGetNumberField(TEXT("pitch"), NumValue))
	{
		AudioComp->PitchMultiplier = static_cast<float>(NumValue);
		PropertiesSet.Add(FString::Printf(TEXT("pitch=%.2f"), NumValue));
	}

	bool BoolValue;
	if (Properties->TryGetBoolField(TEXT("auto_activate"), BoolValue))
	{
		AudioComp->bAutoActivate = BoolValue;
		PropertiesSet.Add(FString::Printf(TEXT("auto_activate=%s"), BoolValue ? TEXT("true") : TEXT("false")));
	}

	if (Properties->TryGetBoolField(TEXT("is_spatialized"), BoolValue))
	{
		AudioComp->bIsUISound = !BoolValue;
		PropertiesSet.Add(FString::Printf(TEXT("is_spatialized=%s"), BoolValue ? TEXT("true") : TEXT("false")));
	}

	FString StringValue;
	if (Properties->TryGetStringField(TEXT("sound"), StringValue))
	{
		USoundBase* NewSound = LoadObject<USoundBase>(nullptr, *StringValue);
		if (NewSound)
		{
			AudioComp->SetSound(NewSound);
			PropertiesSet.Add(FString::Printf(TEXT("sound=%s"), *StringValue));
		}
		else
		{
			return ErrorResult(FString::Printf(TEXT("Failed to load sound asset: %s"), *StringValue));
		}
	}

	if (Properties->TryGetStringField(TEXT("attenuation_path"), StringValue))
	{
		USoundAttenuation* Attenuation = LoadObject<USoundAttenuation>(nullptr, *StringValue);
		if (Attenuation)
		{
			AudioComp->AttenuationSettings = Attenuation;
			PropertiesSet.Add(FString::Printf(TEXT("attenuation_path=%s"), *StringValue));
		}
		else
		{
			return ErrorResult(FString::Printf(TEXT("Failed to load attenuation asset: %s"), *StringValue));
		}
	}

	const TSharedPtr<FJsonObject>* AttenuationOverrides = nullptr;
	if (Properties->TryGetObjectField(TEXT("attenuation_overrides"), AttenuationOverrides) && AttenuationOverrides && *AttenuationOverrides)
	{
		AudioComp->bOverrideAttenuation = true;

		if ((*AttenuationOverrides)->TryGetNumberField(TEXT("inner_radius"), NumValue))
		{
			AudioComp->AttenuationOverrides.AttenuationShapeExtents.X = static_cast<float>(NumValue);
		}

		double FalloffValue;
		if ((*AttenuationOverrides)->TryGetNumberField(TEXT("falloff_distance"), FalloffValue))
		{
			AudioComp->AttenuationOverrides.FalloffDistance = static_cast<float>(FalloffValue);
		}

		FString FalloffModel;
		if ((*AttenuationOverrides)->TryGetStringField(TEXT("distance_algorithm"), FalloffModel))
		{
			if (FalloffModel.Equals(TEXT("Linear"), ESearchCase::IgnoreCase))
				AudioComp->AttenuationOverrides.DistanceAlgorithm = EAttenuationDistanceModel::Linear;
			else if (FalloffModel.Equals(TEXT("Logarithmic"), ESearchCase::IgnoreCase))
				AudioComp->AttenuationOverrides.DistanceAlgorithm = EAttenuationDistanceModel::Logarithmic;
			else if (FalloffModel.Equals(TEXT("Inverse"), ESearchCase::IgnoreCase))
				AudioComp->AttenuationOverrides.DistanceAlgorithm = EAttenuationDistanceModel::Inverse;
			else if (FalloffModel.Equals(TEXT("NaturalSound"), ESearchCase::IgnoreCase))
				AudioComp->AttenuationOverrides.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
			else if (FalloffModel.Equals(TEXT("LogReverse"), ESearchCase::IgnoreCase))
				AudioComp->AttenuationOverrides.DistanceAlgorithm = EAttenuationDistanceModel::LogReverse;
			else if (FalloffModel.Equals(TEXT("Custom"), ESearchCase::IgnoreCase))
				AudioComp->AttenuationOverrides.DistanceAlgorithm = EAttenuationDistanceModel::Custom;
		}

		PropertiesSet.Add(TEXT("attenuation_overrides"));
	}

	if (BP)
	{
		FKismetEditorUtilities::CompileBlueprint(BP);
		BP->MarkPackageDirty();
	}

	TSharedPtr<FJsonObject> Result = SuccessResult(
		FString::Printf(TEXT("Set %d properties on audio component in %s"), PropertiesSet.Num(), *TargetPath));
	TArray<TSharedPtr<FJsonValue>> PropsArray;
	for (const FString& Prop : PropertiesSet)
	{
		PropsArray.Add(MakeShared<FJsonValueString>(Prop));
	}
	Result->SetArrayField(TEXT("properties_set"), PropsArray);
	Result->SetStringField(TEXT("target_path"), TargetPath);
	return Result;
}

TSharedPtr<FJsonObject> FAudioEditor::CreateSoundCue(const FString& PackagePath, const FString& Name, const FString& SoundWavePath)
{
	USoundWave* Wave = LoadObject<USoundWave>(nullptr, *SoundWavePath);
	if (!Wave)
	{
		return ErrorResult(FString::Printf(TEXT("Failed to load SoundWave: %s"), *SoundWavePath));
	}

	FString FullPath = PackagePath / Name;
	UPackage* Package = CreatePackage(*FullPath);
	if (!Package)
	{
		return ErrorResult(FString::Printf(TEXT("Failed to create package: %s"), *FullPath));
	}

	USoundCue* SoundCue = NewObject<USoundCue>(Package, *Name, RF_Public | RF_Standalone);
	if (!SoundCue)
	{
		return ErrorResult(TEXT("Failed to create USoundCue object"));
	}

	USoundNodeWavePlayer* WavePlayer = NewObject<USoundNodeWavePlayer>(SoundCue);
	WavePlayer->SetSoundWave(Wave);
	SoundCue->FirstNode = WavePlayer;
	SoundCue->AllNodes.Add(WavePlayer);

	FAssetRegistryModule::AssetCreated(SoundCue);
	SoundCue->MarkPackageDirty();

	TSharedPtr<FJsonObject> Result = SuccessResult(
		FString::Printf(TEXT("Created SoundCue: %s from SoundWave: %s"), *FullPath, *SoundWavePath));
	Result->SetStringField(TEXT("asset_path"), SoundCue->GetPathName());
	Result->SetStringField(TEXT("sound_wave"), Wave->GetPathName());
	return Result;
}

TSharedPtr<FJsonObject> FAudioEditor::CreateAttenuation(const FString& PackagePath, const FString& Name, const TSharedPtr<FJsonObject>& Settings)
{
	if (!Settings.IsValid())
	{
		return ErrorResult(TEXT("Settings object is required"));
	}

	FString FullPath = PackagePath / Name;
	UPackage* Package = CreatePackage(*FullPath);
	if (!Package)
	{
		return ErrorResult(FString::Printf(TEXT("Failed to create package: %s"), *FullPath));
	}

	USoundAttenuation* Attenuation = NewObject<USoundAttenuation>(Package, *Name, RF_Public | RF_Standalone);
	if (!Attenuation)
	{
		return ErrorResult(TEXT("Failed to create USoundAttenuation object"));
	}

	FSoundAttenuationSettings& Atten = Attenuation->Attenuation;

	FString StrVal;
	double NumValue;
	bool BoolValue;

	if (Settings->TryGetStringField(TEXT("shape"), StrVal) || Settings->TryGetStringField(TEXT("attenuation_shape"), StrVal))
	{
		if (StrVal.Equals(TEXT("Sphere"), ESearchCase::IgnoreCase))
			Atten.AttenuationShape = EAttenuationShape::Sphere;
		else if (StrVal.Equals(TEXT("Capsule"), ESearchCase::IgnoreCase))
			Atten.AttenuationShape = EAttenuationShape::Capsule;
		else if (StrVal.Equals(TEXT("Box"), ESearchCase::IgnoreCase))
			Atten.AttenuationShape = EAttenuationShape::Box;
		else if (StrVal.Equals(TEXT("Cone"), ESearchCase::IgnoreCase))
			Atten.AttenuationShape = EAttenuationShape::Cone;
	}

	if (Settings->TryGetNumberField(TEXT("inner_radius"), NumValue))
	{
		Atten.AttenuationShapeExtents.X = static_cast<float>(NumValue);
	}

	if (Settings->TryGetNumberField(TEXT("falloff_distance"), NumValue))
	{
		Atten.FalloffDistance = static_cast<float>(NumValue);
	}
	else if (Settings->TryGetNumberField(TEXT("outer_radius"), NumValue))
	{
		Atten.FalloffDistance = FMath::Max(0.f, static_cast<float>(NumValue) - Atten.AttenuationShapeExtents.X);
	}

	if (Settings->TryGetStringField(TEXT("distance_algorithm"), StrVal))
	{
		if (StrVal.Equals(TEXT("Linear"), ESearchCase::IgnoreCase))
			Atten.DistanceAlgorithm = EAttenuationDistanceModel::Linear;
		else if (StrVal.Equals(TEXT("Logarithmic"), ESearchCase::IgnoreCase))
			Atten.DistanceAlgorithm = EAttenuationDistanceModel::Logarithmic;
		else if (StrVal.Equals(TEXT("NaturalSound"), ESearchCase::IgnoreCase))
			Atten.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
		else if (StrVal.Equals(TEXT("Custom"), ESearchCase::IgnoreCase))
			Atten.DistanceAlgorithm = EAttenuationDistanceModel::Custom;
		else if (StrVal.Equals(TEXT("LogReverse"), ESearchCase::IgnoreCase))
			Atten.DistanceAlgorithm = EAttenuationDistanceModel::LogReverse;
		else if (StrVal.Equals(TEXT("Inverse"), ESearchCase::IgnoreCase))
			Atten.DistanceAlgorithm = EAttenuationDistanceModel::Inverse;
	}

	if (Settings->TryGetNumberField(TEXT("db_attenuation_at_max"), NumValue))
	{
		Atten.dBAttenuationAtMax = static_cast<float>(NumValue);
	}

	if (Settings->TryGetBoolField(TEXT("is_spatialized"), BoolValue))
	{
		Atten.bSpatialize = BoolValue;
	}

	if (Settings->TryGetStringField(TEXT("spatialization_algorithm"), StrVal))
	{
		if (StrVal.Equals(TEXT("Default"), ESearchCase::IgnoreCase) || StrVal.Equals(TEXT("Panning"), ESearchCase::IgnoreCase))
			Atten.SpatializationAlgorithm = SPATIALIZATION_Default;
		else if (StrVal.Equals(TEXT("HRTF"), ESearchCase::IgnoreCase) || StrVal.Equals(TEXT("Binaural"), ESearchCase::IgnoreCase))
			Atten.SpatializationAlgorithm = SPATIALIZATION_HRTF;
	}

	FAssetRegistryModule::AssetCreated(Attenuation);
	Attenuation->MarkPackageDirty();

	TSharedPtr<FJsonObject> Result = SuccessResult(
		FString::Printf(TEXT("Created SoundAttenuation: %s"), *FullPath));
	Result->SetStringField(TEXT("asset_path"), Attenuation->GetPathName());
	return Result;
}

TSharedPtr<FJsonObject> FAudioEditor::SpawnAmbientSound(UWorld* World, const FString& SoundAssetPath, const FVector& Location, const FString& Label)
{
	if (!World)
	{
		if (!GEditor)
		{
			return ErrorResult(TEXT("Editor not available"));
		}
		World = GEditor->GetEditorWorldContext().World();
	}

	if (!World)
	{
		return ErrorResult(TEXT("No editor world available"));
	}

	USoundBase* Sound = LoadObject<USoundBase>(nullptr, *SoundAssetPath);
	if (!Sound)
	{
		return ErrorResult(FString::Printf(TEXT("Failed to load sound asset: %s"), *SoundAssetPath));
	}

	FString ActorLabel = Label.IsEmpty() ? TEXT("AmbientSound") : Label;

	FActorSpawnParameters SpawnParams;
	AAmbientSound* AmbientActor = World->SpawnActor<AAmbientSound>(Location, FRotator::ZeroRotator, SpawnParams);
	if (!AmbientActor)
	{
		return ErrorResult(TEXT("Failed to spawn AAmbientSound actor"));
	}

	UAudioComponent* AudioComp = AmbientActor->GetAudioComponent();
	if (AudioComp)
	{
		AudioComp->SetSound(Sound);
	}

	AmbientActor->SetActorLabel(ActorLabel);
	World->MarkPackageDirty();

	TSharedPtr<FJsonObject> Result = SuccessResult(
		FString::Printf(TEXT("Spawned AmbientSound '%s' at (%.1f, %.1f, %.1f)"),
			*ActorLabel, Location.X, Location.Y, Location.Z));
	Result->SetStringField(TEXT("actor_name"), AmbientActor->GetName());
	Result->SetStringField(TEXT("actor_label"), ActorLabel);

	TSharedPtr<FJsonObject> LocationJson = MakeShared<FJsonObject>();
	LocationJson->SetNumberField(TEXT("x"), Location.X);
	LocationJson->SetNumberField(TEXT("y"), Location.Y);
	LocationJson->SetNumberField(TEXT("z"), Location.Z);
	Result->SetObjectField(TEXT("location"), LocationJson);
	Result->SetStringField(TEXT("sound_asset"), Sound->GetPathName());
	return Result;
}
