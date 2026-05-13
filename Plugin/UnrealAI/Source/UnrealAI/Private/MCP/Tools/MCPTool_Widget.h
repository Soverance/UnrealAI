// Copyright Natali Caggiano. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MCP/MCPToolBase.h"

/**
 * MCP Tool: Widget Blueprint read + write operations.
 *
 * Read Operations:
 *   - 'inspect': Full widget tree of a Widget Blueprint
 *   - 'get_properties': Detailed properties of a single widget
 *
 * Write Operations:
 *   - 'create': Create a new Widget Blueprint
 *   - 'add_widget': Add a widget to the tree
 *   - 'remove_widget': Remove a widget from the tree
 *   - 'set_property': Set visual/behavioral properties on a widget
 *   - 'set_slot': Set layout slot properties (position, anchors, size)
 *   - 'save': Save Widget Blueprint to disk
 *   - 'batch': Multiple operations in sequence
 */
class FMCPTool_Widget : public FMCPToolBase
{
public:
	virtual FMCPToolInfo GetInfo() const override
	{
		FMCPToolInfo Info;
		Info.Name = TEXT("widget_editor");
		Info.Description = TEXT(
			"Widget Blueprint read + write tool.\n\n"
			"READ OPERATIONS:\n"
			"- 'inspect': Full widget tree — names, classes, slots, visual properties, children.\n"
			"  Params: asset_path (required)\n\n"
			"- 'get_properties': Detailed properties of a single widget by name.\n"
			"  Params: asset_path, widget_name (required)\n\n"
			"WRITE OPERATIONS:\n"
			"- 'create': Create a new Widget Blueprint.\n"
			"  Params: name, package_path (required); parent_class (optional, default 'UserWidget'), root_widget_class (optional, default 'CanvasPanel')\n\n"
			"- 'add_widget': Add a widget to the tree.\n"
			"  Params: asset_path, widget_class, widget_name (required); parent_name (optional, defaults to root)\n"
			"  Supported classes: CanvasPanel, Overlay, VerticalBox, HorizontalBox, SizeBox, Border, ProgressBar, TextBlock, Image, Spacer, Button, ScaleBox, GridPanel, WrapBox, "
			"CheckBox, Slider, EditableText, EditableTextBox, ComboBoxString, ScrollBox, RichTextBlock, CircularThrobber, Throbber, BackgroundBlur, InvalidationBox, RetainerBox, WidgetSwitcher, NamedSlot, ListView\n\n"
			"- 'remove_widget': Remove a widget from the tree.\n"
			"  Params: asset_path, widget_name (required)\n\n"
			"- 'set_property': Set visual/behavioral properties on a widget.\n"
			"  Params: asset_path, widget_name (required), properties (JSON object)\n"
			"  All widgets: visibility, render_opacity, is_enabled\n"
			"  ProgressBar: percent, fill_color (hex/named), bar_fill_type, is_marquee, style_fill_tint (hex/named), style_background_tint (hex/named)\n"
			"  TextBlock: text, color, font_size, justification (Left/Center/Right), auto_wrap\n"
			"  Image: color_and_opacity, brush_texture (asset path)\n"
			"  SizeBox: width_override, height_override, min_desired_width/height, max_desired_width/height\n"
			"  CheckBox: is_checked (Checked/Unchecked/Undetermined or bool)\n"
			"  Slider: value, min_value, max_value, step_size\n"
			"  EditableText: text, hint_text, is_read_only\n"
			"  EditableTextBox: text, hint_text, is_read_only\n"
			"  ComboBoxString: selected_option\n"
			"  ScrollBox: orientation (Horizontal/Vertical), scroll_bar_visibility\n"
			"  RichTextBlock: text\n"
			"  CircularThrobber: number_of_pieces, period\n"
			"  Throbber: number_of_pieces\n"
			"  BackgroundBlur: blur_strength\n"
			"  WidgetSwitcher: active_widget_index\n\n"
			"- 'set_slot': Set layout slot properties.\n"
			"  Params: asset_path, widget_name (required), slot_properties (JSON object)\n"
			"  CanvasPanelSlot: position {x,y}, size {x,y}, anchors {min_x,min_y,max_x,max_y}, alignment {x,y}, auto_size, z_order\n"
			"  OverlaySlot: h_align, v_align, padding {left,top,right,bottom}\n"
			"  VerticalBoxSlot/HorizontalBoxSlot: h_align, v_align, padding, size_rule (Auto/Fill), fill_weight\n"
			"  ScrollBoxSlot: h_align, v_align, padding\n"
			"  GridSlot: row, column, row_span, column_span, h_align, v_align, padding\n\n"
			"- 'set_brush': Set FSlateBrush properties on a brush-bearing widget.\n"
			"  Params: asset_path, widget_name (required); brush_property (optional — which brush); brush (required, JSON object)\n"
			"  Brush JSON: texture (asset path), tint (color obj/string), image_size {x,y}, draw_as (NoDrawType/Box/Border/Image/RoundedBox), tiling (NoTile/Horizontal/Vertical/Both), margin {left,top,right,bottom}\n"
			"  Image: brush (default)\n"
			"  Button: style_normal (default), style_hovered, style_pressed, style_disabled\n"
			"  Border: background (default)\n"
			"  ProgressBar: style_fill (default), style_background\n\n"
			"- 'reparent_widget': Move a widget to a different parent panel.\n"
			"  Params: asset_path, widget_name, new_parent_name (all required)\n\n"
			"- 'reorder_child': Change a child's position within its parent panel.\n"
			"  Params: asset_path, parent_name, child_name, new_index (all required)\n\n"
			"- 'clone_widget': Deep-copy a widget (and its children) into a parent panel.\n"
			"  Params: asset_path, widget_name, new_name (required); target_parent (optional, defaults to source's parent)\n\n"
			"- 'list_events': List available delegate events on a widget.\n"
			"  Params: asset_path, widget_name (required)\n"
			"  Returns event names, owner class, and parameter signatures.\n\n"
			"- 'bind_event': Bind a widget delegate event (creates event handler node in event graph).\n"
			"  Params: asset_path, widget_name, event_name (required); function_name (optional)\n"
			"  Common events: Button (OnClicked, OnPressed, OnReleased, OnHovered, OnUnhovered),\n"
			"  CheckBox (OnCheckStateChanged), Slider (OnValueChanged),\n"
			"  EditableText (OnTextChanged, OnTextCommitted), ComboBoxString (OnSelectionChanged, OnOpening)\n\n"
			"PROPERTY BINDING OPERATIONS:\n"
			"- 'bind_property': Bind a widget property to a Blueprint function for reactive updates.\n"
			"  Params: asset_path, widget_name, property_name, function_name (all required)\n"
			"  Property names use UMG delegate naming (e.g. TextDelegate, PercentDelegate, VisibilityDelegate, ColorAndOpacityDelegate).\n\n"
			"- 'unbind_property': Remove a property binding from a widget.\n"
			"  Params: asset_path, widget_name, property_name (all required)\n\n"
			"- 'list_bindings': List all property bindings in a Widget Blueprint.\n"
			"  Params: asset_path (required)\n"
			"  Returns array of {widget_name, property_name, function_name, kind}.\n\n"
			"- 'save': Save Widget Blueprint to disk.\n"
			"  Params: asset_path (required)\n\n"
			"ANIMATION OPERATIONS:\n"
			"- 'list_animations': List all animations in a Widget Blueprint.\n"
			"  Params: asset_path (required)\n\n"
			"- 'inspect_animation': Detailed info about a specific animation (bindings, tracks, keyframes).\n"
			"  Params: asset_path, animation_name (required)\n\n"
			"- 'create_animation': Create a new UWidgetAnimation.\n"
			"  Params: asset_path, animation_name (required); length (optional, default 1.0 seconds)\n\n"
			"- 'add_animation_track': Add a float property track with keyframes to an animation.\n"
			"  Params: asset_path, animation_name, widget_name, track_type (all required); keyframes (required, array of {time, value, interp})\n"
			"  Track types: opacity, translation_x, translation_y, scale_x, scale_y, rotation\n"
			"  Interp modes: linear, cubic (default), constant\n\n"
			"- 'batch': Multiple ops in sequence, single save at end.\n"
			"  Params: asset_path (required), operations (array of {op, ...params})\n"
			"  Valid batch ops: add_widget, remove_widget, set_property, set_slot, set_brush, reparent_widget, reorder_child, clone_widget"
		);
		Info.Parameters = {
			FMCPToolParameter(TEXT("operation"), TEXT("string"), TEXT("Operation: inspect, get_properties, create, add_widget, remove_widget, set_property, set_slot, set_brush, reparent_widget, reorder_child, clone_widget, list_events, bind_event, bind_property, unbind_property, list_bindings, list_animations, inspect_animation, create_animation, add_animation_track, save, batch"), true),
			FMCPToolParameter(TEXT("asset_path"), TEXT("string"), TEXT("Widget Blueprint asset path")),
			FMCPToolParameter(TEXT("widget_name"), TEXT("string"), TEXT("Name of widget within the tree")),
			FMCPToolParameter(TEXT("widget_class"), TEXT("string"), TEXT("Widget class short name (e.g. ProgressBar, TextBlock)")),
			FMCPToolParameter(TEXT("parent_name"), TEXT("string"), TEXT("Parent widget name (defaults to root)")),
			FMCPToolParameter(TEXT("name"), TEXT("string"), TEXT("Asset name (for create)")),
			FMCPToolParameter(TEXT("package_path"), TEXT("string"), TEXT("Package path for new asset (for create)")),
			FMCPToolParameter(TEXT("parent_class"), TEXT("string"), TEXT("Parent class (for create)"), false, TEXT("UserWidget")),
			FMCPToolParameter(TEXT("root_widget_class"), TEXT("string"), TEXT("Root widget class (for create)"), false, TEXT("CanvasPanel")),
			FMCPToolParameter(TEXT("properties"), TEXT("object"), TEXT("Widget properties to set (for set_property)")),
			FMCPToolParameter(TEXT("slot_properties"), TEXT("object"), TEXT("Slot properties to set (for set_slot)")),
			FMCPToolParameter(TEXT("brush_property"), TEXT("string"), TEXT("Which brush property to target (for set_brush)")),
			FMCPToolParameter(TEXT("brush"), TEXT("object"), TEXT("Brush properties JSON (for set_brush)")),
			FMCPToolParameter(TEXT("operations"), TEXT("array"), TEXT("Batch operations array [{op, ...params}]")),
			FMCPToolParameter(TEXT("new_parent_name"), TEXT("string"), TEXT("New parent panel name (for reparent_widget)")),
			FMCPToolParameter(TEXT("new_index"), TEXT("number"), TEXT("Target child index (for reorder_child)")),
			FMCPToolParameter(TEXT("new_name"), TEXT("string"), TEXT("Name for cloned widget (for clone_widget)")),
			FMCPToolParameter(TEXT("target_parent"), TEXT("string"), TEXT("Target parent for clone (optional, defaults to source's parent)")),
			FMCPToolParameter(TEXT("event_name"), TEXT("string"), TEXT("Delegate event name (e.g. OnClicked, OnHovered) for bind_event")),
			FMCPToolParameter(TEXT("function_name"), TEXT("string"), TEXT("Function to connect to event (optional, for bind_event)")),
			FMCPToolParameter(TEXT("animation_name"), TEXT("string"), TEXT("Animation name (for animation operations)")),
			FMCPToolParameter(TEXT("length"), TEXT("number"), TEXT("Animation length in seconds (for create_animation)"), false, TEXT("1.0")),
			FMCPToolParameter(TEXT("track_type"), TEXT("string"), TEXT("Track type: opacity, translation_x, translation_y, scale_x, scale_y, rotation")),
			FMCPToolParameter(TEXT("keyframes"), TEXT("array"), TEXT("Array of keyframe objects: [{time, value, interp}]"))
		};
		Info.Annotations = FMCPToolAnnotations::Modifying();
		return Info;
	}

	virtual FMCPToolResult Execute(const TSharedRef<FJsonObject>& Params) override;

private:
	FMCPToolResult HandleInspect(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleGetProperties(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleCreate(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleAddWidget(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleRemoveWidget(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleSetProperty(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleSetSlot(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleSetBrush(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleReparentWidget(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleReorderChild(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleCloneWidget(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleListEvents(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleBindEvent(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleBindProperty(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleUnbindProperty(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleListBindings(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleSave(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleBatch(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleListAnimations(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleInspectAnimation(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleCreateAnimation(const TSharedRef<FJsonObject>& Params);
	FMCPToolResult HandleAddAnimationTrack(const TSharedRef<FJsonObject>& Params);
};
