// Copyright Natali Caggiano. All Rights Reserved.

#include "MCPTool_Widget.h"
#include "WidgetEditor.h"

static FMCPToolResult WidgetJsonToToolResult(const TSharedPtr<FJsonObject>& Result, const FString& SuccessContext)
{
	if (!Result)
	{
		return FMCPToolResult::Error(TEXT("Operation returned null result"));
	}

	bool bSuccess = false;
	Result->TryGetBoolField(TEXT("success"), bSuccess);

	if (bSuccess)
	{
		FString Message;
		Result->TryGetStringField(TEXT("message"), Message);
		if (Message.IsEmpty())
		{
			Message = SuccessContext;
		}
		return FMCPToolResult::Success(Message, Result);
	}
	else
	{
		FString Error;
		Result->TryGetStringField(TEXT("error"), Error);
		return FMCPToolResult::Error(Error.IsEmpty() ? TEXT("Unknown error") : Error);
	}
}

// ============================================================================
// Main Dispatch
// ============================================================================

FMCPToolResult FMCPTool_Widget::Execute(const TSharedRef<FJsonObject>& Params)
{
	FString Operation;
	TOptional<FMCPToolResult> Error;
	if (!ExtractRequiredString(Params, TEXT("operation"), Operation, Error))
	{
		return Error.GetValue();
	}

	Operation = Operation.ToLower();

	if (Operation == TEXT("inspect"))
	{
		return HandleInspect(Params);
	}
	else if (Operation == TEXT("get_properties"))
	{
		return HandleGetProperties(Params);
	}
	else if (Operation == TEXT("create"))
	{
		return HandleCreate(Params);
	}
	else if (Operation == TEXT("add_widget"))
	{
		return HandleAddWidget(Params);
	}
	else if (Operation == TEXT("remove_widget"))
	{
		return HandleRemoveWidget(Params);
	}
	else if (Operation == TEXT("set_property"))
	{
		return HandleSetProperty(Params);
	}
	else if (Operation == TEXT("set_slot"))
	{
		return HandleSetSlot(Params);
	}
	else if (Operation == TEXT("set_brush"))
	{
		return HandleSetBrush(Params);
	}
	else if (Operation == TEXT("reparent_widget"))
	{
		return HandleReparentWidget(Params);
	}
	else if (Operation == TEXT("reorder_child") || Operation == TEXT("reorder_children"))
	{
		return HandleReorderChild(Params);
	}
	else if (Operation == TEXT("clone_widget"))
	{
		return HandleCloneWidget(Params);
	}
	else if (Operation == TEXT("list_events"))
	{
		return HandleListEvents(Params);
	}
	else if (Operation == TEXT("bind_event"))
	{
		return HandleBindEvent(Params);
	}
	else if (Operation == TEXT("bind_property"))
	{
		return HandleBindProperty(Params);
	}
	else if (Operation == TEXT("unbind_property"))
	{
		return HandleUnbindProperty(Params);
	}
	else if (Operation == TEXT("list_bindings"))
	{
		return HandleListBindings(Params);
	}
	else if (Operation == TEXT("list_animations"))
	{
		return HandleListAnimations(Params);
	}
	else if (Operation == TEXT("inspect_animation"))
	{
		return HandleInspectAnimation(Params);
	}
	else if (Operation == TEXT("create_animation"))
	{
		return HandleCreateAnimation(Params);
	}
	else if (Operation == TEXT("add_animation_track"))
	{
		return HandleAddAnimationTrack(Params);
	}
	else if (Operation == TEXT("save"))
	{
		return HandleSave(Params);
	}
	else if (Operation == TEXT("batch"))
	{
		return HandleBatch(Params);
	}

	return FMCPToolResult::Error(FString::Printf(
		TEXT("Unknown operation: '%s'. Valid: inspect, get_properties, create, add_widget, remove_widget, set_property, set_slot, set_brush, reparent_widget, reorder_child, clone_widget, list_events, bind_event, bind_property, unbind_property, list_bindings, list_animations, inspect_animation, create_animation, add_animation_track, save, batch"),
		*Operation));
}

// ============================================================================
// Read Handlers
// ============================================================================

FMCPToolResult FMCPTool_Widget::HandleInspect(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath;
	TOptional<FMCPToolResult> Error;
	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}

	TSharedPtr<FJsonObject> Result = FWidgetEditor::InspectWidgetTree(AssetPath);
	return WidgetJsonToToolResult(Result, TEXT("Widget tree inspected"));
}

FMCPToolResult FMCPTool_Widget::HandleGetProperties(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath, WidgetName;
	TOptional<FMCPToolResult> Error;
	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("widget_name"), WidgetName, Error))
	{
		return Error.GetValue();
	}

	TSharedPtr<FJsonObject> Result = FWidgetEditor::GetWidgetProperties(AssetPath, WidgetName);
	return WidgetJsonToToolResult(Result, TEXT("Widget properties retrieved"));
}

// ============================================================================
// Write Handlers
// ============================================================================

FMCPToolResult FMCPTool_Widget::HandleCreate(const TSharedRef<FJsonObject>& Params)
{
	FString Name, PackagePath;
	TOptional<FMCPToolResult> Error;

	if (!ExtractRequiredString(Params, TEXT("name"), Name, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("package_path"), PackagePath, Error))
	{
		return Error.GetValue();
	}

	FString ParentClass = ExtractOptionalString(Params, TEXT("parent_class"), TEXT("UserWidget"));
	FString RootWidgetClass = ExtractOptionalString(Params, TEXT("root_widget_class"), TEXT("CanvasPanel"));

	TSharedPtr<FJsonObject> Result = FWidgetEditor::CreateWidgetBlueprint(Name, PackagePath, ParentClass, RootWidgetClass);
	return WidgetJsonToToolResult(Result, TEXT("Widget Blueprint created"));
}

FMCPToolResult FMCPTool_Widget::HandleAddWidget(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath, WidgetClass, WidgetName;
	TOptional<FMCPToolResult> Error;

	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("widget_class"), WidgetClass, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("widget_name"), WidgetName, Error))
	{
		return Error.GetValue();
	}

	FString LoadError;
	UWidgetBlueprint* WBP = FWidgetEditor::LoadWidgetBlueprint(AssetPath, LoadError);
	if (!WBP)
	{
		return FMCPToolResult::Error(LoadError);
	}

	FString ParentName = ExtractOptionalString(Params, TEXT("parent_name"), FString());

	TSharedPtr<FJsonObject> Result = FWidgetEditor::AddWidget(WBP, WidgetClass, WidgetName, ParentName);
	return WidgetJsonToToolResult(Result, TEXT("Widget added"));
}

FMCPToolResult FMCPTool_Widget::HandleRemoveWidget(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath, WidgetName;
	TOptional<FMCPToolResult> Error;

	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("widget_name"), WidgetName, Error))
	{
		return Error.GetValue();
	}

	FString LoadError;
	UWidgetBlueprint* WBP = FWidgetEditor::LoadWidgetBlueprint(AssetPath, LoadError);
	if (!WBP)
	{
		return FMCPToolResult::Error(LoadError);
	}

	TSharedPtr<FJsonObject> Result = FWidgetEditor::RemoveWidget(WBP, WidgetName);
	return WidgetJsonToToolResult(Result, TEXT("Widget removed"));
}

FMCPToolResult FMCPTool_Widget::HandleSetProperty(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath, WidgetName;
	TOptional<FMCPToolResult> Error;

	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("widget_name"), WidgetName, Error))
	{
		return Error.GetValue();
	}

	const TSharedPtr<FJsonObject>* PropsObj = nullptr;
	if (!Params->TryGetObjectField(TEXT("properties"), PropsObj) || !PropsObj)
	{
		return FMCPToolResult::Error(TEXT("Missing required parameter: properties (JSON object)"));
	}

	FString LoadError;
	UWidgetBlueprint* WBP = FWidgetEditor::LoadWidgetBlueprint(AssetPath, LoadError);
	if (!WBP)
	{
		return FMCPToolResult::Error(LoadError);
	}

	TSharedPtr<FJsonObject> Result = FWidgetEditor::SetWidgetProperty(WBP, WidgetName, *PropsObj);
	return WidgetJsonToToolResult(Result, TEXT("Properties set"));
}

FMCPToolResult FMCPTool_Widget::HandleSetSlot(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath, WidgetName;
	TOptional<FMCPToolResult> Error;

	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("widget_name"), WidgetName, Error))
	{
		return Error.GetValue();
	}

	const TSharedPtr<FJsonObject>* SlotObj = nullptr;
	if (!Params->TryGetObjectField(TEXT("slot_properties"), SlotObj) || !SlotObj)
	{
		return FMCPToolResult::Error(TEXT("Missing required parameter: slot_properties (JSON object)"));
	}

	FString LoadError;
	UWidgetBlueprint* WBP = FWidgetEditor::LoadWidgetBlueprint(AssetPath, LoadError);
	if (!WBP)
	{
		return FMCPToolResult::Error(LoadError);
	}

	TSharedPtr<FJsonObject> Result = FWidgetEditor::SetSlotProperty(WBP, WidgetName, *SlotObj);
	return WidgetJsonToToolResult(Result, TEXT("Slot properties set"));
}

FMCPToolResult FMCPTool_Widget::HandleSetBrush(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath, WidgetName;
	TOptional<FMCPToolResult> Error;

	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("widget_name"), WidgetName, Error))
	{
		return Error.GetValue();
	}

	const TSharedPtr<FJsonObject>* BrushObj = nullptr;
	if (!Params->TryGetObjectField(TEXT("brush"), BrushObj) || !BrushObj)
	{
		return FMCPToolResult::Error(TEXT("Missing required parameter: brush (JSON object)"));
	}

	FString LoadError;
	UWidgetBlueprint* WBP = FWidgetEditor::LoadWidgetBlueprint(AssetPath, LoadError);
	if (!WBP)
	{
		return FMCPToolResult::Error(LoadError);
	}

	FString BrushProperty = ExtractOptionalString(Params, TEXT("brush_property"), FString());

	TSharedPtr<FJsonObject> Result = FWidgetEditor::SetBrush(WBP, WidgetName, BrushProperty, *BrushObj);
	return WidgetJsonToToolResult(Result, TEXT("Brush set"));
}

FMCPToolResult FMCPTool_Widget::HandleReparentWidget(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath, WidgetName, NewParentName;
	TOptional<FMCPToolResult> Error;

	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("widget_name"), WidgetName, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("new_parent_name"), NewParentName, Error))
	{
		return Error.GetValue();
	}

	FString LoadError;
	UWidgetBlueprint* WBP = FWidgetEditor::LoadWidgetBlueprint(AssetPath, LoadError);
	if (!WBP)
	{
		return FMCPToolResult::Error(LoadError);
	}

	TSharedPtr<FJsonObject> Result = FWidgetEditor::ReparentWidget(WBP, WidgetName, NewParentName);
	return WidgetJsonToToolResult(Result, TEXT("Widget reparented"));
}

FMCPToolResult FMCPTool_Widget::HandleReorderChild(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath, ParentName, ChildName;
	TOptional<FMCPToolResult> Error;

	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("parent_name"), ParentName, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("child_name"), ChildName, Error))
	{
		return Error.GetValue();
	}

	double NewIndex = 0;
	if (!Params->TryGetNumberField(TEXT("new_index"), NewIndex))
	{
		return FMCPToolResult::Error(TEXT("Missing required parameter: new_index"));
	}

	FString LoadError;
	UWidgetBlueprint* WBP = FWidgetEditor::LoadWidgetBlueprint(AssetPath, LoadError);
	if (!WBP)
	{
		return FMCPToolResult::Error(LoadError);
	}

	TSharedPtr<FJsonObject> Result = FWidgetEditor::ReorderChild(WBP, ParentName, ChildName, static_cast<int32>(NewIndex));
	return WidgetJsonToToolResult(Result, TEXT("Child reordered"));
}

FMCPToolResult FMCPTool_Widget::HandleCloneWidget(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath, WidgetName, NewName;
	TOptional<FMCPToolResult> Error;

	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("widget_name"), WidgetName, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("new_name"), NewName, Error))
	{
		return Error.GetValue();
	}

	FString TargetParent = ExtractOptionalString(Params, TEXT("target_parent"), FString());

	FString LoadError;
	UWidgetBlueprint* WBP = FWidgetEditor::LoadWidgetBlueprint(AssetPath, LoadError);
	if (!WBP)
	{
		return FMCPToolResult::Error(LoadError);
	}

	TSharedPtr<FJsonObject> Result = FWidgetEditor::CloneWidget(WBP, WidgetName, NewName, TargetParent);
	return WidgetJsonToToolResult(Result, TEXT("Widget cloned"));
}

FMCPToolResult FMCPTool_Widget::HandleListEvents(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath, WidgetName;
	TOptional<FMCPToolResult> Error;

	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("widget_name"), WidgetName, Error))
	{
		return Error.GetValue();
	}

	TSharedPtr<FJsonObject> Result = FWidgetEditor::ListWidgetEvents(AssetPath, WidgetName);
	return WidgetJsonToToolResult(Result, TEXT("Widget events listed"));
}

FMCPToolResult FMCPTool_Widget::HandleBindEvent(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath, WidgetName, EventName;
	TOptional<FMCPToolResult> Error;

	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("widget_name"), WidgetName, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("event_name"), EventName, Error))
	{
		return Error.GetValue();
	}

	FString FunctionName = ExtractOptionalString(Params, TEXT("function_name"), FString());

	FString LoadError;
	UWidgetBlueprint* WBP = FWidgetEditor::LoadWidgetBlueprint(AssetPath, LoadError);
	if (!WBP)
	{
		return FMCPToolResult::Error(LoadError);
	}

	TSharedPtr<FJsonObject> Result = FWidgetEditor::BindEvent(WBP, WidgetName, EventName, FunctionName);
	return WidgetJsonToToolResult(Result, TEXT("Event bound"));
}

FMCPToolResult FMCPTool_Widget::HandleBindProperty(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath, WidgetName, PropertyName, FunctionName;
	TOptional<FMCPToolResult> Error;

	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("widget_name"), WidgetName, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("property_name"), PropertyName, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("function_name"), FunctionName, Error))
	{
		return Error.GetValue();
	}

	FString LoadError;
	UWidgetBlueprint* WBP = FWidgetEditor::LoadWidgetBlueprint(AssetPath, LoadError);
	if (!WBP)
	{
		return FMCPToolResult::Error(LoadError);
	}

	TSharedPtr<FJsonObject> Result = FWidgetEditor::BindProperty(WBP, WidgetName, PropertyName, FunctionName);
	return WidgetJsonToToolResult(Result, TEXT("Property bound"));
}

FMCPToolResult FMCPTool_Widget::HandleUnbindProperty(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath, WidgetName, PropertyName;
	TOptional<FMCPToolResult> Error;

	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("widget_name"), WidgetName, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("property_name"), PropertyName, Error))
	{
		return Error.GetValue();
	}

	FString LoadError;
	UWidgetBlueprint* WBP = FWidgetEditor::LoadWidgetBlueprint(AssetPath, LoadError);
	if (!WBP)
	{
		return FMCPToolResult::Error(LoadError);
	}

	TSharedPtr<FJsonObject> Result = FWidgetEditor::UnbindProperty(WBP, WidgetName, PropertyName);
	return WidgetJsonToToolResult(Result, TEXT("Property unbound"));
}

FMCPToolResult FMCPTool_Widget::HandleListBindings(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath;
	TOptional<FMCPToolResult> Error;
	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}

	TSharedPtr<FJsonObject> Result = FWidgetEditor::ListBindings(AssetPath);
	return WidgetJsonToToolResult(Result, TEXT("Bindings listed"));
}

FMCPToolResult FMCPTool_Widget::HandleSave(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath;
	TOptional<FMCPToolResult> Error;

	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}

	FString LoadError;
	UWidgetBlueprint* WBP = FWidgetEditor::LoadWidgetBlueprint(AssetPath, LoadError);
	if (!WBP)
	{
		return FMCPToolResult::Error(LoadError);
	}

	TSharedPtr<FJsonObject> Result = FWidgetEditor::SaveWidgetBlueprint(WBP);
	return WidgetJsonToToolResult(Result, TEXT("Widget Blueprint saved"));
}

FMCPToolResult FMCPTool_Widget::HandleBatch(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath;
	TOptional<FMCPToolResult> Error;

	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}

	FString LoadError;
	UWidgetBlueprint* WBP = FWidgetEditor::LoadWidgetBlueprint(AssetPath, LoadError);
	if (!WBP)
	{
		return FMCPToolResult::Error(LoadError);
	}

	const TArray<TSharedPtr<FJsonValue>>* OpsArray = nullptr;
	if (!Params->TryGetArrayField(TEXT("operations"), OpsArray) || !OpsArray)
	{
		return FMCPToolResult::Error(TEXT("Missing required parameter: operations (JSON array)"));
	}

	TSharedPtr<FJsonObject> Result = FWidgetEditor::ExecuteBatch(WBP, *OpsArray);
	if (!Result)
	{
		return FMCPToolResult::Error(TEXT("Batch operation returned null result"));
	}

	bool bSuccess = false;
	Result->TryGetBoolField(TEXT("success"), bSuccess);

	int32 OKCount = static_cast<int32>(Result->GetNumberField(TEXT("ok_count")));
	int32 ErrCount = static_cast<int32>(Result->GetNumberField(TEXT("error_count")));
	int32 Total = static_cast<int32>(Result->GetNumberField(TEXT("total")));

	FString Message = FString::Printf(TEXT("Batch: %d OK, %d errors, %d total"), OKCount, ErrCount, Total);

	if (bSuccess)
	{
		return FMCPToolResult::Success(Message, Result);
	}
	else
	{
		FMCPToolResult PartialResult;
		PartialResult.bSuccess = false;
		PartialResult.Message = Message;
		PartialResult.Data = Result;
		return PartialResult;
	}
}

// ============================================================================
// Animation Handlers
// ============================================================================

FMCPToolResult FMCPTool_Widget::HandleListAnimations(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath;
	TOptional<FMCPToolResult> Error;
	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}

	TSharedPtr<FJsonObject> Result = FWidgetEditor::ListAnimations(AssetPath);
	return WidgetJsonToToolResult(Result, TEXT("Animations listed"));
}

FMCPToolResult FMCPTool_Widget::HandleInspectAnimation(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath, AnimationName;
	TOptional<FMCPToolResult> Error;
	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("animation_name"), AnimationName, Error))
	{
		return Error.GetValue();
	}

	TSharedPtr<FJsonObject> Result = FWidgetEditor::InspectAnimation(AssetPath, AnimationName);
	return WidgetJsonToToolResult(Result, TEXT("Animation inspected"));
}

FMCPToolResult FMCPTool_Widget::HandleCreateAnimation(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath, AnimationName;
	TOptional<FMCPToolResult> Error;

	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("animation_name"), AnimationName, Error))
	{
		return Error.GetValue();
	}

	double Length = 1.0;
	Params->TryGetNumberField(TEXT("length"), Length);

	FString LoadError;
	UWidgetBlueprint* WBP = FWidgetEditor::LoadWidgetBlueprint(AssetPath, LoadError);
	if (!WBP)
	{
		return FMCPToolResult::Error(LoadError);
	}

	TSharedPtr<FJsonObject> Result = FWidgetEditor::CreateAnimation(WBP, AnimationName, static_cast<float>(Length));
	return WidgetJsonToToolResult(Result, TEXT("Animation created"));
}

FMCPToolResult FMCPTool_Widget::HandleAddAnimationTrack(const TSharedRef<FJsonObject>& Params)
{
	FString AssetPath, AnimationName, WidgetName, TrackType;
	TOptional<FMCPToolResult> Error;

	if (!ExtractRequiredString(Params, TEXT("asset_path"), AssetPath, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("animation_name"), AnimationName, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("widget_name"), WidgetName, Error))
	{
		return Error.GetValue();
	}
	if (!ExtractRequiredString(Params, TEXT("track_type"), TrackType, Error))
	{
		return Error.GetValue();
	}

	const TArray<TSharedPtr<FJsonValue>>* KeyframesArray = nullptr;
	if (!Params->TryGetArrayField(TEXT("keyframes"), KeyframesArray) || !KeyframesArray)
	{
		return FMCPToolResult::Error(TEXT("Missing required parameter: keyframes (JSON array)"));
	}

	FString LoadError;
	UWidgetBlueprint* WBP = FWidgetEditor::LoadWidgetBlueprint(AssetPath, LoadError);
	if (!WBP)
	{
		return FMCPToolResult::Error(LoadError);
	}

	TSharedPtr<FJsonObject> Result = FWidgetEditor::AddAnimationTrack(WBP, AnimationName, WidgetName, TrackType, *KeyframesArray);
	return WidgetJsonToToolResult(Result, TEXT("Animation track added"));
}
