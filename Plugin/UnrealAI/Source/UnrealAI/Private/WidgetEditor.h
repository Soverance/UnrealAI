// Copyright Natali Caggiano. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class UWidgetBlueprint;
class UWidget;
class UPanelWidget;

/**
 * Utility class for Widget Blueprint read/write operations.
 * Covers: inspect tree, get properties, create WBP, add/remove widgets,
 * set visual properties, set slot properties, save, batch.
 *
 * All methods are static, return JSON, and contain no MCP/JSON dispatch logic.
 */
class FWidgetEditor
{
public:
	// ===== Read Operations =====

	static TSharedPtr<FJsonObject> InspectWidgetTree(const FString& AssetPath);
	static TSharedPtr<FJsonObject> GetWidgetProperties(const FString& AssetPath, const FString& WidgetName);

	// ===== Write Operations =====

	static TSharedPtr<FJsonObject> CreateWidgetBlueprint(const FString& Name, const FString& PackagePath,
		const FString& ParentClass, const FString& RootWidgetClass);
	static TSharedPtr<FJsonObject> AddWidget(UWidgetBlueprint* WBP, const FString& WidgetClass,
		const FString& WidgetName, const FString& ParentName);
	static TSharedPtr<FJsonObject> RemoveWidget(UWidgetBlueprint* WBP, const FString& WidgetName);
	static TSharedPtr<FJsonObject> SetWidgetProperty(UWidgetBlueprint* WBP, const FString& WidgetName,
		const TSharedPtr<FJsonObject>& Properties);
	static TSharedPtr<FJsonObject> SetSlotProperty(UWidgetBlueprint* WBP, const FString& WidgetName,
		const TSharedPtr<FJsonObject>& SlotProperties);
	static TSharedPtr<FJsonObject> SetBrush(UWidgetBlueprint* WBP, const FString& WidgetName,
		const FString& BrushProperty, const TSharedPtr<FJsonObject>& BrushJson);
	static TSharedPtr<FJsonObject> SaveWidgetBlueprint(UWidgetBlueprint* WBP);

	// ===== Structural Operations =====

	static TSharedPtr<FJsonObject> ReparentWidget(UWidgetBlueprint* WBP, const FString& WidgetName, const FString& NewParentName);
	static TSharedPtr<FJsonObject> ReorderChild(UWidgetBlueprint* WBP, const FString& ParentName, const FString& ChildName, int32 NewIndex);
	static TSharedPtr<FJsonObject> CloneWidget(UWidgetBlueprint* WBP, const FString& SourceWidgetName, const FString& NewName, const FString& TargetParentName);

	// ===== Event Binding =====

	static TSharedPtr<FJsonObject> ListWidgetEvents(const FString& AssetPath, const FString& WidgetName);
	static TSharedPtr<FJsonObject> BindEvent(UWidgetBlueprint* WBP, const FString& WidgetName,
		const FString& EventName, const FString& FunctionName);

	// ===== Property Binding =====

	static TSharedPtr<FJsonObject> BindProperty(UWidgetBlueprint* WBP, const FString& WidgetName,
		const FString& PropertyName, const FString& FunctionName);
	static TSharedPtr<FJsonObject> UnbindProperty(UWidgetBlueprint* WBP, const FString& WidgetName,
		const FString& PropertyName);
	static TSharedPtr<FJsonObject> ListBindings(const FString& AssetPath);

	// ===== Animation Operations =====

	static TSharedPtr<FJsonObject> ListAnimations(const FString& AssetPath);
	static TSharedPtr<FJsonObject> InspectAnimation(const FString& AssetPath, const FString& AnimationName);
	static TSharedPtr<FJsonObject> CreateAnimation(UWidgetBlueprint* WBP, const FString& AnimationName, float Length);
	static TSharedPtr<FJsonObject> AddAnimationTrack(UWidgetBlueprint* WBP, const FString& AnimationName,
		const FString& WidgetName, const FString& TrackType, const TArray<TSharedPtr<FJsonValue>>& Keyframes);

	// ===== Batch =====

	static TSharedPtr<FJsonObject> ExecuteBatch(UWidgetBlueprint* WBP, const TArray<TSharedPtr<FJsonValue>>& Operations);

	// ===== Asset Loading =====

	static UWidgetBlueprint* LoadWidgetBlueprint(const FString& Path, FString& OutError);

	// ===== Helpers =====

	static UClass* ResolveWidgetClass(const FString& ShortName);
	static FLinearColor ParseColor(const FString& ColorStr);

private:
	static TSharedPtr<FJsonObject> SuccessResult(const FString& Message);
	static TSharedPtr<FJsonObject> ErrorResult(const FString& Message);

	static TSharedPtr<FJsonObject> SerializeWidgetTree(UWidget* Widget, UWidgetBlueprint* WBP);
	static TSharedPtr<FJsonObject> SerializeWidgetProperties(UWidget* Widget);
	static TSharedPtr<FJsonObject> SerializeSlotProperties(UWidget* Widget);

	static TSharedPtr<FJsonObject> DispatchBatchOp(UWidgetBlueprint* WBP, const TSharedPtr<FJsonObject>& OpData);
};
