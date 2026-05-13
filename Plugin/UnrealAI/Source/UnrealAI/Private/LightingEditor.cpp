// Copyright Natali Caggiano. All Rights Reserved.

#include "LightingEditor.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Engine/DirectionalLight.h"
#include "Engine/RectLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/TextureCube.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Dom/JsonValue.h"
#include "Engine/Texture.h"

namespace
{
	AActor* FindLightingActorByNameOrLabel(UWorld* World, const FString& NameOrLabel)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (Actor && (Actor->GetActorLabel() == NameOrLabel || Actor->GetName() == NameOrLabel))
			{
				return Actor;
			}
		}
		return nullptr;
	}

	void AddCommonLightProperties(TSharedPtr<FJsonObject>& OutJson, ULightComponent* LightComp)
	{
		OutJson->SetNumberField(TEXT("intensity"), LightComp->Intensity);

		TSharedPtr<FJsonObject> ColorObj = MakeShared<FJsonObject>();
		ColorObj->SetNumberField(TEXT("r"), LightComp->LightColor.R);
		ColorObj->SetNumberField(TEXT("g"), LightComp->LightColor.G);
		ColorObj->SetNumberField(TEXT("b"), LightComp->LightColor.B);
		OutJson->SetObjectField(TEXT("light_color"), ColorObj);

		OutJson->SetNumberField(TEXT("temperature"), LightComp->Temperature);
		OutJson->SetBoolField(TEXT("use_temperature"), LightComp->bUseTemperature);
		OutJson->SetBoolField(TEXT("cast_shadows"), LightComp->CastShadows != 0);
		OutJson->SetBoolField(TEXT("affects_world"), LightComp->bAffectsWorld != 0);
	}

	void AddActorTransform(TSharedPtr<FJsonObject>& OutJson, AActor* Actor)
	{
		FVector Loc = Actor->GetActorLocation();
		TSharedPtr<FJsonObject> LocObj = MakeShared<FJsonObject>();
		LocObj->SetNumberField(TEXT("x"), Loc.X);
		LocObj->SetNumberField(TEXT("y"), Loc.Y);
		LocObj->SetNumberField(TEXT("z"), Loc.Z);
		OutJson->SetObjectField(TEXT("location"), LocObj);

		FRotator Rot = Actor->GetActorRotation();
		TSharedPtr<FJsonObject> RotObj = MakeShared<FJsonObject>();
		RotObj->SetNumberField(TEXT("pitch"), Rot.Pitch);
		RotObj->SetNumberField(TEXT("yaw"), Rot.Yaw);
		RotObj->SetNumberField(TEXT("roll"), Rot.Roll);
		OutJson->SetObjectField(TEXT("rotation"), RotObj);
	}

	FString GetLightingActorType(AActor* Actor)
	{
		if (Cast<APointLight>(Actor)) return TEXT("PointLight");
		if (Cast<ASpotLight>(Actor)) return TEXT("SpotLight");
		if (Cast<ADirectionalLight>(Actor)) return TEXT("DirectionalLight");
		if (Cast<ARectLight>(Actor)) return TEXT("RectLight");
		if (Cast<ASkyLight>(Actor)) return TEXT("SkyLight");
		if (Actor->FindComponentByClass<USkyAtmosphereComponent>()) return TEXT("SkyAtmosphere");
		if (Cast<AExponentialHeightFog>(Actor)) return TEXT("ExponentialHeightFog");
		if (Cast<APostProcessVolume>(Actor)) return TEXT("PostProcessVolume");
		return TEXT("");
	}

	bool IsLightingActor(AActor* Actor)
	{
		return !GetLightingActorType(Actor).IsEmpty();
	}

	bool MatchesTypeFilter(const FString& ActorType, const FString& TypeFilter)
	{
		if (TypeFilter.IsEmpty()) return true;
		return ActorType.Equals(TypeFilter, ESearchCase::IgnoreCase);
	}

	TArray<FString> ApplyLightProperties(ULightComponent* LightComp, const TSharedPtr<FJsonObject>& Props)
	{
		TArray<FString> Applied;
		if (!LightComp || !Props.IsValid()) return Applied;

		double NumVal = 0.0;
		if (Props->TryGetNumberField(TEXT("intensity"), NumVal))
		{
			LightComp->SetIntensity(static_cast<float>(NumVal));
			Applied.Add(TEXT("intensity"));
		}

		const TSharedPtr<FJsonObject>* ColorObj = nullptr;
		if (Props->TryGetObjectField(TEXT("color"), ColorObj) && ColorObj)
		{
			double R = 0, G = 0, B = 0;
			(*ColorObj)->TryGetNumberField(TEXT("r"), R);
			(*ColorObj)->TryGetNumberField(TEXT("g"), G);
			(*ColorObj)->TryGetNumberField(TEXT("b"), B);

			if (R > 1.0 || G > 1.0 || B > 1.0)
			{
				LightComp->SetLightColor(FColor(
					static_cast<uint8>(FMath::Clamp(R, 0.0, 255.0)),
					static_cast<uint8>(FMath::Clamp(G, 0.0, 255.0)),
					static_cast<uint8>(FMath::Clamp(B, 0.0, 255.0))
				));
			}
			else
			{
				LightComp->SetLightColor(FLinearColor(
					static_cast<float>(R),
					static_cast<float>(G),
					static_cast<float>(B)
				).ToFColor(true));
			}
			Applied.Add(TEXT("color"));
		}

		if (Props->TryGetNumberField(TEXT("temperature"), NumVal))
		{
			LightComp->Temperature = static_cast<float>(NumVal);
			Applied.Add(TEXT("temperature"));
		}

		bool BoolVal = false;
		if (Props->TryGetBoolField(TEXT("use_temperature"), BoolVal))
		{
			LightComp->bUseTemperature = BoolVal;
			Applied.Add(TEXT("use_temperature"));
		}

		if (Props->TryGetBoolField(TEXT("cast_shadows"), BoolVal))
		{
			LightComp->SetCastShadows(BoolVal);
			Applied.Add(TEXT("cast_shadows"));
		}

		if (Props->TryGetBoolField(TEXT("affects_world"), BoolVal))
		{
			LightComp->bAffectsWorld = BoolVal;
			Applied.Add(TEXT("affects_world"));
		}

		if (Props->TryGetNumberField(TEXT("indirect_lighting_intensity"), NumVal))
		{
			LightComp->IndirectLightingIntensity = static_cast<float>(NumVal);
			Applied.Add(TEXT("indirect_lighting_intensity"));
		}

		if (Props->TryGetNumberField(TEXT("volumetric_scattering_intensity"), NumVal))
		{
			LightComp->VolumetricScatteringIntensity = static_cast<float>(NumVal);
			Applied.Add(TEXT("volumetric_scattering_intensity"));
		}

		const TSharedPtr<FJsonObject>* ChannelsObj = nullptr;
		if (Props->TryGetObjectField(TEXT("lighting_channels"), ChannelsObj) && ChannelsObj)
		{
			bool Ch = false;
			if ((*ChannelsObj)->TryGetBoolField(TEXT("channel0"), Ch))
			{
				LightComp->LightingChannels.bChannel0 = Ch;
			}
			if ((*ChannelsObj)->TryGetBoolField(TEXT("channel1"), Ch))
			{
				LightComp->LightingChannels.bChannel1 = Ch;
			}
			if ((*ChannelsObj)->TryGetBoolField(TEXT("channel2"), Ch))
			{
				LightComp->LightingChannels.bChannel2 = Ch;
			}
			Applied.Add(TEXT("lighting_channels"));
		}

		if (UPointLightComponent* PointComp = Cast<UPointLightComponent>(LightComp))
		{
			if (Props->TryGetNumberField(TEXT("attenuation_radius"), NumVal))
			{
				PointComp->SetAttenuationRadius(static_cast<float>(NumVal));
				Applied.Add(TEXT("attenuation_radius"));
			}
			if (Props->TryGetNumberField(TEXT("source_radius"), NumVal))
			{
				PointComp->SourceRadius = static_cast<float>(NumVal);
				Applied.Add(TEXT("source_radius"));
			}
			if (Props->TryGetNumberField(TEXT("soft_source_radius"), NumVal))
			{
				PointComp->SoftSourceRadius = static_cast<float>(NumVal);
				Applied.Add(TEXT("soft_source_radius"));
			}

			if (USpotLightComponent* SpotComp = Cast<USpotLightComponent>(LightComp))
			{
				if (Props->TryGetNumberField(TEXT("inner_cone_angle"), NumVal))
				{
					SpotComp->SetInnerConeAngle(static_cast<float>(NumVal));
					Applied.Add(TEXT("inner_cone_angle"));
				}
				if (Props->TryGetNumberField(TEXT("outer_cone_angle"), NumVal))
				{
					SpotComp->SetOuterConeAngle(static_cast<float>(NumVal));
					Applied.Add(TEXT("outer_cone_angle"));
				}
			}
		}

		if (URectLightComponent* RectComp = Cast<URectLightComponent>(LightComp))
		{
			if (Props->TryGetNumberField(TEXT("source_width"), NumVal))
			{
				RectComp->SourceWidth = static_cast<float>(NumVal);
				Applied.Add(TEXT("source_width"));
			}
			if (Props->TryGetNumberField(TEXT("source_height"), NumVal))
			{
				RectComp->SourceHeight = static_cast<float>(NumVal);
				Applied.Add(TEXT("source_height"));
			}
			if (Props->TryGetNumberField(TEXT("barn_door_angle"), NumVal))
			{
				RectComp->BarnDoorAngle = static_cast<float>(NumVal);
				Applied.Add(TEXT("barn_door_angle"));
			}
			if (Props->TryGetNumberField(TEXT("barn_door_length"), NumVal))
			{
				RectComp->BarnDoorLength = static_cast<float>(NumVal);
				Applied.Add(TEXT("barn_door_length"));
			}
		}

		LightComp->MarkRenderStateDirty();
		return Applied;
	}

	ULightComponent* GetLightComponentFromActor(AActor* Actor)
	{
		if (APointLight* PL = Cast<APointLight>(Actor)) return PL->GetLightComponent();
		if (ASpotLight* SL = Cast<ASpotLight>(Actor)) return SL->GetLightComponent();
		if (ADirectionalLight* DL = Cast<ADirectionalLight>(Actor)) return DL->GetLightComponent();
		if (ARectLight* RL = Cast<ARectLight>(Actor)) return RL->GetLightComponent();
		return nullptr;
	}

	FVector4 ParseVector4FromJson(const TSharedPtr<FJsonObject>& ParentObj, const FString& Key)
	{
		double ScalarVal = 0.0;
		if (ParentObj->TryGetNumberField(Key, ScalarVal))
		{
			float F = static_cast<float>(ScalarVal);
			return FVector4(F, F, F, F);
		}

		const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
		if (ParentObj->TryGetObjectField(Key, ObjPtr) && ObjPtr)
		{
			double R = 1.0, G = 1.0, B = 1.0, A = 1.0;
			(*ObjPtr)->TryGetNumberField(TEXT("r"), R);
			(*ObjPtr)->TryGetNumberField(TEXT("g"), G);
			(*ObjPtr)->TryGetNumberField(TEXT("b"), B);
			(*ObjPtr)->TryGetNumberField(TEXT("a"), A);
			return FVector4(static_cast<float>(R), static_cast<float>(G), static_cast<float>(B), static_cast<float>(A));
		}

		return FVector4(1.0f, 1.0f, 1.0f, 1.0f);
	}

	TArray<FString> ApplyPostProcessSettingsFromJson(FPostProcessSettings& PPSettings, const TSharedPtr<FJsonObject>& Json)
	{
		TArray<FString> Applied;
		if (!Json.IsValid()) return Applied;

		double NumVal = 0.0;

		if (Json->TryGetNumberField(TEXT("bloom_intensity"), NumVal))
		{
			PPSettings.bOverride_BloomIntensity = true;
			PPSettings.BloomIntensity = static_cast<float>(NumVal);
			Applied.Add(TEXT("bloom_intensity"));
		}
		if (Json->TryGetNumberField(TEXT("bloom_threshold"), NumVal))
		{
			PPSettings.bOverride_BloomThreshold = true;
			PPSettings.BloomThreshold = static_cast<float>(NumVal);
			Applied.Add(TEXT("bloom_threshold"));
		}
		FString BloomMethodStr;
		if (Json->TryGetStringField(TEXT("bloom_method"), BloomMethodStr))
		{
			PPSettings.bOverride_BloomMethod = true;
			if (BloomMethodStr.Equals(TEXT("convolution"), ESearchCase::IgnoreCase) || BloomMethodStr.Equals(TEXT("fft"), ESearchCase::IgnoreCase))
			{
				PPSettings.BloomMethod = EBloomMethod::BM_FFT;
			}
			else
			{
				PPSettings.BloomMethod = EBloomMethod::BM_SOG;
			}
			Applied.Add(TEXT("bloom_method"));
		}

		if (Json->TryGetNumberField(TEXT("dof_focal_distance"), NumVal))
		{
			PPSettings.bOverride_DepthOfFieldFocalDistance = true;
			PPSettings.DepthOfFieldFocalDistance = static_cast<float>(NumVal);
			Applied.Add(TEXT("dof_focal_distance"));
		}
		if (Json->TryGetNumberField(TEXT("dof_fstop"), NumVal))
		{
			PPSettings.bOverride_DepthOfFieldFstop = true;
			PPSettings.DepthOfFieldFstop = static_cast<float>(NumVal);
			Applied.Add(TEXT("dof_fstop"));
		}
		if (Json->TryGetNumberField(TEXT("dof_sensor_width"), NumVal))
		{
			PPSettings.bOverride_DepthOfFieldSensorWidth = true;
			PPSettings.DepthOfFieldSensorWidth = static_cast<float>(NumVal);
			Applied.Add(TEXT("dof_sensor_width"));
		}
		if (Json->TryGetNumberField(TEXT("dof_min_fstop"), NumVal))
		{
			PPSettings.bOverride_DepthOfFieldMinFstop = true;
			PPSettings.DepthOfFieldMinFstop = static_cast<float>(NumVal);
			Applied.Add(TEXT("dof_min_fstop"));
		}
		if (Json->TryGetNumberField(TEXT("dof_blade_count"), NumVal))
		{
			PPSettings.bOverride_DepthOfFieldBladeCount = true;
			PPSettings.DepthOfFieldBladeCount = static_cast<int32>(NumVal);
			Applied.Add(TEXT("dof_blade_count"));
		}

		if (Json->TryGetNumberField(TEXT("motion_blur_amount"), NumVal))
		{
			PPSettings.bOverride_MotionBlurAmount = true;
			PPSettings.MotionBlurAmount = static_cast<float>(NumVal);
			Applied.Add(TEXT("motion_blur_amount"));
		}
		if (Json->TryGetNumberField(TEXT("motion_blur_max"), NumVal))
		{
			PPSettings.bOverride_MotionBlurMax = true;
			PPSettings.MotionBlurMax = static_cast<float>(NumVal);
			Applied.Add(TEXT("motion_blur_max"));
		}

		if (Json->TryGetNumberField(TEXT("exposure_compensation"), NumVal))
		{
			PPSettings.bOverride_AutoExposureBias = true;
			PPSettings.AutoExposureBias = static_cast<float>(NumVal);
			Applied.Add(TEXT("exposure_compensation"));
		}
		if (Json->TryGetNumberField(TEXT("exposure_min_brightness"), NumVal))
		{
			PPSettings.bOverride_AutoExposureMinBrightness = true;
			PPSettings.AutoExposureMinBrightness = static_cast<float>(NumVal);
			Applied.Add(TEXT("exposure_min_brightness"));
		}
		if (Json->TryGetNumberField(TEXT("exposure_max_brightness"), NumVal))
		{
			PPSettings.bOverride_AutoExposureMaxBrightness = true;
			PPSettings.AutoExposureMaxBrightness = static_cast<float>(NumVal);
			Applied.Add(TEXT("exposure_max_brightness"));
		}
		if (Json->TryGetNumberField(TEXT("exposure_speed_up"), NumVal))
		{
			PPSettings.bOverride_AutoExposureSpeedUp = true;
			PPSettings.AutoExposureSpeedUp = static_cast<float>(NumVal);
			Applied.Add(TEXT("exposure_speed_up"));
		}
		if (Json->TryGetNumberField(TEXT("exposure_speed_down"), NumVal))
		{
			PPSettings.bOverride_AutoExposureSpeedDown = true;
			PPSettings.AutoExposureSpeedDown = static_cast<float>(NumVal);
			Applied.Add(TEXT("exposure_speed_down"));
		}

		if (Json->TryGetNumberField(TEXT("white_balance_temp"), NumVal))
		{
			PPSettings.bOverride_WhiteTemp = true;
			PPSettings.WhiteTemp = static_cast<float>(NumVal);
			Applied.Add(TEXT("white_balance_temp"));
		}
		if (Json->TryGetNumberField(TEXT("white_balance_tint"), NumVal))
		{
			PPSettings.bOverride_WhiteTint = true;
			PPSettings.WhiteTint = static_cast<float>(NumVal);
			Applied.Add(TEXT("white_balance_tint"));
		}
		if (Json->HasField(TEXT("saturation")))
		{
			PPSettings.bOverride_ColorSaturation = true;
			PPSettings.ColorSaturation = ParseVector4FromJson(Json, TEXT("saturation"));
			Applied.Add(TEXT("saturation"));
		}
		if (Json->HasField(TEXT("contrast")))
		{
			PPSettings.bOverride_ColorContrast = true;
			PPSettings.ColorContrast = ParseVector4FromJson(Json, TEXT("contrast"));
			Applied.Add(TEXT("contrast"));
		}
		if (Json->HasField(TEXT("gamma")))
		{
			PPSettings.bOverride_ColorGamma = true;
			PPSettings.ColorGamma = ParseVector4FromJson(Json, TEXT("gamma"));
			Applied.Add(TEXT("gamma"));
		}
		if (Json->HasField(TEXT("gain")))
		{
			PPSettings.bOverride_ColorGain = true;
			PPSettings.ColorGain = ParseVector4FromJson(Json, TEXT("gain"));
			Applied.Add(TEXT("gain"));
		}
		FString LUTPath;
		if (Json->TryGetStringField(TEXT("color_grading_lut"), LUTPath))
		{
			UTexture* LUTTexture = Cast<UTexture>(
				StaticLoadObject(UTexture::StaticClass(), nullptr, *LUTPath));
			if (LUTTexture)
			{
				PPSettings.bOverride_ColorGradingLUT = true;
				PPSettings.ColorGradingLUT = LUTTexture;
				Applied.Add(TEXT("color_grading_lut"));
			}
		}

		if (Json->TryGetNumberField(TEXT("vignette_intensity"), NumVal))
		{
			PPSettings.bOverride_VignetteIntensity = true;
			PPSettings.VignetteIntensity = static_cast<float>(NumVal);
			Applied.Add(TEXT("vignette_intensity"));
		}

		if (Json->TryGetNumberField(TEXT("ao_intensity"), NumVal))
		{
			PPSettings.bOverride_AmbientOcclusionIntensity = true;
			PPSettings.AmbientOcclusionIntensity = static_cast<float>(NumVal);
			Applied.Add(TEXT("ao_intensity"));
		}
		if (Json->TryGetNumberField(TEXT("ao_radius"), NumVal))
		{
			PPSettings.bOverride_AmbientOcclusionRadius = true;
			PPSettings.AmbientOcclusionRadius = static_cast<float>(NumVal);
			Applied.Add(TEXT("ao_radius"));
		}
		if (Json->TryGetNumberField(TEXT("ao_quality"), NumVal))
		{
			PPSettings.bOverride_AmbientOcclusionQuality = true;
			PPSettings.AmbientOcclusionQuality = static_cast<float>(NumVal);
			Applied.Add(TEXT("ao_quality"));
		}

		return Applied;
	}
}

// ============================================================================
// SuccessResult / ErrorResult
// ============================================================================

TSharedPtr<FJsonObject> FLightingEditor::SuccessResult(const FString& Message)
{
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("message"), Message);
	return Result;
}

TSharedPtr<FJsonObject> FLightingEditor::ErrorResult(const FString& Message)
{
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), false);
	Result->SetStringField(TEXT("error"), Message);
	return Result;
}

// ============================================================================
// InspectLight
// ============================================================================

TSharedPtr<FJsonObject> FLightingEditor::InspectLight(UWorld* World, const FString& ActorNameOrLabel)
{
	if (!World)
	{
		return ErrorResult(TEXT("World is null"));
	}
	if (ActorNameOrLabel.IsEmpty())
	{
		return ErrorResult(TEXT("Actor name/label is required"));
	}

	AActor* FoundActor = FindLightingActorByNameOrLabel(World, ActorNameOrLabel);
	if (!FoundActor)
	{
		return ErrorResult(FString::Printf(TEXT("Actor not found: %s"), *ActorNameOrLabel));
	}

	TSharedPtr<FJsonObject> Result = SuccessResult(TEXT("Light inspected"));
	Result->SetStringField(TEXT("actor_name"), FoundActor->GetName());
	Result->SetStringField(TEXT("actor_label"), FoundActor->GetActorLabel());
	Result->SetStringField(TEXT("class"), FoundActor->GetClass()->GetName());
	AddActorTransform(Result, FoundActor);

	if (APointLight* PointLightActor = Cast<APointLight>(FoundActor))
	{
		Result->SetStringField(TEXT("type"), TEXT("PointLight"));
		UPointLightComponent* PointComp = Cast<UPointLightComponent>(PointLightActor->GetLightComponent());
		if (PointComp)
		{
			AddCommonLightProperties(Result, PointComp);
			Result->SetNumberField(TEXT("attenuation_radius"), PointComp->AttenuationRadius);
			Result->SetNumberField(TEXT("source_radius"), PointComp->SourceRadius);
			Result->SetNumberField(TEXT("soft_source_radius"), PointComp->SoftSourceRadius);
		}
	}
	else if (ASpotLight* SpotLightActor = Cast<ASpotLight>(FoundActor))
	{
		Result->SetStringField(TEXT("type"), TEXT("SpotLight"));
		USpotLightComponent* SpotComp = Cast<USpotLightComponent>(SpotLightActor->GetLightComponent());
		if (SpotComp)
		{
			AddCommonLightProperties(Result, SpotComp);
			Result->SetNumberField(TEXT("attenuation_radius"), SpotComp->AttenuationRadius);
			Result->SetNumberField(TEXT("inner_cone_angle"), SpotComp->InnerConeAngle);
			Result->SetNumberField(TEXT("outer_cone_angle"), SpotComp->OuterConeAngle);
			Result->SetNumberField(TEXT("source_radius"), SpotComp->SourceRadius);
		}
	}
	else if (ADirectionalLight* DirLightActor = Cast<ADirectionalLight>(FoundActor))
	{
		Result->SetStringField(TEXT("type"), TEXT("DirectionalLight"));
		UDirectionalLightComponent* DirComp = Cast<UDirectionalLightComponent>(DirLightActor->GetLightComponent());
		if (DirComp)
		{
			AddCommonLightProperties(Result, DirComp);
			Result->SetBoolField(TEXT("light_shaft_occlusion"), DirComp->bEnableLightShaftOcclusion != 0);
			Result->SetBoolField(TEXT("light_shaft_bloom"), DirComp->bEnableLightShaftBloom != 0);
		}
	}
	else if (ARectLight* RectLightActor = Cast<ARectLight>(FoundActor))
	{
		Result->SetStringField(TEXT("type"), TEXT("RectLight"));
		URectLightComponent* RectComp = Cast<URectLightComponent>(RectLightActor->GetLightComponent());
		if (RectComp)
		{
			AddCommonLightProperties(Result, RectComp);
			Result->SetNumberField(TEXT("source_width"), RectComp->SourceWidth);
			Result->SetNumberField(TEXT("source_height"), RectComp->SourceHeight);
			Result->SetNumberField(TEXT("barn_door_angle"), RectComp->BarnDoorAngle);
			Result->SetNumberField(TEXT("barn_door_length"), RectComp->BarnDoorLength);
		}
	}
	else if (ASkyLight* SkyLightActor = Cast<ASkyLight>(FoundActor))
	{
		Result->SetStringField(TEXT("type"), TEXT("SkyLight"));
		USkyLightComponent* SkyComp = SkyLightActor->GetLightComponent();
		if (SkyComp)
		{
			Result->SetNumberField(TEXT("intensity"), SkyComp->Intensity);

			TSharedPtr<FJsonObject> SkyColorObj = MakeShared<FJsonObject>();
			SkyColorObj->SetNumberField(TEXT("r"), SkyComp->LightColor.R);
			SkyColorObj->SetNumberField(TEXT("g"), SkyComp->LightColor.G);
			SkyColorObj->SetNumberField(TEXT("b"), SkyComp->LightColor.B);
			Result->SetObjectField(TEXT("light_color"), SkyColorObj);

			Result->SetBoolField(TEXT("cast_shadows"), SkyComp->CastShadows != 0);

			FString CubemapPath = SkyComp->Cubemap ? SkyComp->Cubemap->GetPathName() : TEXT("None");
			Result->SetStringField(TEXT("cubemap"), CubemapPath);
			Result->SetNumberField(TEXT("sky_distance_threshold"), SkyComp->SkyDistanceThreshold);
			Result->SetBoolField(TEXT("lower_hemisphere_is_solid_color"), SkyComp->bLowerHemisphereIsBlack != 0);

			FString SourceTypeStr;
			switch (SkyComp->SourceType)
			{
			case ESkyLightSourceType::SLS_CapturedScene: SourceTypeStr = TEXT("CapturedScene"); break;
			case ESkyLightSourceType::SLS_SpecifiedCubemap: SourceTypeStr = TEXT("SpecifiedCubemap"); break;
			default: SourceTypeStr = TEXT("Unknown"); break;
			}
			Result->SetStringField(TEXT("source_type"), SourceTypeStr);
			Result->SetBoolField(TEXT("captures_scene"), SkyComp->SourceType == ESkyLightSourceType::SLS_CapturedScene);
		}
	}
	else if (USkyAtmosphereComponent* AtmoComp = FoundActor->FindComponentByClass<USkyAtmosphereComponent>())
	{
		Result->SetStringField(TEXT("type"), TEXT("SkyAtmosphere"));
		Result->SetNumberField(TEXT("bottom_radius"), AtmoComp->BottomRadius);
		Result->SetNumberField(TEXT("atmosphere_height"), AtmoComp->AtmosphereHeight);

		TSharedPtr<FJsonObject> AlbedoObj = MakeShared<FJsonObject>();
		AlbedoObj->SetNumberField(TEXT("r"), AtmoComp->GroundAlbedo.R);
		AlbedoObj->SetNumberField(TEXT("g"), AtmoComp->GroundAlbedo.G);
		AlbedoObj->SetNumberField(TEXT("b"), AtmoComp->GroundAlbedo.B);
		Result->SetObjectField(TEXT("ground_albedo"), AlbedoObj);

		Result->SetNumberField(TEXT("rayleigh_scattering_scale"), AtmoComp->RayleighScatteringScale);
		Result->SetNumberField(TEXT("mie_scattering_scale"), AtmoComp->MieScatteringScale);
	}
	else if (AExponentialHeightFog* FogActor = Cast<AExponentialHeightFog>(FoundActor))
	{
		Result->SetStringField(TEXT("type"), TEXT("ExponentialHeightFog"));
		UExponentialHeightFogComponent* FogComp = FogActor->FindComponentByClass<UExponentialHeightFogComponent>();
		if (FogComp)
		{
			Result->SetNumberField(TEXT("fog_density"), FogComp->FogDensity);
			Result->SetNumberField(TEXT("fog_height_falloff"), FogComp->FogHeightFalloff);
			Result->SetNumberField(TEXT("start_distance"), FogComp->StartDistance);
			Result->SetNumberField(TEXT("fog_cutoff_distance"), FogComp->FogCutoffDistance);
			Result->SetNumberField(TEXT("fog_max_opacity"), FogComp->FogMaxOpacity);
		}
	}
	else if (APostProcessVolume* PPVActor = Cast<APostProcessVolume>(FoundActor))
	{
		Result->SetStringField(TEXT("type"), TEXT("PostProcessVolume"));
		Result->SetBoolField(TEXT("enabled"), PPVActor->bEnabled);
		Result->SetBoolField(TEXT("infinite_extent"), PPVActor->bUnbound);
		Result->SetNumberField(TEXT("blend_radius"), PPVActor->BlendRadius);
		Result->SetNumberField(TEXT("blend_weight"), PPVActor->BlendWeight);
		Result->SetNumberField(TEXT("priority"), PPVActor->Priority);

		const FPostProcessSettings& PPSettings = PPVActor->Settings;
		TSharedPtr<FJsonObject> SettingsObj = MakeShared<FJsonObject>();
		SettingsObj->SetBoolField(TEXT("bloom_enabled"), PPSettings.bOverride_BloomIntensity);
		SettingsObj->SetNumberField(TEXT("bloom_intensity"), PPSettings.BloomIntensity);
		SettingsObj->SetBoolField(TEXT("auto_exposure_enabled"), PPSettings.bOverride_AutoExposureBias);
		SettingsObj->SetNumberField(TEXT("auto_exposure_bias"), PPSettings.AutoExposureBias);
		Result->SetObjectField(TEXT("settings_sample"), SettingsObj);
	}
	else
	{
		return ErrorResult(FString::Printf(TEXT("Actor '%s' is not a recognized lighting/atmosphere/post-process type (class: %s)"),
			*ActorNameOrLabel, *FoundActor->GetClass()->GetName()));
	}

	return Result;
}

// ============================================================================
// ListLights
// ============================================================================

TSharedPtr<FJsonObject> FLightingEditor::ListLights(UWorld* World, const FString& TypeFilter)
{
	if (!World)
	{
		return ErrorResult(TEXT("World is null"));
	}

	TArray<TSharedPtr<FJsonValue>> LightsArray;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor) continue;

		FString ActorType = GetLightingActorType(Actor);
		if (ActorType.IsEmpty()) continue;
		if (!MatchesTypeFilter(ActorType, TypeFilter)) continue;

		TSharedPtr<FJsonObject> EntryObj = MakeShared<FJsonObject>();
		EntryObj->SetStringField(TEXT("name"), Actor->GetName());
		EntryObj->SetStringField(TEXT("label"), Actor->GetActorLabel());
		EntryObj->SetStringField(TEXT("class"), Actor->GetClass()->GetName());
		EntryObj->SetStringField(TEXT("type"), ActorType);

		FVector Loc = Actor->GetActorLocation();
		TSharedPtr<FJsonObject> LocObj = MakeShared<FJsonObject>();
		LocObj->SetNumberField(TEXT("x"), Loc.X);
		LocObj->SetNumberField(TEXT("y"), Loc.Y);
		LocObj->SetNumberField(TEXT("z"), Loc.Z);
		EntryObj->SetObjectField(TEXT("location"), LocObj);

		LightsArray.Add(MakeShared<FJsonValueObject>(EntryObj));
	}

	TSharedPtr<FJsonObject> Result = SuccessResult(FString::Printf(TEXT("Found %d lighting actors"), LightsArray.Num()));
	Result->SetNumberField(TEXT("count"), LightsArray.Num());
	if (!TypeFilter.IsEmpty())
	{
		Result->SetStringField(TEXT("type_filter"), TypeFilter);
	}
	Result->SetArrayField(TEXT("actors"), LightsArray);

	return Result;
}

// ============================================================================
// Stub implementations
// ============================================================================

TSharedPtr<FJsonObject> FLightingEditor::SpawnLight(UWorld* World, const FString& LightType, const FVector& Location,
	const FRotator& Rotation, const FString& Label, const TSharedPtr<FJsonObject>& Properties)
{
	if (!World)
	{
		return ErrorResult(TEXT("World is null"));
	}

	FString TypeLower = LightType.ToLower();
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* SpawnedActor = nullptr;
	FString ResolvedType;

	if (TypeLower == TEXT("point") || TypeLower == TEXT("pointlight"))
	{
		SpawnedActor = World->SpawnActor<APointLight>(Location, Rotation, SpawnParams);
		ResolvedType = TEXT("PointLight");
	}
	else if (TypeLower == TEXT("spot") || TypeLower == TEXT("spotlight"))
	{
		SpawnedActor = World->SpawnActor<ASpotLight>(Location, Rotation, SpawnParams);
		ResolvedType = TEXT("SpotLight");
	}
	else if (TypeLower == TEXT("directional") || TypeLower == TEXT("directionallight"))
	{
		SpawnedActor = World->SpawnActor<ADirectionalLight>(Location, Rotation, SpawnParams);
		ResolvedType = TEXT("DirectionalLight");
	}
	else if (TypeLower == TEXT("rect") || TypeLower == TEXT("rectlight"))
	{
		SpawnedActor = World->SpawnActor<ARectLight>(Location, Rotation, SpawnParams);
		ResolvedType = TEXT("RectLight");
	}
	else
	{
		return ErrorResult(FString::Printf(TEXT("Unknown light type: '%s'. Expected: point, spot, directional, rect"), *LightType));
	}

	if (!SpawnedActor)
	{
		return ErrorResult(FString::Printf(TEXT("Failed to spawn %s actor"), *ResolvedType));
	}

	if (!Label.IsEmpty())
	{
		SpawnedActor->SetActorLabel(Label);
	}

	if (Properties.IsValid())
	{
		ULightComponent* LightComp = GetLightComponentFromActor(SpawnedActor);
		if (LightComp)
		{
			ApplyLightProperties(LightComp, Properties);
		}
	}

	SpawnedActor->MarkPackageDirty();
	World->MarkPackageDirty();

	TSharedPtr<FJsonObject> Result = SuccessResult(FString::Printf(TEXT("Spawned %s"), *ResolvedType));
	Result->SetStringField(TEXT("actor_name"), SpawnedActor->GetName());
	Result->SetStringField(TEXT("actor_label"), SpawnedActor->GetActorLabel());
	Result->SetStringField(TEXT("class"), SpawnedActor->GetClass()->GetName());
	Result->SetStringField(TEXT("type"), ResolvedType);
	AddActorTransform(Result, SpawnedActor);

	return Result;
}

TSharedPtr<FJsonObject> FLightingEditor::SetLightProperties(UWorld* World, const FString& ActorNameOrLabel,
	const TSharedPtr<FJsonObject>& Properties)
{
	if (!World)
	{
		return ErrorResult(TEXT("World is null"));
	}
	if (ActorNameOrLabel.IsEmpty())
	{
		return ErrorResult(TEXT("Actor name/label is required"));
	}
	if (!Properties.IsValid())
	{
		return ErrorResult(TEXT("Properties object is required"));
	}

	AActor* FoundActor = FindLightingActorByNameOrLabel(World, ActorNameOrLabel);
	if (!FoundActor)
	{
		return ErrorResult(FString::Printf(TEXT("Actor not found: %s"), *ActorNameOrLabel));
	}

	ULightComponent* LightComp = GetLightComponentFromActor(FoundActor);
	if (!LightComp)
	{
		return ErrorResult(FString::Printf(TEXT("Actor '%s' (class: %s) is not a standard light type (point/spot/directional/rect)"),
			*ActorNameOrLabel, *FoundActor->GetClass()->GetName()));
	}

	TArray<FString> AppliedProps = ApplyLightProperties(LightComp, Properties);

	FoundActor->MarkPackageDirty();

	TSharedPtr<FJsonObject> Result = SuccessResult(FString::Printf(TEXT("Set %d properties on '%s'"), AppliedProps.Num(), *ActorNameOrLabel));
	Result->SetStringField(TEXT("actor_name"), FoundActor->GetName());
	Result->SetStringField(TEXT("actor_label"), FoundActor->GetActorLabel());

	TArray<TSharedPtr<FJsonValue>> PropsArray;
	for (const FString& PropName : AppliedProps)
	{
		PropsArray.Add(MakeShared<FJsonValueString>(PropName));
	}
	Result->SetArrayField(TEXT("properties_set"), PropsArray);

	return Result;
}

TSharedPtr<FJsonObject> FLightingEditor::SetSkyLight(UWorld* World, const FString& ActorNameOrLabel,
	const TSharedPtr<FJsonObject>& Properties)
{
	if (!World)
	{
		return ErrorResult(TEXT("World is null"));
	}
	if (ActorNameOrLabel.IsEmpty())
	{
		return ErrorResult(TEXT("Actor name/label is required"));
	}
	if (!Properties.IsValid())
	{
		return ErrorResult(TEXT("Properties object is required"));
	}

	AActor* FoundActor = FindLightingActorByNameOrLabel(World, ActorNameOrLabel);
	if (!FoundActor)
	{
		return ErrorResult(FString::Printf(TEXT("Actor not found: %s"), *ActorNameOrLabel));
	}

	ASkyLight* SkyLightActor = Cast<ASkyLight>(FoundActor);
	if (!SkyLightActor)
	{
		return ErrorResult(FString::Printf(TEXT("Actor '%s' is not a SkyLight (class: %s)"),
			*ActorNameOrLabel, *FoundActor->GetClass()->GetName()));
	}

	USkyLightComponent* SkyComp = SkyLightActor->GetLightComponent();
	if (!SkyComp)
	{
		return ErrorResult(FString::Printf(TEXT("SkyLight '%s' has no SkyLightComponent"), *ActorNameOrLabel));
	}

	TArray<FString> Applied;
	double NumVal = 0.0;

	if (Properties->TryGetNumberField(TEXT("intensity"), NumVal))
	{
		SkyComp->Intensity = static_cast<float>(NumVal);
		Applied.Add(TEXT("intensity"));
	}

	const TSharedPtr<FJsonObject>* ColorObj = nullptr;
	if (Properties->TryGetObjectField(TEXT("color"), ColorObj) && ColorObj)
	{
		double R = 0, G = 0, B = 0;
		(*ColorObj)->TryGetNumberField(TEXT("r"), R);
		(*ColorObj)->TryGetNumberField(TEXT("g"), G);
		(*ColorObj)->TryGetNumberField(TEXT("b"), B);

		if (R > 1.0 || G > 1.0 || B > 1.0)
		{
			SkyComp->LightColor = FColor(
				static_cast<uint8>(FMath::Clamp(R, 0.0, 255.0)),
				static_cast<uint8>(FMath::Clamp(G, 0.0, 255.0)),
				static_cast<uint8>(FMath::Clamp(B, 0.0, 255.0))
			);
		}
		else
		{
			SkyComp->LightColor = FLinearColor(
				static_cast<float>(R),
				static_cast<float>(G),
				static_cast<float>(B)
			).ToFColor(true);
		}
		Applied.Add(TEXT("color"));
	}

	FString CubemapPath;
	if (Properties->TryGetStringField(TEXT("cubemap"), CubemapPath))
	{
		UTextureCube* CubeTexture = Cast<UTextureCube>(
			StaticLoadObject(UTextureCube::StaticClass(), nullptr, *CubemapPath));
		if (CubeTexture)
		{
			SkyComp->Cubemap = CubeTexture;
			Applied.Add(TEXT("cubemap"));
		}
		else
		{
			return ErrorResult(FString::Printf(TEXT("Failed to load cubemap: %s"), *CubemapPath));
		}
	}

	FString SourceTypeStr;
	if (Properties->TryGetStringField(TEXT("source_type"), SourceTypeStr))
	{
		if (SourceTypeStr.Equals(TEXT("captured_scene"), ESearchCase::IgnoreCase))
		{
			SkyComp->SourceType = ESkyLightSourceType::SLS_CapturedScene;
			Applied.Add(TEXT("source_type"));
		}
		else if (SourceTypeStr.Equals(TEXT("specified_cubemap"), ESearchCase::IgnoreCase))
		{
			SkyComp->SourceType = ESkyLightSourceType::SLS_SpecifiedCubemap;
			Applied.Add(TEXT("source_type"));
		}
	}

	const TSharedPtr<FJsonObject>* LowerColorObj = nullptr;
	if (Properties->TryGetObjectField(TEXT("lower_hemisphere_color"), LowerColorObj) && LowerColorObj)
	{
		double R = 0, G = 0, B = 0, A = 1;
		(*LowerColorObj)->TryGetNumberField(TEXT("r"), R);
		(*LowerColorObj)->TryGetNumberField(TEXT("g"), G);
		(*LowerColorObj)->TryGetNumberField(TEXT("b"), B);
		(*LowerColorObj)->TryGetNumberField(TEXT("a"), A);
		SkyComp->LowerHemisphereColor = FLinearColor(
			static_cast<float>(R), static_cast<float>(G),
			static_cast<float>(B), static_cast<float>(A));
		Applied.Add(TEXT("lower_hemisphere_color"));
	}

	bool BoolVal = false;
	if (Properties->TryGetBoolField(TEXT("lower_hemisphere_is_solid_color"), BoolVal))
	{
		SkyComp->bLowerHemisphereIsBlack = BoolVal;
		Applied.Add(TEXT("lower_hemisphere_is_solid_color"));
	}

	if (Properties->TryGetBoolField(TEXT("cloud_ambient_occlusion"), BoolVal))
	{
		SkyComp->bCloudAmbientOcclusion = BoolVal;
		Applied.Add(TEXT("cloud_ambient_occlusion"));
	}

	if (Properties->TryGetNumberField(TEXT("cloud_ambient_occlusion_strength"), NumVal))
	{
		SkyComp->CloudAmbientOcclusionStrength = static_cast<float>(NumVal);
		Applied.Add(TEXT("cloud_ambient_occlusion_strength"));
	}

	if (Properties->TryGetBoolField(TEXT("recapture"), BoolVal) && BoolVal)
	{
		SkyComp->RecaptureSky();
		Applied.Add(TEXT("recapture"));
	}

	SkyComp->MarkRenderStateDirty();
	FoundActor->MarkPackageDirty();

	TSharedPtr<FJsonObject> Result = SuccessResult(FString::Printf(
		TEXT("Set %d properties on SkyLight '%s'"), Applied.Num(), *ActorNameOrLabel));
	Result->SetStringField(TEXT("actor_name"), FoundActor->GetName());

	TArray<TSharedPtr<FJsonValue>> PropsArray;
	for (const FString& PropName : Applied)
	{
		PropsArray.Add(MakeShared<FJsonValueString>(PropName));
	}
	Result->SetArrayField(TEXT("properties_set"), PropsArray);

	return Result;
}

TSharedPtr<FJsonObject> FLightingEditor::SetSkyAtmosphere(UWorld* World, const FString& ActorNameOrLabel,
	const TSharedPtr<FJsonObject>& Properties)
{
	if (!World)
	{
		return ErrorResult(TEXT("World is null"));
	}
	if (ActorNameOrLabel.IsEmpty())
	{
		return ErrorResult(TEXT("Actor name/label is required"));
	}
	if (!Properties.IsValid())
	{
		return ErrorResult(TEXT("Properties object is required"));
	}

	AActor* FoundActor = FindLightingActorByNameOrLabel(World, ActorNameOrLabel);
	if (!FoundActor)
	{
		return ErrorResult(FString::Printf(TEXT("Actor not found: %s"), *ActorNameOrLabel));
	}

	USkyAtmosphereComponent* AtmoComp = FoundActor->FindComponentByClass<USkyAtmosphereComponent>();
	if (!AtmoComp)
	{
		return ErrorResult(FString::Printf(TEXT("Actor '%s' has no SkyAtmosphereComponent (class: %s)"),
			*ActorNameOrLabel, *FoundActor->GetClass()->GetName()));
	}

	TArray<FString> Applied;
	double NumVal = 0.0;

	const TSharedPtr<FJsonObject>* RayleighObj = nullptr;
	if (Properties->TryGetObjectField(TEXT("rayleigh_scattering"), RayleighObj) && RayleighObj)
	{
		double R = 0, G = 0, B = 0;
		(*RayleighObj)->TryGetNumberField(TEXT("r"), R);
		(*RayleighObj)->TryGetNumberField(TEXT("g"), G);
		(*RayleighObj)->TryGetNumberField(TEXT("b"), B);
		AtmoComp->RayleighScattering = FLinearColor(
			static_cast<float>(R), static_cast<float>(G), static_cast<float>(B));
		Applied.Add(TEXT("rayleigh_scattering"));
	}

	if (Properties->TryGetNumberField(TEXT("rayleigh_exponential_distribution"), NumVal))
	{
		AtmoComp->RayleighExponentialDistribution = static_cast<float>(NumVal);
		Applied.Add(TEXT("rayleigh_exponential_distribution"));
	}

	const TSharedPtr<FJsonObject>* MieScatObj = nullptr;
	if (Properties->TryGetObjectField(TEXT("mie_scattering"), MieScatObj) && MieScatObj)
	{
		double R = 0, G = 0, B = 0;
		(*MieScatObj)->TryGetNumberField(TEXT("r"), R);
		(*MieScatObj)->TryGetNumberField(TEXT("g"), G);
		(*MieScatObj)->TryGetNumberField(TEXT("b"), B);
		AtmoComp->MieScattering = FLinearColor(
			static_cast<float>(R), static_cast<float>(G), static_cast<float>(B));
		Applied.Add(TEXT("mie_scattering"));
	}

	const TSharedPtr<FJsonObject>* MieAbsObj = nullptr;
	if (Properties->TryGetObjectField(TEXT("mie_absorption"), MieAbsObj) && MieAbsObj)
	{
		double R = 0, G = 0, B = 0;
		(*MieAbsObj)->TryGetNumberField(TEXT("r"), R);
		(*MieAbsObj)->TryGetNumberField(TEXT("g"), G);
		(*MieAbsObj)->TryGetNumberField(TEXT("b"), B);
		AtmoComp->MieAbsorption = FLinearColor(
			static_cast<float>(R), static_cast<float>(G), static_cast<float>(B));
		Applied.Add(TEXT("mie_absorption"));
	}

	if (Properties->TryGetNumberField(TEXT("mie_anisotropy"), NumVal))
	{
		AtmoComp->MieAnisotropy = static_cast<float>(NumVal);
		Applied.Add(TEXT("mie_anisotropy"));
	}

	if (Properties->TryGetNumberField(TEXT("mie_exponential_distribution"), NumVal))
	{
		AtmoComp->MieExponentialDistribution = static_cast<float>(NumVal);
		Applied.Add(TEXT("mie_exponential_distribution"));
	}

	if (Properties->TryGetNumberField(TEXT("atmosphere_height"), NumVal))
	{
		AtmoComp->AtmosphereHeight = static_cast<float>(NumVal);
		Applied.Add(TEXT("atmosphere_height"));
	}

	const TSharedPtr<FJsonObject>* AlbedoObj = nullptr;
	if (Properties->TryGetObjectField(TEXT("ground_albedo"), AlbedoObj) && AlbedoObj)
	{
		double R = 0, G = 0, B = 0;
		(*AlbedoObj)->TryGetNumberField(TEXT("r"), R);
		(*AlbedoObj)->TryGetNumberField(TEXT("g"), G);
		(*AlbedoObj)->TryGetNumberField(TEXT("b"), B);
		AtmoComp->GroundAlbedo = FColor(
			static_cast<uint8>(FMath::Clamp(R, 0.0, 255.0)),
			static_cast<uint8>(FMath::Clamp(G, 0.0, 255.0)),
			static_cast<uint8>(FMath::Clamp(B, 0.0, 255.0)));
		Applied.Add(TEXT("ground_albedo"));
	}

	if (Properties->TryGetNumberField(TEXT("multi_scattering_factor"), NumVal))
	{
		AtmoComp->MultiScatteringFactor = static_cast<float>(NumVal);
		Applied.Add(TEXT("multi_scattering_factor"));
	}

	AtmoComp->MarkRenderStateDirty();
	FoundActor->MarkPackageDirty();

	TSharedPtr<FJsonObject> Result = SuccessResult(FString::Printf(
		TEXT("Set %d properties on SkyAtmosphere '%s'"), Applied.Num(), *ActorNameOrLabel));
	Result->SetStringField(TEXT("actor_name"), FoundActor->GetName());

	TArray<TSharedPtr<FJsonValue>> PropsArray;
	for (const FString& PropName : Applied)
	{
		PropsArray.Add(MakeShared<FJsonValueString>(PropName));
	}
	Result->SetArrayField(TEXT("properties_set"), PropsArray);

	return Result;
}

TSharedPtr<FJsonObject> FLightingEditor::SetFog(UWorld* World, const FString& ActorNameOrLabel,
	const TSharedPtr<FJsonObject>& Properties)
{
	if (!World)
	{
		return ErrorResult(TEXT("World is null"));
	}
	if (ActorNameOrLabel.IsEmpty())
	{
		return ErrorResult(TEXT("Actor name/label is required"));
	}
	if (!Properties.IsValid())
	{
		return ErrorResult(TEXT("Properties object is required"));
	}

	AActor* FoundActor = FindLightingActorByNameOrLabel(World, ActorNameOrLabel);
	if (!FoundActor)
	{
		return ErrorResult(FString::Printf(TEXT("Actor not found: %s"), *ActorNameOrLabel));
	}

	AExponentialHeightFog* FogActor = Cast<AExponentialHeightFog>(FoundActor);
	if (!FogActor)
	{
		return ErrorResult(FString::Printf(TEXT("Actor '%s' is not an ExponentialHeightFog (class: %s)"),
			*ActorNameOrLabel, *FoundActor->GetClass()->GetName()));
	}

	UExponentialHeightFogComponent* FogComp = FogActor->FindComponentByClass<UExponentialHeightFogComponent>();
	if (!FogComp)
	{
		return ErrorResult(FString::Printf(TEXT("Fog actor '%s' has no ExponentialHeightFogComponent"), *ActorNameOrLabel));
	}

	TArray<FString> Applied;
	double NumVal = 0.0;
	bool BoolVal = false;

	if (Properties->TryGetNumberField(TEXT("fog_density"), NumVal))
	{
		FogComp->FogDensity = static_cast<float>(NumVal);
		Applied.Add(TEXT("fog_density"));
	}

	if (Properties->TryGetNumberField(TEXT("fog_height_falloff"), NumVal))
	{
		FogComp->FogHeightFalloff = static_cast<float>(NumVal);
		Applied.Add(TEXT("fog_height_falloff"));
	}

	const TSharedPtr<FJsonObject>* InscatterColorObj = nullptr;
	if (Properties->TryGetObjectField(TEXT("fog_inscattering_color"), InscatterColorObj) && InscatterColorObj)
	{
		double R = 0, G = 0, B = 0;
		(*InscatterColorObj)->TryGetNumberField(TEXT("r"), R);
		(*InscatterColorObj)->TryGetNumberField(TEXT("g"), G);
		(*InscatterColorObj)->TryGetNumberField(TEXT("b"), B);
		FogComp->FogInscatteringLuminance = FLinearColor(
			static_cast<float>(R), static_cast<float>(G), static_cast<float>(B));
		Applied.Add(TEXT("fog_inscattering_color"));
	}

	if (Properties->TryGetNumberField(TEXT("fog_max_opacity"), NumVal))
	{
		FogComp->FogMaxOpacity = static_cast<float>(NumVal);
		Applied.Add(TEXT("fog_max_opacity"));
	}

	if (Properties->TryGetNumberField(TEXT("start_distance"), NumVal))
	{
		FogComp->StartDistance = static_cast<float>(NumVal);
		Applied.Add(TEXT("start_distance"));
	}

	if (Properties->TryGetNumberField(TEXT("fog_cutoff_distance"), NumVal))
	{
		FogComp->FogCutoffDistance = static_cast<float>(NumVal);
		Applied.Add(TEXT("fog_cutoff_distance"));
	}

	if (Properties->TryGetNumberField(TEXT("directional_inscattering_exponent"), NumVal))
	{
		FogComp->DirectionalInscatteringExponent = static_cast<float>(NumVal);
		Applied.Add(TEXT("directional_inscattering_exponent"));
	}

	if (Properties->TryGetNumberField(TEXT("directional_inscattering_start_distance"), NumVal))
	{
		FogComp->DirectionalInscatteringStartDistance = static_cast<float>(NumVal);
		Applied.Add(TEXT("directional_inscattering_start_distance"));
	}

	const TSharedPtr<FJsonObject>* DirInscatterColorObj = nullptr;
	if (Properties->TryGetObjectField(TEXT("directional_inscattering_color"), DirInscatterColorObj) && DirInscatterColorObj)
	{
		double R = 0, G = 0, B = 0;
		(*DirInscatterColorObj)->TryGetNumberField(TEXT("r"), R);
		(*DirInscatterColorObj)->TryGetNumberField(TEXT("g"), G);
		(*DirInscatterColorObj)->TryGetNumberField(TEXT("b"), B);
		FogComp->DirectionalInscatteringLuminance = FLinearColor(
			static_cast<float>(R), static_cast<float>(G), static_cast<float>(B));
		Applied.Add(TEXT("directional_inscattering_color"));
	}

	if (Properties->TryGetBoolField(TEXT("volumetric_fog"), BoolVal))
	{
		FogComp->bEnableVolumetricFog = BoolVal;
		Applied.Add(TEXT("volumetric_fog"));
	}

	if (Properties->TryGetNumberField(TEXT("volumetric_fog_scattering_distribution"), NumVal))
	{
		FogComp->VolumetricFogScatteringDistribution = static_cast<float>(NumVal);
		Applied.Add(TEXT("volumetric_fog_scattering_distribution"));
	}

	const TSharedPtr<FJsonObject>* VfAlbedoObj = nullptr;
	if (Properties->TryGetObjectField(TEXT("volumetric_fog_albedo"), VfAlbedoObj) && VfAlbedoObj)
	{
		double R = 0, G = 0, B = 0;
		(*VfAlbedoObj)->TryGetNumberField(TEXT("r"), R);
		(*VfAlbedoObj)->TryGetNumberField(TEXT("g"), G);
		(*VfAlbedoObj)->TryGetNumberField(TEXT("b"), B);
		FogComp->VolumetricFogAlbedo = FColor(
			static_cast<uint8>(FMath::Clamp(R, 0.0, 255.0)),
			static_cast<uint8>(FMath::Clamp(G, 0.0, 255.0)),
			static_cast<uint8>(FMath::Clamp(B, 0.0, 255.0)));
		Applied.Add(TEXT("volumetric_fog_albedo"));
	}

	const TSharedPtr<FJsonObject>* VfEmissiveObj = nullptr;
	if (Properties->TryGetObjectField(TEXT("volumetric_fog_emission"), VfEmissiveObj) && VfEmissiveObj)
	{
		double R = 0, G = 0, B = 0;
		(*VfEmissiveObj)->TryGetNumberField(TEXT("r"), R);
		(*VfEmissiveObj)->TryGetNumberField(TEXT("g"), G);
		(*VfEmissiveObj)->TryGetNumberField(TEXT("b"), B);
		FogComp->VolumetricFogEmissive = FLinearColor(
			static_cast<float>(R), static_cast<float>(G), static_cast<float>(B));
		Applied.Add(TEXT("volumetric_fog_emission"));
	}

	if (Properties->TryGetNumberField(TEXT("volumetric_fog_extinction_scale"), NumVal))
	{
		FogComp->VolumetricFogExtinctionScale = static_cast<float>(NumVal);
		Applied.Add(TEXT("volumetric_fog_extinction_scale"));
	}

	if (Properties->TryGetNumberField(TEXT("volumetric_fog_distance"), NumVal))
	{
		FogComp->VolumetricFogDistance = static_cast<float>(NumVal);
		Applied.Add(TEXT("volumetric_fog_distance"));
	}

	if (Properties->TryGetNumberField(TEXT("second_fog_density"), NumVal))
	{
		FogComp->SecondFogData.FogDensity = static_cast<float>(NumVal);
		Applied.Add(TEXT("second_fog_density"));
	}

	if (Properties->TryGetNumberField(TEXT("second_fog_height_falloff"), NumVal))
	{
		FogComp->SecondFogData.FogHeightFalloff = static_cast<float>(NumVal);
		Applied.Add(TEXT("second_fog_height_falloff"));
	}

	if (Properties->TryGetNumberField(TEXT("second_fog_height_offset"), NumVal))
	{
		FogComp->SecondFogData.FogHeightOffset = static_cast<float>(NumVal);
		Applied.Add(TEXT("second_fog_height_offset"));
	}

	FogComp->MarkRenderStateDirty();
	FoundActor->MarkPackageDirty();

	TSharedPtr<FJsonObject> Result = SuccessResult(FString::Printf(
		TEXT("Set %d properties on ExponentialHeightFog '%s'"), Applied.Num(), *ActorNameOrLabel));
	Result->SetStringField(TEXT("actor_name"), FoundActor->GetName());

	TArray<TSharedPtr<FJsonValue>> PropsArray;
	for (const FString& PropName : Applied)
	{
		PropsArray.Add(MakeShared<FJsonValueString>(PropName));
	}
	Result->SetArrayField(TEXT("properties_set"), PropsArray);

	return Result;
}

TSharedPtr<FJsonObject> FLightingEditor::InspectPostProcess(UWorld* World, const FString& ActorNameOrLabel)
{
	if (!World)
	{
		return ErrorResult(TEXT("World is null"));
	}
	if (ActorNameOrLabel.IsEmpty())
	{
		return ErrorResult(TEXT("Actor name/label is required"));
	}

	AActor* FoundActor = FindLightingActorByNameOrLabel(World, ActorNameOrLabel);
	if (!FoundActor)
	{
		return ErrorResult(FString::Printf(TEXT("Actor not found: %s"), *ActorNameOrLabel));
	}

	APostProcessVolume* PPV = Cast<APostProcessVolume>(FoundActor);
	if (!PPV)
	{
		return ErrorResult(FString::Printf(TEXT("Actor '%s' is not a PostProcessVolume (class: %s)"),
			*ActorNameOrLabel, *FoundActor->GetClass()->GetName()));
	}

	TSharedPtr<FJsonObject> Result = SuccessResult(TEXT("Post-process volume inspected"));
	Result->SetStringField(TEXT("actor_name"), PPV->GetName());
	Result->SetStringField(TEXT("actor_label"), PPV->GetActorLabel());
	Result->SetStringField(TEXT("class"), PPV->GetClass()->GetName());
	AddActorTransform(Result, PPV);

	Result->SetBoolField(TEXT("is_infinite"), PPV->bUnbound);
	Result->SetNumberField(TEXT("blend_radius"), PPV->BlendRadius);
	Result->SetNumberField(TEXT("blend_weight"), PPV->BlendWeight);
	Result->SetNumberField(TEXT("priority"), PPV->Priority);
	Result->SetBoolField(TEXT("is_enabled"), PPV->bEnabled);

	const FPostProcessSettings& S = PPV->Settings;

	auto Vec4ToJson = [](const FVector4& V) -> TSharedPtr<FJsonObject>
	{
		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetNumberField(TEXT("r"), V.X);
		Obj->SetNumberField(TEXT("g"), V.Y);
		Obj->SetNumberField(TEXT("b"), V.Z);
		Obj->SetNumberField(TEXT("a"), V.W);
		return Obj;
	};

	// Bloom
	{
		TSharedPtr<FJsonObject> BloomObj = MakeShared<FJsonObject>();
		bool bHasBloom = false;
		if (S.bOverride_BloomIntensity)
		{
			BloomObj->SetBoolField(TEXT("enabled"), true);
			BloomObj->SetNumberField(TEXT("intensity"), S.BloomIntensity);
			bHasBloom = true;
		}
		if (S.bOverride_BloomThreshold)
		{
			BloomObj->SetNumberField(TEXT("threshold"), S.BloomThreshold);
			bHasBloom = true;
		}
		if (S.bOverride_BloomMethod)
		{
			FString MethodStr = (S.BloomMethod == EBloomMethod::BM_FFT) ? TEXT("convolution") : TEXT("standard");
			BloomObj->SetStringField(TEXT("method"), MethodStr);
			bHasBloom = true;
		}
		if (bHasBloom)
		{
			Result->SetObjectField(TEXT("bloom"), BloomObj);
		}
	}

	// Depth of Field
	{
		TSharedPtr<FJsonObject> DOFObj = MakeShared<FJsonObject>();
		bool bHasDOF = false;
		if (S.bOverride_DepthOfFieldFocalDistance)
		{
			DOFObj->SetNumberField(TEXT("focal_distance"), S.DepthOfFieldFocalDistance);
			bHasDOF = true;
		}
		if (S.bOverride_DepthOfFieldFstop)
		{
			DOFObj->SetNumberField(TEXT("fstop"), S.DepthOfFieldFstop);
			bHasDOF = true;
		}
		if (S.bOverride_DepthOfFieldSensorWidth)
		{
			DOFObj->SetNumberField(TEXT("sensor_width"), S.DepthOfFieldSensorWidth);
			bHasDOF = true;
		}
		if (S.bOverride_DepthOfFieldMinFstop)
		{
			DOFObj->SetNumberField(TEXT("min_fstop"), S.DepthOfFieldMinFstop);
			bHasDOF = true;
		}
		if (S.bOverride_DepthOfFieldBladeCount)
		{
			DOFObj->SetNumberField(TEXT("blade_count"), S.DepthOfFieldBladeCount);
			bHasDOF = true;
		}
		if (bHasDOF)
		{
			Result->SetObjectField(TEXT("depth_of_field"), DOFObj);
		}
	}

	// Motion Blur
	{
		TSharedPtr<FJsonObject> MBObj = MakeShared<FJsonObject>();
		bool bHasMB = false;
		if (S.bOverride_MotionBlurAmount)
		{
			MBObj->SetNumberField(TEXT("amount"), S.MotionBlurAmount);
			bHasMB = true;
		}
		if (S.bOverride_MotionBlurMax)
		{
			MBObj->SetNumberField(TEXT("max"), S.MotionBlurMax);
			bHasMB = true;
		}
		if (bHasMB)
		{
			Result->SetObjectField(TEXT("motion_blur"), MBObj);
		}
	}

	// Exposure
	{
		TSharedPtr<FJsonObject> ExpObj = MakeShared<FJsonObject>();
		bool bHasExp = false;
		if (S.bOverride_AutoExposureBias)
		{
			ExpObj->SetNumberField(TEXT("compensation"), S.AutoExposureBias);
			bHasExp = true;
		}
		if (S.bOverride_AutoExposureMinBrightness)
		{
			ExpObj->SetNumberField(TEXT("min_brightness"), S.AutoExposureMinBrightness);
			bHasExp = true;
		}
		if (S.bOverride_AutoExposureMaxBrightness)
		{
			ExpObj->SetNumberField(TEXT("max_brightness"), S.AutoExposureMaxBrightness);
			bHasExp = true;
		}
		if (S.bOverride_AutoExposureSpeedUp)
		{
			ExpObj->SetNumberField(TEXT("speed_up"), S.AutoExposureSpeedUp);
			bHasExp = true;
		}
		if (S.bOverride_AutoExposureSpeedDown)
		{
			ExpObj->SetNumberField(TEXT("speed_down"), S.AutoExposureSpeedDown);
			bHasExp = true;
		}
		if (bHasExp)
		{
			Result->SetObjectField(TEXT("exposure"), ExpObj);
		}
	}

	// Color Grading
	{
		TSharedPtr<FJsonObject> CGObj = MakeShared<FJsonObject>();
		bool bHasCG = false;
		if (S.bOverride_WhiteTemp)
		{
			CGObj->SetNumberField(TEXT("white_balance_temp"), S.WhiteTemp);
			bHasCG = true;
		}
		if (S.bOverride_WhiteTint)
		{
			CGObj->SetNumberField(TEXT("white_balance_tint"), S.WhiteTint);
			bHasCG = true;
		}
		if (S.bOverride_ColorSaturation)
		{
			CGObj->SetObjectField(TEXT("saturation"), Vec4ToJson(S.ColorSaturation));
			bHasCG = true;
		}
		if (S.bOverride_ColorContrast)
		{
			CGObj->SetObjectField(TEXT("contrast"), Vec4ToJson(S.ColorContrast));
			bHasCG = true;
		}
		if (S.bOverride_ColorGamma)
		{
			CGObj->SetObjectField(TEXT("gamma"), Vec4ToJson(S.ColorGamma));
			bHasCG = true;
		}
		if (S.bOverride_ColorGain)
		{
			CGObj->SetObjectField(TEXT("gain"), Vec4ToJson(S.ColorGain));
			bHasCG = true;
		}
		if (S.bOverride_ColorGradingLUT)
		{
			FString LUTPath = S.ColorGradingLUT ? S.ColorGradingLUT->GetPathName() : TEXT("None");
			CGObj->SetStringField(TEXT("color_grading_lut"), LUTPath);
			bHasCG = true;
		}
		if (bHasCG)
		{
			Result->SetObjectField(TEXT("color_grading"), CGObj);
		}
	}

	// Vignette
	if (S.bOverride_VignetteIntensity)
	{
		TSharedPtr<FJsonObject> VigObj = MakeShared<FJsonObject>();
		VigObj->SetNumberField(TEXT("intensity"), S.VignetteIntensity);
		Result->SetObjectField(TEXT("vignette"), VigObj);
	}

	// Ambient Occlusion
	{
		TSharedPtr<FJsonObject> AOObj = MakeShared<FJsonObject>();
		bool bHasAO = false;
		if (S.bOverride_AmbientOcclusionIntensity)
		{
			AOObj->SetNumberField(TEXT("intensity"), S.AmbientOcclusionIntensity);
			bHasAO = true;
		}
		if (S.bOverride_AmbientOcclusionRadius)
		{
			AOObj->SetNumberField(TEXT("radius"), S.AmbientOcclusionRadius);
			bHasAO = true;
		}
		if (bHasAO)
		{
			Result->SetObjectField(TEXT("ambient_occlusion"), AOObj);
		}
	}

	return Result;
}

TSharedPtr<FJsonObject> FLightingEditor::SetPostProcess(UWorld* World, const FString& ActorNameOrLabel,
	const TSharedPtr<FJsonObject>& Settings)
{
	if (!World)
	{
		return ErrorResult(TEXT("World is null"));
	}
	if (ActorNameOrLabel.IsEmpty())
	{
		return ErrorResult(TEXT("Actor name/label is required"));
	}
	if (!Settings.IsValid())
	{
		return ErrorResult(TEXT("Settings object is required"));
	}

	AActor* FoundActor = FindLightingActorByNameOrLabel(World, ActorNameOrLabel);
	if (!FoundActor)
	{
		return ErrorResult(FString::Printf(TEXT("Actor not found: %s"), *ActorNameOrLabel));
	}

	APostProcessVolume* PPV = Cast<APostProcessVolume>(FoundActor);
	if (!PPV)
	{
		return ErrorResult(FString::Printf(TEXT("Actor '%s' is not a PostProcessVolume (class: %s)"),
			*ActorNameOrLabel, *FoundActor->GetClass()->GetName()));
	}

	TArray<FString> Applied = ApplyPostProcessSettingsFromJson(PPV->Settings, Settings);

	PPV->MarkPackageDirty();

	TSharedPtr<FJsonObject> Result = SuccessResult(FString::Printf(
		TEXT("Set %d post-process settings on '%s'"), Applied.Num(), *ActorNameOrLabel));
	Result->SetStringField(TEXT("actor_name"), PPV->GetName());
	Result->SetStringField(TEXT("actor_label"), PPV->GetActorLabel());

	TArray<TSharedPtr<FJsonValue>> AppliedArray;
	for (const FString& Key : Applied)
	{
		AppliedArray.Add(MakeShared<FJsonValueString>(Key));
	}
	Result->SetArrayField(TEXT("settings_applied"), AppliedArray);

	return Result;
}

TSharedPtr<FJsonObject> FLightingEditor::SpawnPostProcessVolume(UWorld* World, const FVector& Location,
	const FVector& Extent, bool bInfinite, const FString& Label, const TSharedPtr<FJsonObject>& Settings)
{
	if (!World)
	{
		return ErrorResult(TEXT("World is null"));
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APostProcessVolume* PPV = World->SpawnActor<APostProcessVolume>(Location, FRotator::ZeroRotator, SpawnParams);
	if (!PPV)
	{
		return ErrorResult(TEXT("Failed to spawn PostProcessVolume actor"));
	}

	PPV->bUnbound = bInfinite;

	if (!Label.IsEmpty())
	{
		PPV->SetActorLabel(Label);
	}

	if (Settings.IsValid())
	{
		ApplyPostProcessSettingsFromJson(PPV->Settings, Settings);
	}

	PPV->MarkPackageDirty();
	World->MarkPackageDirty();

	TSharedPtr<FJsonObject> Result = SuccessResult(TEXT("Spawned PostProcessVolume"));
	Result->SetStringField(TEXT("actor_name"), PPV->GetName());
	Result->SetStringField(TEXT("actor_label"), PPV->GetActorLabel());
	AddActorTransform(Result, PPV);
	Result->SetBoolField(TEXT("is_infinite"), PPV->bUnbound);

	return Result;
}
