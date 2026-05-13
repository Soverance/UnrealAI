// Copyright Natali Caggiano. All Rights Reserved.

#include "WidgetEditor.h"

#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Spacer.h"
#include "Components/Button.h"
#include "Components/ScaleBox.h"
#include "Components/GridPanel.h"
#include "Components/WrapBox.h"
#include "Components/CheckBox.h"
#include "Components/Slider.h"
#include "Components/EditableText.h"
#include "Components/EditableTextBox.h"
#include "Components/ComboBoxString.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/RichTextBlock.h"
#include "Components/CircularThrobber.h"
#include "Components/Throbber.h"
#include "Components/BackgroundBlur.h"
#include "Components/InvalidationBox.h"
#include "Components/RetainerBox.h"
#include "Components/WidgetSwitcher.h"
#include "Components/NamedSlot.h"
#include "Components/ListView.h"
#include "Components/GridSlot.h"
#include "Components/PanelWidget.h"
#include "EditorAssetLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Engine/Engine.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "WidgetBlueprintFactory.h"
#include "K2Node_ComponentBoundEvent.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Animation/WidgetAnimation.h"
#include "MovieScene.h"
#include "MovieSceneBinding.h"
#include "Tracks/MovieSceneFloatTrack.h"
#include "Tracks/MovieScenePropertyTrack.h"
#include "Channels/MovieSceneFloatChannel.h"
#include "Channels/MovieSceneChannelProxy.h"

// ============================================================================
// Private Helpers
// ============================================================================

TSharedPtr<FJsonObject> FWidgetEditor::SuccessResult(const FString& Message)
{
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("message"), Message);
	return Result;
}

TSharedPtr<FJsonObject> FWidgetEditor::ErrorResult(const FString& Message)
{
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), false);
	Result->SetStringField(TEXT("error"), Message);
	return Result;
}

// ============================================================================
// Widget Class Resolution
// ============================================================================

UClass* FWidgetEditor::ResolveWidgetClass(const FString& ShortName)
{
	static TMap<FString, UClass*> ClassMap;
	if (ClassMap.Num() == 0)
	{
		ClassMap.Add(TEXT("CanvasPanel"), UCanvasPanel::StaticClass());
		ClassMap.Add(TEXT("Overlay"), UOverlay::StaticClass());
		ClassMap.Add(TEXT("VerticalBox"), UVerticalBox::StaticClass());
		ClassMap.Add(TEXT("HorizontalBox"), UHorizontalBox::StaticClass());
		ClassMap.Add(TEXT("SizeBox"), USizeBox::StaticClass());
		ClassMap.Add(TEXT("Border"), UBorder::StaticClass());
		ClassMap.Add(TEXT("ProgressBar"), UProgressBar::StaticClass());
		ClassMap.Add(TEXT("TextBlock"), UTextBlock::StaticClass());
		ClassMap.Add(TEXT("Image"), UImage::StaticClass());
		ClassMap.Add(TEXT("Spacer"), USpacer::StaticClass());
		ClassMap.Add(TEXT("Button"), UButton::StaticClass());
		ClassMap.Add(TEXT("ScaleBox"), UScaleBox::StaticClass());
		ClassMap.Add(TEXT("GridPanel"), UGridPanel::StaticClass());
		ClassMap.Add(TEXT("WrapBox"), UWrapBox::StaticClass());
		ClassMap.Add(TEXT("CheckBox"), UCheckBox::StaticClass());
		ClassMap.Add(TEXT("Slider"), USlider::StaticClass());
		ClassMap.Add(TEXT("EditableText"), UEditableText::StaticClass());
		ClassMap.Add(TEXT("EditableTextBox"), UEditableTextBox::StaticClass());
		ClassMap.Add(TEXT("ComboBoxString"), UComboBoxString::StaticClass());
		ClassMap.Add(TEXT("ScrollBox"), UScrollBox::StaticClass());
		ClassMap.Add(TEXT("RichTextBlock"), URichTextBlock::StaticClass());
		ClassMap.Add(TEXT("CircularThrobber"), UCircularThrobber::StaticClass());
		ClassMap.Add(TEXT("Throbber"), UThrobber::StaticClass());
		ClassMap.Add(TEXT("BackgroundBlur"), UBackgroundBlur::StaticClass());
		ClassMap.Add(TEXT("InvalidationBox"), UInvalidationBox::StaticClass());
		ClassMap.Add(TEXT("RetainerBox"), URetainerBox::StaticClass());
		ClassMap.Add(TEXT("WidgetSwitcher"), UWidgetSwitcher::StaticClass());
		ClassMap.Add(TEXT("NamedSlot"), UNamedSlot::StaticClass());
		ClassMap.Add(TEXT("ListView"), UListView::StaticClass());
	}

	if (UClass** Found = ClassMap.Find(ShortName))
	{
		return *Found;
	}

	UClass* FoundClass = FindObject<UClass>(nullptr, *FString::Printf(TEXT("/Script/UMG.%s"), *ShortName));
	return FoundClass;
}

// ============================================================================
// Color Parsing
// ============================================================================

FLinearColor FWidgetEditor::ParseColor(const FString& ColorStr)
{
	FString Str = ColorStr.TrimStartAndEnd();

	if (Str.Equals(TEXT("red"), ESearchCase::IgnoreCase)) return FLinearColor::Red;
	if (Str.Equals(TEXT("green"), ESearchCase::IgnoreCase)) return FLinearColor::Green;
	if (Str.Equals(TEXT("blue"), ESearchCase::IgnoreCase)) return FLinearColor::Blue;
	if (Str.Equals(TEXT("white"), ESearchCase::IgnoreCase)) return FLinearColor::White;
	if (Str.Equals(TEXT("black"), ESearchCase::IgnoreCase)) return FLinearColor::Black;
	if (Str.Equals(TEXT("yellow"), ESearchCase::IgnoreCase)) return FLinearColor::Yellow;
	if (Str.Equals(TEXT("gray"), ESearchCase::IgnoreCase) || Str.Equals(TEXT("grey"), ESearchCase::IgnoreCase))
		return FLinearColor::Gray;
	if (Str.Equals(TEXT("transparent"), ESearchCase::IgnoreCase)) return FLinearColor::Transparent;

	if (Str.StartsWith(TEXT("#")))
	{
		Str = Str.Mid(1);
	}

	FColor ParsedColor = FColor::White;
	if (Str.Len() == 6)
	{
		ParsedColor = FColor::FromHex(Str + TEXT("FF"));
	}
	else if (Str.Len() == 8)
	{
		ParsedColor = FColor::FromHex(Str);
	}

	return ParsedColor.ReinterpretAsLinear();
}

// ============================================================================
// Asset Loading
// ============================================================================

UWidgetBlueprint* FWidgetEditor::LoadWidgetBlueprint(const FString& Path, FString& OutError)
{
	UObject* Loaded = StaticLoadObject(UWidgetBlueprint::StaticClass(), nullptr, *Path);
	if (!Loaded)
	{
		OutError = FString::Printf(TEXT("Failed to load Widget Blueprint: %s"), *Path);
		return nullptr;
	}

	UWidgetBlueprint* WBP = Cast<UWidgetBlueprint>(Loaded);
	if (!WBP)
	{
		OutError = FString::Printf(TEXT("Asset is not a Widget Blueprint: %s (is %s)"),
			*Path, *Loaded->GetClass()->GetName());
		return nullptr;
	}

	return WBP;
}

// ============================================================================
// Serialization Helpers
// ============================================================================

static FString GetSlotTypeName(UPanelSlot* Slot)
{
	if (!Slot) return TEXT("None");
	if (Cast<UCanvasPanelSlot>(Slot)) return TEXT("CanvasPanelSlot");
	if (Cast<UOverlaySlot>(Slot)) return TEXT("OverlaySlot");
	if (Cast<UVerticalBoxSlot>(Slot)) return TEXT("VerticalBoxSlot");
	if (Cast<UHorizontalBoxSlot>(Slot)) return TEXT("HorizontalBoxSlot");
	return Slot->GetClass()->GetName();
}

static FString AlignToString(EHorizontalAlignment Align)
{
	switch (Align)
	{
	case HAlign_Fill: return TEXT("Fill");
	case HAlign_Left: return TEXT("Left");
	case HAlign_Center: return TEXT("Center");
	case HAlign_Right: return TEXT("Right");
	default: return TEXT("Fill");
	}
}

static FString VAlignToString(EVerticalAlignment Align)
{
	switch (Align)
	{
	case VAlign_Fill: return TEXT("Fill");
	case VAlign_Top: return TEXT("Top");
	case VAlign_Center: return TEXT("Center");
	case VAlign_Bottom: return TEXT("Bottom");
	default: return TEXT("Fill");
	}
}

static EHorizontalAlignment StringToHAlign(const FString& Str)
{
	if (Str.Equals(TEXT("Left"), ESearchCase::IgnoreCase)) return HAlign_Left;
	if (Str.Equals(TEXT("Center"), ESearchCase::IgnoreCase)) return HAlign_Center;
	if (Str.Equals(TEXT("Right"), ESearchCase::IgnoreCase)) return HAlign_Right;
	return HAlign_Fill;
}

static EVerticalAlignment StringToVAlign(const FString& Str)
{
	if (Str.Equals(TEXT("Top"), ESearchCase::IgnoreCase)) return VAlign_Top;
	if (Str.Equals(TEXT("Center"), ESearchCase::IgnoreCase)) return VAlign_Center;
	if (Str.Equals(TEXT("Bottom"), ESearchCase::IgnoreCase)) return VAlign_Bottom;
	return VAlign_Fill;
}

static TSharedPtr<FJsonObject> ColorToJson(const FLinearColor& Color)
{
	TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetNumberField(TEXT("r"), Color.R);
	Obj->SetNumberField(TEXT("g"), Color.G);
	Obj->SetNumberField(TEXT("b"), Color.B);
	Obj->SetNumberField(TEXT("a"), Color.A);
	FColor SRGBColor = Color.ToFColor(true);
	Obj->SetStringField(TEXT("hex"), FString::Printf(TEXT("#%02X%02X%02X%02X"),
		SRGBColor.R, SRGBColor.G, SRGBColor.B, SRGBColor.A));
	return Obj;
}

static TSharedPtr<FJsonObject> SerializeBrush(const FSlateBrush& Brush)
{
	TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();

	UObject* Resource = Brush.GetResourceObject();
	Obj->SetStringField(TEXT("texture"), Resource ? Resource->GetPathName() : TEXT(""));

	Obj->SetObjectField(TEXT("tint"), ColorToJson(Brush.TintColor.GetSpecifiedColor()));

	TSharedPtr<FJsonObject> SizeObj = MakeShared<FJsonObject>();
	SizeObj->SetNumberField(TEXT("x"), Brush.ImageSize.X);
	SizeObj->SetNumberField(TEXT("y"), Brush.ImageSize.Y);
	Obj->SetObjectField(TEXT("image_size"), SizeObj);

	FString DrawAsStr;
	switch (Brush.DrawAs)
	{
	case ESlateBrushDrawType::NoDrawType: DrawAsStr = TEXT("NoDrawType"); break;
	case ESlateBrushDrawType::Box: DrawAsStr = TEXT("Box"); break;
	case ESlateBrushDrawType::Border: DrawAsStr = TEXT("Border"); break;
	case ESlateBrushDrawType::Image: DrawAsStr = TEXT("Image"); break;
	case ESlateBrushDrawType::RoundedBox: DrawAsStr = TEXT("RoundedBox"); break;
	default: DrawAsStr = TEXT("Image"); break;
	}
	Obj->SetStringField(TEXT("draw_as"), DrawAsStr);

	FString TilingStr;
	switch (Brush.Tiling)
	{
	case ESlateBrushTileType::NoTile: TilingStr = TEXT("NoTile"); break;
	case ESlateBrushTileType::Horizontal: TilingStr = TEXT("Horizontal"); break;
	case ESlateBrushTileType::Vertical: TilingStr = TEXT("Vertical"); break;
	case ESlateBrushTileType::Both: TilingStr = TEXT("Both"); break;
	default: TilingStr = TEXT("NoTile"); break;
	}
	Obj->SetStringField(TEXT("tiling"), TilingStr);

	TSharedPtr<FJsonObject> MarginObj = MakeShared<FJsonObject>();
	MarginObj->SetNumberField(TEXT("left"), Brush.Margin.Left);
	MarginObj->SetNumberField(TEXT("top"), Brush.Margin.Top);
	MarginObj->SetNumberField(TEXT("right"), Brush.Margin.Right);
	MarginObj->SetNumberField(TEXT("bottom"), Brush.Margin.Bottom);
	Obj->SetObjectField(TEXT("margin"), MarginObj);

	return Obj;
}

static FLinearColor ParseColorFromJson(const TSharedPtr<FJsonObject>& ColorObj)
{
	FString HexStr;
	if (ColorObj->TryGetStringField(TEXT("hex"), HexStr))
	{
		return FWidgetEditor::ParseColor(HexStr);
	}
	double R = 1.0, G = 1.0, B = 1.0, A = 1.0;
	ColorObj->TryGetNumberField(TEXT("r"), R);
	ColorObj->TryGetNumberField(TEXT("g"), G);
	ColorObj->TryGetNumberField(TEXT("b"), B);
	ColorObj->TryGetNumberField(TEXT("a"), A);
	return FLinearColor(R, G, B, A);
}

static TArray<FString> ApplyBrushProperties(FSlateBrush& Brush, const TSharedPtr<FJsonObject>& BrushJson)
{
	TArray<FString> SetProps;
	FString StrVal;

	if (BrushJson->TryGetStringField(TEXT("texture"), StrVal))
	{
		if (StrVal.IsEmpty())
		{
			Brush.SetResourceObject(nullptr);
		}
		else
		{
			UTexture2D* Tex = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *StrVal));
			if (Tex)
			{
				Brush.SetResourceObject(Tex);
			}
		}
		SetProps.Add(TEXT("texture"));
	}

	const TSharedPtr<FJsonObject>* TintObj;
	if (BrushJson->TryGetObjectField(TEXT("tint"), TintObj))
	{
		Brush.TintColor = FSlateColor(ParseColorFromJson(*TintObj));
		SetProps.Add(TEXT("tint"));
	}
	else if (BrushJson->TryGetStringField(TEXT("tint"), StrVal))
	{
		Brush.TintColor = FSlateColor(FWidgetEditor::ParseColor(StrVal));
		SetProps.Add(TEXT("tint"));
	}

	const TSharedPtr<FJsonObject>* SizeObj;
	if (BrushJson->TryGetObjectField(TEXT("image_size"), SizeObj))
	{
		double X = 0, Y = 0;
		(*SizeObj)->TryGetNumberField(TEXT("x"), X);
		(*SizeObj)->TryGetNumberField(TEXT("y"), Y);
		Brush.ImageSize = FVector2D(X, Y);
		SetProps.Add(TEXT("image_size"));
	}

	if (BrushJson->TryGetStringField(TEXT("draw_as"), StrVal))
	{
		if (StrVal == TEXT("NoDrawType")) Brush.DrawAs = ESlateBrushDrawType::NoDrawType;
		else if (StrVal == TEXT("Box")) Brush.DrawAs = ESlateBrushDrawType::Box;
		else if (StrVal == TEXT("Border")) Brush.DrawAs = ESlateBrushDrawType::Border;
		else if (StrVal == TEXT("Image")) Brush.DrawAs = ESlateBrushDrawType::Image;
		else if (StrVal == TEXT("RoundedBox")) Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		SetProps.Add(TEXT("draw_as"));
	}

	if (BrushJson->TryGetStringField(TEXT("tiling"), StrVal))
	{
		if (StrVal == TEXT("NoTile")) Brush.Tiling = ESlateBrushTileType::NoTile;
		else if (StrVal == TEXT("Horizontal")) Brush.Tiling = ESlateBrushTileType::Horizontal;
		else if (StrVal == TEXT("Vertical")) Brush.Tiling = ESlateBrushTileType::Vertical;
		else if (StrVal == TEXT("Both")) Brush.Tiling = ESlateBrushTileType::Both;
		SetProps.Add(TEXT("tiling"));
	}

	const TSharedPtr<FJsonObject>* MarginObj;
	if (BrushJson->TryGetObjectField(TEXT("margin"), MarginObj))
	{
		double L = 0, T = 0, R = 0, B = 0;
		(*MarginObj)->TryGetNumberField(TEXT("left"), L);
		(*MarginObj)->TryGetNumberField(TEXT("top"), T);
		(*MarginObj)->TryGetNumberField(TEXT("right"), R);
		(*MarginObj)->TryGetNumberField(TEXT("bottom"), B);
		Brush.Margin = FMargin(L, T, R, B);
		SetProps.Add(TEXT("margin"));
	}

	return SetProps;
}

TSharedPtr<FJsonObject> FWidgetEditor::SerializeSlotProperties(UWidget* Widget)
{
	TSharedPtr<FJsonObject> SlotJson = MakeShared<FJsonObject>();
	UPanelSlot* Slot = Widget->Slot;
	if (!Slot) return SlotJson;

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		FVector2D Pos = CanvasSlot->GetPosition();
		FVector2D Size = CanvasSlot->GetSize();
		FAnchors Anchors = CanvasSlot->GetAnchors();
		FVector2D Alignment = CanvasSlot->GetAlignment();

		TSharedPtr<FJsonObject> PosObj = MakeShared<FJsonObject>();
		PosObj->SetNumberField(TEXT("x"), Pos.X);
		PosObj->SetNumberField(TEXT("y"), Pos.Y);
		SlotJson->SetObjectField(TEXT("position"), PosObj);

		TSharedPtr<FJsonObject> SizeObj = MakeShared<FJsonObject>();
		SizeObj->SetNumberField(TEXT("x"), Size.X);
		SizeObj->SetNumberField(TEXT("y"), Size.Y);
		SlotJson->SetObjectField(TEXT("size"), SizeObj);

		TSharedPtr<FJsonObject> AnchorsObj = MakeShared<FJsonObject>();
		AnchorsObj->SetNumberField(TEXT("min_x"), Anchors.Minimum.X);
		AnchorsObj->SetNumberField(TEXT("min_y"), Anchors.Minimum.Y);
		AnchorsObj->SetNumberField(TEXT("max_x"), Anchors.Maximum.X);
		AnchorsObj->SetNumberField(TEXT("max_y"), Anchors.Maximum.Y);
		SlotJson->SetObjectField(TEXT("anchors"), AnchorsObj);

		TSharedPtr<FJsonObject> AlignObj = MakeShared<FJsonObject>();
		AlignObj->SetNumberField(TEXT("x"), Alignment.X);
		AlignObj->SetNumberField(TEXT("y"), Alignment.Y);
		SlotJson->SetObjectField(TEXT("alignment"), AlignObj);

		SlotJson->SetBoolField(TEXT("auto_size"), CanvasSlot->GetAutoSize());
		SlotJson->SetNumberField(TEXT("z_order"), CanvasSlot->GetZOrder());
	}
	else if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(Slot))
	{
		SlotJson->SetStringField(TEXT("h_align"), AlignToString(OverlaySlot->GetHorizontalAlignment()));
		SlotJson->SetStringField(TEXT("v_align"), VAlignToString(OverlaySlot->GetVerticalAlignment()));

		FMargin Pad = OverlaySlot->GetPadding();
		TSharedPtr<FJsonObject> PadObj = MakeShared<FJsonObject>();
		PadObj->SetNumberField(TEXT("left"), Pad.Left);
		PadObj->SetNumberField(TEXT("top"), Pad.Top);
		PadObj->SetNumberField(TEXT("right"), Pad.Right);
		PadObj->SetNumberField(TEXT("bottom"), Pad.Bottom);
		SlotJson->SetObjectField(TEXT("padding"), PadObj);
	}
	else if (UVerticalBoxSlot* VSlot = Cast<UVerticalBoxSlot>(Slot))
	{
		SlotJson->SetStringField(TEXT("h_align"), AlignToString(VSlot->GetHorizontalAlignment()));
		SlotJson->SetStringField(TEXT("v_align"), VAlignToString(VSlot->GetVerticalAlignment()));

		FSlateChildSize SizeRule = VSlot->GetSize();
		SlotJson->SetStringField(TEXT("size_rule"), SizeRule.SizeRule == ESlateSizeRule::Automatic ? TEXT("Auto") : TEXT("Fill"));
		SlotJson->SetNumberField(TEXT("fill_weight"), SizeRule.Value);

		FMargin Pad = VSlot->GetPadding();
		TSharedPtr<FJsonObject> PadObj = MakeShared<FJsonObject>();
		PadObj->SetNumberField(TEXT("left"), Pad.Left);
		PadObj->SetNumberField(TEXT("top"), Pad.Top);
		PadObj->SetNumberField(TEXT("right"), Pad.Right);
		PadObj->SetNumberField(TEXT("bottom"), Pad.Bottom);
		SlotJson->SetObjectField(TEXT("padding"), PadObj);
	}
	else if (UHorizontalBoxSlot* HSlot = Cast<UHorizontalBoxSlot>(Slot))
	{
		SlotJson->SetStringField(TEXT("h_align"), AlignToString(HSlot->GetHorizontalAlignment()));
		SlotJson->SetStringField(TEXT("v_align"), VAlignToString(HSlot->GetVerticalAlignment()));

		FSlateChildSize SizeRule = HSlot->GetSize();
		SlotJson->SetStringField(TEXT("size_rule"), SizeRule.SizeRule == ESlateSizeRule::Automatic ? TEXT("Auto") : TEXT("Fill"));
		SlotJson->SetNumberField(TEXT("fill_weight"), SizeRule.Value);

		FMargin Pad = HSlot->GetPadding();
		TSharedPtr<FJsonObject> PadObj = MakeShared<FJsonObject>();
		PadObj->SetNumberField(TEXT("left"), Pad.Left);
		PadObj->SetNumberField(TEXT("top"), Pad.Top);
		PadObj->SetNumberField(TEXT("right"), Pad.Right);
		PadObj->SetNumberField(TEXT("bottom"), Pad.Bottom);
		SlotJson->SetObjectField(TEXT("padding"), PadObj);
	}
	else if (UScrollBoxSlot* SBSlot = Cast<UScrollBoxSlot>(Slot))
	{
		SlotJson->SetStringField(TEXT("h_align"), AlignToString(SBSlot->GetHorizontalAlignment()));
		SlotJson->SetStringField(TEXT("v_align"), VAlignToString(SBSlot->GetVerticalAlignment()));

		FMargin Pad = SBSlot->GetPadding();
		TSharedPtr<FJsonObject> PadObj = MakeShared<FJsonObject>();
		PadObj->SetNumberField(TEXT("left"), Pad.Left);
		PadObj->SetNumberField(TEXT("top"), Pad.Top);
		PadObj->SetNumberField(TEXT("right"), Pad.Right);
		PadObj->SetNumberField(TEXT("bottom"), Pad.Bottom);
		SlotJson->SetObjectField(TEXT("padding"), PadObj);
	}
	else if (UGridSlot* GSlot = Cast<UGridSlot>(Slot))
	{
		SlotJson->SetNumberField(TEXT("row"), GSlot->GetRow());
		SlotJson->SetNumberField(TEXT("column"), GSlot->GetColumn());
		SlotJson->SetNumberField(TEXT("row_span"), GSlot->GetRowSpan());
		SlotJson->SetNumberField(TEXT("column_span"), GSlot->GetColumnSpan());
		SlotJson->SetStringField(TEXT("h_align"), AlignToString(GSlot->GetHorizontalAlignment()));
		SlotJson->SetStringField(TEXT("v_align"), VAlignToString(GSlot->GetVerticalAlignment()));

		FMargin Pad = GSlot->GetPadding();
		TSharedPtr<FJsonObject> PadObj = MakeShared<FJsonObject>();
		PadObj->SetNumberField(TEXT("left"), Pad.Left);
		PadObj->SetNumberField(TEXT("top"), Pad.Top);
		PadObj->SetNumberField(TEXT("right"), Pad.Right);
		PadObj->SetNumberField(TEXT("bottom"), Pad.Bottom);
		SlotJson->SetObjectField(TEXT("padding"), PadObj);
	}

	return SlotJson;
}

TSharedPtr<FJsonObject> FWidgetEditor::SerializeWidgetProperties(UWidget* Widget)
{
	TSharedPtr<FJsonObject> Props = MakeShared<FJsonObject>();
	if (!Widget) return Props;

	Props->SetStringField(TEXT("visibility"), StaticEnum<ESlateVisibility>()->GetNameStringByValue((int64)Widget->GetVisibility()));
	Props->SetNumberField(TEXT("render_opacity"), Widget->GetRenderOpacity());
	Props->SetBoolField(TEXT("is_enabled"), Widget->GetIsEnabled());

	if (UProgressBar* PB = Cast<UProgressBar>(Widget))
	{
		Props->SetNumberField(TEXT("percent"), PB->GetPercent());
		Props->SetObjectField(TEXT("fill_color"), ColorToJson(PB->GetFillColorAndOpacity()));
		Props->SetStringField(TEXT("bar_fill_type"), StaticEnum<EProgressBarFillType::Type>()->GetNameStringByValue((int64)PB->GetBarFillType()));
		const FProgressBarStyle& Style = PB->GetWidgetStyle();
		Props->SetObjectField(TEXT("style_fill_tint"), ColorToJson(Style.FillImage.TintColor.GetSpecifiedColor()));
		Props->SetObjectField(TEXT("style_background_tint"), ColorToJson(Style.BackgroundImage.TintColor.GetSpecifiedColor()));
		TSharedPtr<FJsonObject> BrushesObj = MakeShared<FJsonObject>();
		BrushesObj->SetObjectField(TEXT("style_fill"), SerializeBrush(Style.FillImage));
		BrushesObj->SetObjectField(TEXT("style_background"), SerializeBrush(Style.BackgroundImage));
		Props->SetObjectField(TEXT("brushes"), BrushesObj);
	}
	else if (UTextBlock* TB = Cast<UTextBlock>(Widget))
	{
		Props->SetStringField(TEXT("text"), TB->GetText().ToString());
		Props->SetObjectField(TEXT("color"), ColorToJson(TB->GetColorAndOpacity().GetSpecifiedColor()));
		FSlateFontInfo FontInfo = TB->GetFont();
		Props->SetNumberField(TEXT("font_size"), FontInfo.Size);
	}
	else if (UImage* Img = Cast<UImage>(Widget))
	{
		Props->SetObjectField(TEXT("color_and_opacity"), ColorToJson(Img->GetColorAndOpacity()));
		Props->SetObjectField(TEXT("brush"), SerializeBrush(Img->GetBrush()));
	}
	else if (UButton* Btn = Cast<UButton>(Widget))
	{
		const FButtonStyle& BtnStyle = Btn->GetStyle();
		TSharedPtr<FJsonObject> BrushesObj = MakeShared<FJsonObject>();
		BrushesObj->SetObjectField(TEXT("style_normal"), SerializeBrush(BtnStyle.Normal));
		BrushesObj->SetObjectField(TEXT("style_hovered"), SerializeBrush(BtnStyle.Hovered));
		BrushesObj->SetObjectField(TEXT("style_pressed"), SerializeBrush(BtnStyle.Pressed));
		BrushesObj->SetObjectField(TEXT("style_disabled"), SerializeBrush(BtnStyle.Disabled));
		Props->SetObjectField(TEXT("brushes"), BrushesObj);
	}
	else if (UBorder* BorderWidget = Cast<UBorder>(Widget))
	{
		Props->SetObjectField(TEXT("brush"), SerializeBrush(BorderWidget->Background));
	}
	else if (USizeBox* SB = Cast<USizeBox>(Widget))
	{
		Props->SetNumberField(TEXT("width_override"), SB->GetWidthOverride());
		Props->SetNumberField(TEXT("height_override"), SB->GetHeightOverride());
		Props->SetNumberField(TEXT("min_desired_width"), SB->GetMinDesiredWidth());
		Props->SetNumberField(TEXT("min_desired_height"), SB->GetMinDesiredHeight());
		Props->SetNumberField(TEXT("max_desired_width"), SB->GetMaxDesiredWidth());
		Props->SetNumberField(TEXT("max_desired_height"), SB->GetMaxDesiredHeight());
	}
	else if (UCheckBox* CB = Cast<UCheckBox>(Widget))
	{
		ECheckBoxState State = CB->GetCheckedState();
		Props->SetStringField(TEXT("is_checked"),
			State == ECheckBoxState::Checked ? TEXT("Checked") :
			State == ECheckBoxState::Undetermined ? TEXT("Undetermined") : TEXT("Unchecked"));
	}
	else if (USlider* SL = Cast<USlider>(Widget))
	{
		Props->SetNumberField(TEXT("value"), SL->GetValue());
		Props->SetNumberField(TEXT("min_value"), SL->GetMinValue());
		Props->SetNumberField(TEXT("max_value"), SL->GetMaxValue());
		Props->SetNumberField(TEXT("step_size"), SL->GetStepSize());
	}
	else if (UEditableTextBox* ETB = Cast<UEditableTextBox>(Widget))
	{
		Props->SetStringField(TEXT("text"), ETB->GetText().ToString());
		Props->SetStringField(TEXT("hint_text"), ETB->GetHintText().ToString());
		Props->SetBoolField(TEXT("is_read_only"), ETB->GetIsReadOnly());
	}
	else if (UEditableText* ET = Cast<UEditableText>(Widget))
	{
		Props->SetStringField(TEXT("text"), ET->GetText().ToString());
		Props->SetStringField(TEXT("hint_text"), ET->GetHintText().ToString());
		Props->SetBoolField(TEXT("is_read_only"), ET->GetIsReadOnly());
	}
	else if (UComboBoxString* CBS = Cast<UComboBoxString>(Widget))
	{
		Props->SetStringField(TEXT("selected_option"), CBS->GetSelectedOption());
	}
	else if (UScrollBox* ScrollB = Cast<UScrollBox>(Widget))
	{
		Props->SetStringField(TEXT("orientation"),
			ScrollB->GetOrientation() == Orient_Horizontal ? TEXT("Horizontal") : TEXT("Vertical"));
		Props->SetStringField(TEXT("scroll_bar_visibility"),
			StaticEnum<ESlateVisibility>()->GetNameStringByValue((int64)ScrollB->GetScrollBarVisibility()));
	}
	else if (URichTextBlock* RTB = Cast<URichTextBlock>(Widget))
	{
		Props->SetStringField(TEXT("text"), RTB->GetText().ToString());
	}
	else if (UCircularThrobber* CT = Cast<UCircularThrobber>(Widget))
	{
		Props->SetNumberField(TEXT("number_of_pieces"), CT->GetNumberOfPieces());
		Props->SetNumberField(TEXT("period"), CT->GetPeriod());
	}
	else if (UThrobber* TH = Cast<UThrobber>(Widget))
	{
		Props->SetNumberField(TEXT("number_of_pieces"), TH->GetNumberOfPieces());
	}
	else if (UBackgroundBlur* BB = Cast<UBackgroundBlur>(Widget))
	{
		Props->SetNumberField(TEXT("blur_strength"), BB->GetBlurStrength());
	}
	else if (UWidgetSwitcher* WS = Cast<UWidgetSwitcher>(Widget))
	{
		Props->SetNumberField(TEXT("active_widget_index"), WS->GetActiveWidgetIndex());
	}

	return Props;
}

TSharedPtr<FJsonObject> FWidgetEditor::SerializeWidgetTree(UWidget* Widget, UWidgetBlueprint* WBP)
{
	TSharedPtr<FJsonObject> Node = MakeShared<FJsonObject>();
	if (!Widget) return Node;

	Node->SetStringField(TEXT("name"), Widget->GetName());
	Node->SetStringField(TEXT("class"), Widget->GetClass()->GetName());

	if (Widget->Slot)
	{
		Node->SetStringField(TEXT("slot_type"), GetSlotTypeName(Widget->Slot));
		Node->SetObjectField(TEXT("slot_properties"), SerializeSlotProperties(Widget));
	}

	Node->SetObjectField(TEXT("visual_properties"), SerializeWidgetProperties(Widget));

	if (WBP)
	{
		FString WidgetName = Widget->GetName();
		TArray<TSharedPtr<FJsonValue>> WidgetBindings;
		for (const FDelegateEditorBinding& Binding : WBP->Bindings)
		{
			if (Binding.ObjectName == WidgetName)
			{
				TSharedPtr<FJsonObject> BindObj = MakeShared<FJsonObject>();
				BindObj->SetStringField(TEXT("property"), Binding.PropertyName.ToString());
				BindObj->SetStringField(TEXT("function"), Binding.FunctionName.ToString());
				BindObj->SetStringField(TEXT("kind"), Binding.Kind == EBindingKind::Function ? TEXT("Function") : TEXT("Property"));
				WidgetBindings.Add(MakeShared<FJsonValueObject>(BindObj));
			}
		}
		if (WidgetBindings.Num() > 0)
		{
			Node->SetArrayField(TEXT("bindings"), WidgetBindings);
		}
	}

	UPanelWidget* Panel = Cast<UPanelWidget>(Widget);
	if (Panel)
	{
		TArray<TSharedPtr<FJsonValue>> ChildrenArray;
		for (int32 i = 0; i < Panel->GetChildrenCount(); ++i)
		{
			UWidget* Child = Panel->GetChildAt(i);
			if (Child)
			{
				ChildrenArray.Add(MakeShared<FJsonValueObject>(SerializeWidgetTree(Child, WBP)));
			}
		}
		Node->SetArrayField(TEXT("children"), ChildrenArray);
	}

	return Node;
}

// ============================================================================
// InspectWidgetTree
// ============================================================================

TSharedPtr<FJsonObject> FWidgetEditor::InspectWidgetTree(const FString& AssetPath)
{
	FString LoadError;
	UWidgetBlueprint* WBP = LoadWidgetBlueprint(AssetPath, LoadError);
	if (!WBP)
	{
		return ErrorResult(LoadError);
	}

	UWidgetTree* WidgetTree = WBP->WidgetTree;
	if (!WidgetTree)
	{
		return ErrorResult(TEXT("Widget Blueprint has no WidgetTree"));
	}

	TSharedPtr<FJsonObject> Result = SuccessResult(TEXT("Widget tree inspected"));
	Result->SetStringField(TEXT("asset_path"), AssetPath);
	Result->SetStringField(TEXT("blueprint_name"), WBP->GetName());

	UWidget* RootWidget = WidgetTree->RootWidget;
	if (RootWidget)
	{
		Result->SetObjectField(TEXT("widget_tree"), SerializeWidgetTree(RootWidget, WBP));
	}
	else
	{
		Result->SetStringField(TEXT("widget_tree"), TEXT("empty"));
	}

	TArray<UWidget*> AllWidgets;
	WidgetTree->GetAllWidgets(AllWidgets);
	Result->SetNumberField(TEXT("total_widgets"), AllWidgets.Num());

	return Result;
}

// ============================================================================
// GetWidgetProperties
// ============================================================================

TSharedPtr<FJsonObject> FWidgetEditor::GetWidgetProperties(const FString& AssetPath, const FString& WidgetName)
{
	FString LoadError;
	UWidgetBlueprint* WBP = LoadWidgetBlueprint(AssetPath, LoadError);
	if (!WBP)
	{
		return ErrorResult(LoadError);
	}

	UWidget* Widget = WBP->WidgetTree->FindWidget(FName(*WidgetName));
	if (!Widget)
	{
		return ErrorResult(FString::Printf(TEXT("Widget not found: %s"), *WidgetName));
	}

	TSharedPtr<FJsonObject> Result = SuccessResult(
		FString::Printf(TEXT("Properties for %s (%s)"), *WidgetName, *Widget->GetClass()->GetName()));
	Result->SetStringField(TEXT("widget_name"), WidgetName);
	Result->SetStringField(TEXT("widget_class"), Widget->GetClass()->GetName());
	Result->SetObjectField(TEXT("visual_properties"), SerializeWidgetProperties(Widget));

	if (Widget->Slot)
	{
		Result->SetStringField(TEXT("slot_type"), GetSlotTypeName(Widget->Slot));
		Result->SetObjectField(TEXT("slot_properties"), SerializeSlotProperties(Widget));
	}

	return Result;
}

// ============================================================================
// CreateWidgetBlueprint
// ============================================================================

TSharedPtr<FJsonObject> FWidgetEditor::CreateWidgetBlueprint(const FString& Name, const FString& PackagePath,
	const FString& ParentClass, const FString& RootWidgetClass)
{
	UClass* ParentUClass = UUserWidget::StaticClass();
	if (!ParentClass.IsEmpty() && !ParentClass.Equals(TEXT("UserWidget"), ESearchCase::IgnoreCase))
	{
		ParentUClass = FindObject<UClass>(nullptr, *FString::Printf(TEXT("/Script/UMG.%s"), *ParentClass));
		if (!ParentUClass)
		{
			ParentUClass = LoadClass<UUserWidget>(nullptr, *ParentClass);
		}
		if (!ParentUClass)
		{
			return ErrorResult(FString::Printf(TEXT("Could not find parent class: %s"), *ParentClass));
		}
	}

	UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
	Factory->ParentClass = ParentUClass;

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UObject* NewAsset = AssetTools.CreateAsset(Name, PackagePath, UWidgetBlueprint::StaticClass(), Factory);
	if (!NewAsset)
	{
		return ErrorResult(FString::Printf(TEXT("Failed to create Widget Blueprint: %s/%s"), *PackagePath, *Name));
	}

	UWidgetBlueprint* WBP = Cast<UWidgetBlueprint>(NewAsset);
	if (!WBP)
	{
		return ErrorResult(TEXT("Created asset is not a Widget Blueprint"));
	}

	if (!RootWidgetClass.IsEmpty() && !RootWidgetClass.Equals(TEXT("None"), ESearchCase::IgnoreCase))
	{
		UClass* RootClass = ResolveWidgetClass(RootWidgetClass);
		if (!RootClass)
		{
			return ErrorResult(FString::Printf(TEXT("Unknown root widget class: %s"), *RootWidgetClass));
		}

		UWidget* Root = WBP->WidgetTree->ConstructWidget<UWidget>(RootClass, FName(TEXT("RootPanel")));
		if (Root)
		{
			WBP->WidgetTree->RootWidget = Root;
		}
	}

	WBP->MarkPackageDirty();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WBP);

	TSharedPtr<FJsonObject> Result = SuccessResult(
		FString::Printf(TEXT("Created Widget Blueprint: %s"), *WBP->GetPathName()));
	Result->SetStringField(TEXT("path"), WBP->GetPathName());
	return Result;
}

// ============================================================================
// AddWidget
// ============================================================================

TSharedPtr<FJsonObject> FWidgetEditor::AddWidget(UWidgetBlueprint* WBP, const FString& WidgetClass,
	const FString& WidgetName, const FString& ParentName)
{
	if (!WBP || !WBP->WidgetTree)
	{
		return ErrorResult(TEXT("Invalid Widget Blueprint or missing WidgetTree"));
	}

	UClass* WidgetUClass = ResolveWidgetClass(WidgetClass);
	if (!WidgetUClass)
	{
		return ErrorResult(FString::Printf(TEXT("Unknown widget class: %s"), *WidgetClass));
	}

	if (WBP->WidgetTree->FindWidget(FName(*WidgetName)))
	{
		return ErrorResult(FString::Printf(TEXT("Widget with name '%s' already exists"), *WidgetName));
	}

	UWidget* NewWidget = WBP->WidgetTree->ConstructWidget<UWidget>(WidgetUClass, FName(*WidgetName));
	if (!NewWidget)
	{
		return ErrorResult(FString::Printf(TEXT("Failed to construct widget of class %s"), *WidgetClass));
	}

	UPanelWidget* Parent = nullptr;
	if (ParentName.IsEmpty() || ParentName.Equals(TEXT("root"), ESearchCase::IgnoreCase))
	{
		Parent = Cast<UPanelWidget>(WBP->WidgetTree->RootWidget);
	}
	else
	{
		UWidget* ParentWidget = WBP->WidgetTree->FindWidget(FName(*ParentName));
		if (!ParentWidget)
		{
			WBP->WidgetTree->RemoveWidget(NewWidget);
			return ErrorResult(FString::Printf(TEXT("Parent widget not found: %s"), *ParentName));
		}
		Parent = Cast<UPanelWidget>(ParentWidget);
		if (!Parent)
		{
			WBP->WidgetTree->RemoveWidget(NewWidget);
			return ErrorResult(FString::Printf(TEXT("Parent '%s' is not a panel widget (is %s)"),
				*ParentName, *ParentWidget->GetClass()->GetName()));
		}
	}

	if (!Parent)
	{
		if (!WBP->WidgetTree->RootWidget)
		{
			WBP->WidgetTree->RootWidget = NewWidget;

			WBP->MarkPackageDirty();
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WBP);

			TSharedPtr<FJsonObject> Result = SuccessResult(
				FString::Printf(TEXT("Set %s as root widget"), *WidgetName));
			Result->SetStringField(TEXT("widget_name"), WidgetName);
			Result->SetStringField(TEXT("widget_class"), WidgetClass);
			Result->SetStringField(TEXT("slot_type"), TEXT("None (root)"));
			return Result;
		}
		else
		{
			WBP->WidgetTree->RemoveWidget(NewWidget);
			return ErrorResult(TEXT("Root widget is not a panel — cannot add children"));
		}
	}

	UPanelSlot* Slot = Parent->AddChild(NewWidget);
	if (!Slot)
	{
		WBP->WidgetTree->RemoveWidget(NewWidget);
		return ErrorResult(TEXT("AddChild returned null slot"));
	}

	WBP->MarkPackageDirty();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WBP);

	TSharedPtr<FJsonObject> Result = SuccessResult(
		FString::Printf(TEXT("Added %s '%s' to %s"), *WidgetClass, *WidgetName, *Parent->GetName()));
	Result->SetStringField(TEXT("widget_name"), WidgetName);
	Result->SetStringField(TEXT("widget_class"), WidgetClass);
	Result->SetStringField(TEXT("parent_name"), Parent->GetName());
	Result->SetStringField(TEXT("slot_type"), GetSlotTypeName(Slot));
	return Result;
}

// ============================================================================
// RemoveWidget
// ============================================================================

TSharedPtr<FJsonObject> FWidgetEditor::RemoveWidget(UWidgetBlueprint* WBP, const FString& WidgetName)
{
	if (!WBP || !WBP->WidgetTree)
	{
		return ErrorResult(TEXT("Invalid Widget Blueprint or missing WidgetTree"));
	}

	UWidget* Widget = WBP->WidgetTree->FindWidget(FName(*WidgetName));
	if (!Widget)
	{
		return ErrorResult(FString::Printf(TEXT("Widget not found: %s"), *WidgetName));
	}

	FString WidgetClass = Widget->GetClass()->GetName();
	bool bRemoved = WBP->WidgetTree->RemoveWidget(Widget);
	if (!bRemoved)
	{
		return ErrorResult(FString::Printf(TEXT("Failed to remove widget: %s"), *WidgetName));
	}

	WBP->MarkPackageDirty();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WBP);

	TSharedPtr<FJsonObject> Result = SuccessResult(
		FString::Printf(TEXT("Removed widget %s (%s)"), *WidgetName, *WidgetClass));
	return Result;
}

// ============================================================================
// SetWidgetProperty
// ============================================================================

TSharedPtr<FJsonObject> FWidgetEditor::SetWidgetProperty(UWidgetBlueprint* WBP, const FString& WidgetName,
	const TSharedPtr<FJsonObject>& Properties)
{
	if (!WBP || !WBP->WidgetTree)
	{
		return ErrorResult(TEXT("Invalid Widget Blueprint or missing WidgetTree"));
	}

	UWidget* Widget = WBP->WidgetTree->FindWidget(FName(*WidgetName));
	if (!Widget)
	{
		return ErrorResult(FString::Printf(TEXT("Widget not found: %s"), *WidgetName));
	}

	TArray<FString> SetProperties;
	FString StrVal;
	double NumVal;
	bool BoolVal;

	if (Properties->TryGetNumberField(TEXT("render_opacity"), NumVal))
	{
		Widget->SetRenderOpacity(static_cast<float>(NumVal));
		SetProperties.Add(TEXT("render_opacity"));
	}

	if (Properties->TryGetBoolField(TEXT("is_enabled"), BoolVal))
	{
		Widget->SetIsEnabled(BoolVal);
		SetProperties.Add(TEXT("is_enabled"));
	}

	if (Properties->TryGetStringField(TEXT("visibility"), StrVal))
	{
		int64 EnumVal = StaticEnum<ESlateVisibility>()->GetValueByNameString(StrVal);
		if (EnumVal != INDEX_NONE)
		{
			Widget->SetVisibility((ESlateVisibility)EnumVal);
			SetProperties.Add(TEXT("visibility"));
		}
	}

	if (UProgressBar* PB = Cast<UProgressBar>(Widget))
	{
		if (Properties->TryGetNumberField(TEXT("percent"), NumVal))
		{
			PB->SetPercent(static_cast<float>(NumVal));
			SetProperties.Add(TEXT("percent"));
		}
		if (Properties->TryGetStringField(TEXT("fill_color"), StrVal))
		{
			PB->SetFillColorAndOpacity(ParseColor(StrVal));
			SetProperties.Add(TEXT("fill_color"));
		}
		if (Properties->TryGetStringField(TEXT("bar_fill_type"), StrVal))
		{
			int64 EnumVal = StaticEnum<EProgressBarFillType::Type>()->GetValueByNameString(StrVal);
			if (EnumVal != INDEX_NONE)
			{
				PB->SetBarFillType((EProgressBarFillType::Type)EnumVal);
				SetProperties.Add(TEXT("bar_fill_type"));
			}
		}
		if (Properties->TryGetBoolField(TEXT("is_marquee"), BoolVal))
		{
			PB->SetIsMarquee(BoolVal);
			SetProperties.Add(TEXT("is_marquee"));
		}
		if (Properties->TryGetStringField(TEXT("style_fill_tint"), StrVal))
		{
			FProgressBarStyle Style = PB->GetWidgetStyle();
			Style.FillImage.TintColor = FSlateColor(ParseColor(StrVal));
			PB->SetWidgetStyle(Style);
			SetProperties.Add(TEXT("style_fill_tint"));
		}
		if (Properties->TryGetStringField(TEXT("style_background_tint"), StrVal))
		{
			FProgressBarStyle Style = PB->GetWidgetStyle();
			Style.BackgroundImage.TintColor = FSlateColor(ParseColor(StrVal));
			PB->SetWidgetStyle(Style);
			SetProperties.Add(TEXT("style_background_tint"));
		}
	}
	else if (UTextBlock* TB = Cast<UTextBlock>(Widget))
	{
		if (Properties->TryGetStringField(TEXT("text"), StrVal))
		{
			TB->SetText(FText::FromString(StrVal));
			SetProperties.Add(TEXT("text"));
		}
		if (Properties->TryGetStringField(TEXT("color"), StrVal))
		{
			FLinearColor Color = ParseColor(StrVal);
			TB->SetColorAndOpacity(FSlateColor(Color));
			SetProperties.Add(TEXT("color"));
		}
		if (Properties->TryGetNumberField(TEXT("font_size"), NumVal))
		{
			FSlateFontInfo FontInfo = TB->GetFont();
			FontInfo.Size = static_cast<int32>(NumVal);
			TB->SetFont(FontInfo);
			SetProperties.Add(TEXT("font_size"));
		}
		if (Properties->TryGetStringField(TEXT("justification"), StrVal))
		{
			if (StrVal.Equals(TEXT("Left"), ESearchCase::IgnoreCase))
				TB->SetJustification(ETextJustify::Left);
			else if (StrVal.Equals(TEXT("Center"), ESearchCase::IgnoreCase))
				TB->SetJustification(ETextJustify::Center);
			else if (StrVal.Equals(TEXT("Right"), ESearchCase::IgnoreCase))
				TB->SetJustification(ETextJustify::Right);
			SetProperties.Add(TEXT("justification"));
		}
		if (Properties->TryGetBoolField(TEXT("auto_wrap"), BoolVal))
		{
			TB->SetAutoWrapText(BoolVal);
			SetProperties.Add(TEXT("auto_wrap"));
		}
	}
	else if (UImage* Img = Cast<UImage>(Widget))
	{
		if (Properties->TryGetStringField(TEXT("color_and_opacity"), StrVal))
		{
			Img->SetColorAndOpacity(ParseColor(StrVal));
			SetProperties.Add(TEXT("color_and_opacity"));
		}
		if (Properties->TryGetStringField(TEXT("brush_texture"), StrVal))
		{
			UTexture2D* Tex = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *StrVal));
			if (Tex)
			{
				Img->SetBrushFromTexture(Tex);
				SetProperties.Add(TEXT("brush_texture"));
			}
		}
	}
	else if (USizeBox* SB = Cast<USizeBox>(Widget))
	{
		if (Properties->TryGetNumberField(TEXT("width_override"), NumVal))
		{
			SB->SetWidthOverride(static_cast<float>(NumVal));
			SetProperties.Add(TEXT("width_override"));
		}
		if (Properties->TryGetNumberField(TEXT("height_override"), NumVal))
		{
			SB->SetHeightOverride(static_cast<float>(NumVal));
			SetProperties.Add(TEXT("height_override"));
		}
		if (Properties->TryGetNumberField(TEXT("min_desired_width"), NumVal))
		{
			SB->SetMinDesiredWidth(static_cast<float>(NumVal));
			SetProperties.Add(TEXT("min_desired_width"));
		}
		if (Properties->TryGetNumberField(TEXT("min_desired_height"), NumVal))
		{
			SB->SetMinDesiredHeight(static_cast<float>(NumVal));
			SetProperties.Add(TEXT("min_desired_height"));
		}
		if (Properties->TryGetNumberField(TEXT("max_desired_width"), NumVal))
		{
			SB->SetMaxDesiredWidth(static_cast<float>(NumVal));
			SetProperties.Add(TEXT("max_desired_width"));
		}
		if (Properties->TryGetNumberField(TEXT("max_desired_height"), NumVal))
		{
			SB->SetMaxDesiredHeight(static_cast<float>(NumVal));
			SetProperties.Add(TEXT("max_desired_height"));
		}
	}
	else if (UCheckBox* CB = Cast<UCheckBox>(Widget))
	{
		if (Properties->TryGetStringField(TEXT("is_checked"), StrVal))
		{
			if (StrVal.Equals(TEXT("Checked"), ESearchCase::IgnoreCase) || StrVal.Equals(TEXT("true"), ESearchCase::IgnoreCase))
				CB->SetCheckedState(ECheckBoxState::Checked);
			else if (StrVal.Equals(TEXT("Undetermined"), ESearchCase::IgnoreCase))
				CB->SetCheckedState(ECheckBoxState::Undetermined);
			else
				CB->SetCheckedState(ECheckBoxState::Unchecked);
			SetProperties.Add(TEXT("is_checked"));
		}
		if (Properties->TryGetBoolField(TEXT("is_checked"), BoolVal))
		{
			CB->SetIsChecked(BoolVal);
			SetProperties.Add(TEXT("is_checked"));
		}
	}
	else if (USlider* SL = Cast<USlider>(Widget))
	{
		if (Properties->TryGetNumberField(TEXT("value"), NumVal))
		{
			SL->SetValue(static_cast<float>(NumVal));
			SetProperties.Add(TEXT("value"));
		}
		if (Properties->TryGetNumberField(TEXT("min_value"), NumVal))
		{
			SL->SetMinValue(static_cast<float>(NumVal));
			SetProperties.Add(TEXT("min_value"));
		}
		if (Properties->TryGetNumberField(TEXT("max_value"), NumVal))
		{
			SL->SetMaxValue(static_cast<float>(NumVal));
			SetProperties.Add(TEXT("max_value"));
		}
		if (Properties->TryGetNumberField(TEXT("step_size"), NumVal))
		{
			SL->SetStepSize(static_cast<float>(NumVal));
			SetProperties.Add(TEXT("step_size"));
		}
	}
	else if (UEditableTextBox* ETB = Cast<UEditableTextBox>(Widget))
	{
		if (Properties->TryGetStringField(TEXT("text"), StrVal))
		{
			ETB->SetText(FText::FromString(StrVal));
			SetProperties.Add(TEXT("text"));
		}
		if (Properties->TryGetStringField(TEXT("hint_text"), StrVal))
		{
			ETB->SetHintText(FText::FromString(StrVal));
			SetProperties.Add(TEXT("hint_text"));
		}
		if (Properties->TryGetBoolField(TEXT("is_read_only"), BoolVal))
		{
			ETB->SetIsReadOnly(BoolVal);
			SetProperties.Add(TEXT("is_read_only"));
		}
	}
	else if (UEditableText* ET = Cast<UEditableText>(Widget))
	{
		if (Properties->TryGetStringField(TEXT("text"), StrVal))
		{
			ET->SetText(FText::FromString(StrVal));
			SetProperties.Add(TEXT("text"));
		}
		if (Properties->TryGetStringField(TEXT("hint_text"), StrVal))
		{
			ET->SetHintText(FText::FromString(StrVal));
			SetProperties.Add(TEXT("hint_text"));
		}
		if (Properties->TryGetBoolField(TEXT("is_read_only"), BoolVal))
		{
			ET->SetIsReadOnly(BoolVal);
			SetProperties.Add(TEXT("is_read_only"));
		}
	}
	else if (UComboBoxString* CBS = Cast<UComboBoxString>(Widget))
	{
		if (Properties->TryGetStringField(TEXT("selected_option"), StrVal))
		{
			CBS->SetSelectedOption(StrVal);
			SetProperties.Add(TEXT("selected_option"));
		}
	}
	else if (UScrollBox* ScrollB = Cast<UScrollBox>(Widget))
	{
		if (Properties->TryGetStringField(TEXT("orientation"), StrVal))
		{
			ScrollB->SetOrientation(StrVal.Equals(TEXT("Horizontal"), ESearchCase::IgnoreCase) ? Orient_Horizontal : Orient_Vertical);
			SetProperties.Add(TEXT("orientation"));
		}
		if (Properties->TryGetStringField(TEXT("scroll_bar_visibility"), StrVal))
		{
			int64 EnumVal = StaticEnum<ESlateVisibility>()->GetValueByNameString(StrVal);
			if (EnumVal != INDEX_NONE)
			{
				ScrollB->SetScrollBarVisibility((ESlateVisibility)EnumVal);
				SetProperties.Add(TEXT("scroll_bar_visibility"));
			}
		}
	}
	else if (URichTextBlock* RTB = Cast<URichTextBlock>(Widget))
	{
		if (Properties->TryGetStringField(TEXT("text"), StrVal))
		{
			RTB->SetText(FText::FromString(StrVal));
			SetProperties.Add(TEXT("text"));
		}
	}
	else if (UCircularThrobber* CT = Cast<UCircularThrobber>(Widget))
	{
		if (Properties->TryGetNumberField(TEXT("number_of_pieces"), NumVal))
		{
			CT->SetNumberOfPieces(static_cast<int32>(NumVal));
			SetProperties.Add(TEXT("number_of_pieces"));
		}
		if (Properties->TryGetNumberField(TEXT("period"), NumVal))
		{
			CT->SetPeriod(static_cast<float>(NumVal));
			SetProperties.Add(TEXT("period"));
		}
	}
	else if (UThrobber* TH = Cast<UThrobber>(Widget))
	{
		if (Properties->TryGetNumberField(TEXT("number_of_pieces"), NumVal))
		{
			TH->SetNumberOfPieces(static_cast<int32>(NumVal));
			SetProperties.Add(TEXT("number_of_pieces"));
		}
	}
	else if (UBackgroundBlur* BB = Cast<UBackgroundBlur>(Widget))
	{
		if (Properties->TryGetNumberField(TEXT("blur_strength"), NumVal))
		{
			BB->SetBlurStrength(static_cast<float>(NumVal));
			SetProperties.Add(TEXT("blur_strength"));
		}
	}
	else if (UWidgetSwitcher* WS = Cast<UWidgetSwitcher>(Widget))
	{
		if (Properties->TryGetNumberField(TEXT("active_widget_index"), NumVal))
		{
			WS->SetActiveWidgetIndex(static_cast<int32>(NumVal));
			SetProperties.Add(TEXT("active_widget_index"));
		}
	}

	if (SetProperties.Num() == 0)
	{
		return ErrorResult(TEXT("No recognized properties were set. Check property names and widget type."));
	}

	WBP->MarkPackageDirty();

	TSharedPtr<FJsonObject> Result = SuccessResult(
		FString::Printf(TEXT("Set %d properties on %s: %s"),
			SetProperties.Num(), *WidgetName, *FString::Join(SetProperties, TEXT(", "))));
	return Result;
}

// ============================================================================
// SetBrush
// ============================================================================

TSharedPtr<FJsonObject> FWidgetEditor::SetBrush(UWidgetBlueprint* WBP, const FString& WidgetName,
	const FString& BrushProperty, const TSharedPtr<FJsonObject>& BrushJson)
{
	if (!WBP || !WBP->WidgetTree)
	{
		return ErrorResult(TEXT("Invalid Widget Blueprint or missing WidgetTree"));
	}

	UWidget* Widget = WBP->WidgetTree->FindWidget(FName(*WidgetName));
	if (!Widget)
	{
		return ErrorResult(FString::Printf(TEXT("Widget not found: %s"), *WidgetName));
	}

	TArray<FString> SetProps;

	if (UImage* Img = Cast<UImage>(Widget))
	{
		if (BrushProperty.IsEmpty() || BrushProperty == TEXT("brush"))
		{
			FSlateBrush Brush = Img->GetBrush();
			SetProps = ApplyBrushProperties(Brush, BrushJson);
			Img->SetBrush(Brush);
		}
		else
		{
			return ErrorResult(FString::Printf(TEXT("Image does not have brush property '%s'. Valid: brush"), *BrushProperty));
		}
	}
	else if (UButton* Btn = Cast<UButton>(Widget))
	{
		FString Prop = BrushProperty.IsEmpty() ? TEXT("style_normal") : BrushProperty;
		FButtonStyle Style = Btn->GetStyle();

		FSlateBrush* TargetBrush = nullptr;
		if (Prop == TEXT("style_normal")) TargetBrush = &Style.Normal;
		else if (Prop == TEXT("style_hovered")) TargetBrush = &Style.Hovered;
		else if (Prop == TEXT("style_pressed")) TargetBrush = &Style.Pressed;
		else if (Prop == TEXT("style_disabled")) TargetBrush = &Style.Disabled;

		if (!TargetBrush)
		{
			return ErrorResult(FString::Printf(TEXT("Button does not have brush property '%s'. Valid: style_normal, style_hovered, style_pressed, style_disabled"), *BrushProperty));
		}

		SetProps = ApplyBrushProperties(*TargetBrush, BrushJson);
		Btn->SetStyle(Style);
	}
	else if (UBorder* BorderWidget = Cast<UBorder>(Widget))
	{
		if (BrushProperty.IsEmpty() || BrushProperty == TEXT("background"))
		{
			FSlateBrush Brush = BorderWidget->Background;
			SetProps = ApplyBrushProperties(Brush, BrushJson);
			BorderWidget->SetBrush(Brush);
		}
		else
		{
			return ErrorResult(FString::Printf(TEXT("Border does not have brush property '%s'. Valid: background"), *BrushProperty));
		}
	}
	else if (UProgressBar* PB = Cast<UProgressBar>(Widget))
	{
		FString Prop = BrushProperty.IsEmpty() ? TEXT("style_fill") : BrushProperty;
		FProgressBarStyle Style = PB->GetWidgetStyle();

		FSlateBrush* TargetBrush = nullptr;
		if (Prop == TEXT("style_fill")) TargetBrush = &Style.FillImage;
		else if (Prop == TEXT("style_background")) TargetBrush = &Style.BackgroundImage;

		if (!TargetBrush)
		{
			return ErrorResult(FString::Printf(TEXT("ProgressBar does not have brush property '%s'. Valid: style_fill, style_background"), *BrushProperty));
		}

		SetProps = ApplyBrushProperties(*TargetBrush, BrushJson);
		PB->SetWidgetStyle(Style);
	}
	else
	{
		return ErrorResult(FString::Printf(TEXT("Widget '%s' (class %s) does not support set_brush"),
			*WidgetName, *Widget->GetClass()->GetName()));
	}

	WBP->MarkPackageDirty();
	FBlueprintEditorUtils::MarkBlueprintAsModified(WBP);

	TSharedPtr<FJsonObject> Result = SuccessResult(
		FString::Printf(TEXT("Set brush on %s: %s"),
			*WidgetName, *FString::Join(SetProps, TEXT(", "))));
	return Result;
}

// ============================================================================
// SetSlotProperty
// ============================================================================

static FMargin ExtractPadding(const TSharedPtr<FJsonObject>& PadObj)
{
	FMargin Pad(0);
	double Val;
	if (PadObj->TryGetNumberField(TEXT("left"), Val)) Pad.Left = static_cast<float>(Val);
	if (PadObj->TryGetNumberField(TEXT("top"), Val)) Pad.Top = static_cast<float>(Val);
	if (PadObj->TryGetNumberField(TEXT("right"), Val)) Pad.Right = static_cast<float>(Val);
	if (PadObj->TryGetNumberField(TEXT("bottom"), Val)) Pad.Bottom = static_cast<float>(Val);
	return Pad;
}

TSharedPtr<FJsonObject> FWidgetEditor::SetSlotProperty(UWidgetBlueprint* WBP, const FString& WidgetName,
	const TSharedPtr<FJsonObject>& SlotProperties)
{
	if (!WBP || !WBP->WidgetTree)
	{
		return ErrorResult(TEXT("Invalid Widget Blueprint or missing WidgetTree"));
	}

	UWidget* Widget = WBP->WidgetTree->FindWidget(FName(*WidgetName));
	if (!Widget)
	{
		return ErrorResult(FString::Printf(TEXT("Widget not found: %s"), *WidgetName));
	}

	UPanelSlot* Slot = Widget->Slot;
	if (!Slot)
	{
		return ErrorResult(FString::Printf(TEXT("Widget '%s' has no slot (is it the root?)"), *WidgetName));
	}

	TArray<FString> SetProps;
	double NumVal;
	bool BoolVal;
	FString StrVal;

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		const TSharedPtr<FJsonObject>* PosObj;
		if (SlotProperties->TryGetObjectField(TEXT("position"), PosObj))
		{
			FVector2D Pos = CanvasSlot->GetPosition();
			double X, Y;
			if ((*PosObj)->TryGetNumberField(TEXT("x"), X)) Pos.X = static_cast<float>(X);
			if ((*PosObj)->TryGetNumberField(TEXT("y"), Y)) Pos.Y = static_cast<float>(Y);
			CanvasSlot->SetPosition(Pos);
			SetProps.Add(TEXT("position"));
		}
		const TSharedPtr<FJsonObject>* SizeObj;
		if (SlotProperties->TryGetObjectField(TEXT("size"), SizeObj))
		{
			FVector2D Size = CanvasSlot->GetSize();
			double X, Y;
			if ((*SizeObj)->TryGetNumberField(TEXT("x"), X)) Size.X = static_cast<float>(X);
			if ((*SizeObj)->TryGetNumberField(TEXT("y"), Y)) Size.Y = static_cast<float>(Y);
			CanvasSlot->SetSize(Size);
			SetProps.Add(TEXT("size"));
		}
		const TSharedPtr<FJsonObject>* AnchorsObj;
		if (SlotProperties->TryGetObjectField(TEXT("anchors"), AnchorsObj))
		{
			FAnchors Anchors = CanvasSlot->GetAnchors();
			double Val;
			if ((*AnchorsObj)->TryGetNumberField(TEXT("min_x"), Val)) Anchors.Minimum.X = static_cast<float>(Val);
			if ((*AnchorsObj)->TryGetNumberField(TEXT("min_y"), Val)) Anchors.Minimum.Y = static_cast<float>(Val);
			if ((*AnchorsObj)->TryGetNumberField(TEXT("max_x"), Val)) Anchors.Maximum.X = static_cast<float>(Val);
			if ((*AnchorsObj)->TryGetNumberField(TEXT("max_y"), Val)) Anchors.Maximum.Y = static_cast<float>(Val);
			CanvasSlot->SetAnchors(Anchors);
			SetProps.Add(TEXT("anchors"));
		}
		const TSharedPtr<FJsonObject>* AlignObj;
		if (SlotProperties->TryGetObjectField(TEXT("alignment"), AlignObj))
		{
			FVector2D Align = CanvasSlot->GetAlignment();
			double X, Y;
			if ((*AlignObj)->TryGetNumberField(TEXT("x"), X)) Align.X = static_cast<float>(X);
			if ((*AlignObj)->TryGetNumberField(TEXT("y"), Y)) Align.Y = static_cast<float>(Y);
			CanvasSlot->SetAlignment(Align);
			SetProps.Add(TEXT("alignment"));
		}
		if (SlotProperties->TryGetBoolField(TEXT("auto_size"), BoolVal))
		{
			CanvasSlot->SetAutoSize(BoolVal);
			SetProps.Add(TEXT("auto_size"));
		}
		if (SlotProperties->TryGetNumberField(TEXT("z_order"), NumVal))
		{
			CanvasSlot->SetZOrder(static_cast<int32>(NumVal));
			SetProps.Add(TEXT("z_order"));
		}
	}
	else if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(Slot))
	{
		if (SlotProperties->TryGetStringField(TEXT("h_align"), StrVal))
		{
			OverlaySlot->SetHorizontalAlignment(StringToHAlign(StrVal));
			SetProps.Add(TEXT("h_align"));
		}
		if (SlotProperties->TryGetStringField(TEXT("v_align"), StrVal))
		{
			OverlaySlot->SetVerticalAlignment(StringToVAlign(StrVal));
			SetProps.Add(TEXT("v_align"));
		}
		const TSharedPtr<FJsonObject>* PadObj;
		if (SlotProperties->TryGetObjectField(TEXT("padding"), PadObj))
		{
			OverlaySlot->SetPadding(ExtractPadding(*PadObj));
			SetProps.Add(TEXT("padding"));
		}
	}
	else if (UVerticalBoxSlot* VSlot = Cast<UVerticalBoxSlot>(Slot))
	{
		if (SlotProperties->TryGetStringField(TEXT("h_align"), StrVal))
		{
			VSlot->SetHorizontalAlignment(StringToHAlign(StrVal));
			SetProps.Add(TEXT("h_align"));
		}
		if (SlotProperties->TryGetStringField(TEXT("v_align"), StrVal))
		{
			VSlot->SetVerticalAlignment(StringToVAlign(StrVal));
			SetProps.Add(TEXT("v_align"));
		}
		if (SlotProperties->TryGetStringField(TEXT("size_rule"), StrVal))
		{
			FSlateChildSize SizeRule = VSlot->GetSize();
			if (StrVal.Equals(TEXT("Auto"), ESearchCase::IgnoreCase))
				SizeRule.SizeRule = ESlateSizeRule::Automatic;
			else if (StrVal.Equals(TEXT("Fill"), ESearchCase::IgnoreCase))
				SizeRule.SizeRule = ESlateSizeRule::Fill;
			VSlot->SetSize(SizeRule);
			SetProps.Add(TEXT("size_rule"));
		}
		if (SlotProperties->TryGetNumberField(TEXT("fill_weight"), NumVal))
		{
			FSlateChildSize SizeRule = VSlot->GetSize();
			SizeRule.Value = static_cast<float>(NumVal);
			VSlot->SetSize(SizeRule);
			SetProps.Add(TEXT("fill_weight"));
		}
		const TSharedPtr<FJsonObject>* PadObj;
		if (SlotProperties->TryGetObjectField(TEXT("padding"), PadObj))
		{
			VSlot->SetPadding(ExtractPadding(*PadObj));
			SetProps.Add(TEXT("padding"));
		}
	}
	else if (UHorizontalBoxSlot* HSlot = Cast<UHorizontalBoxSlot>(Slot))
	{
		if (SlotProperties->TryGetStringField(TEXT("h_align"), StrVal))
		{
			HSlot->SetHorizontalAlignment(StringToHAlign(StrVal));
			SetProps.Add(TEXT("h_align"));
		}
		if (SlotProperties->TryGetStringField(TEXT("v_align"), StrVal))
		{
			HSlot->SetVerticalAlignment(StringToVAlign(StrVal));
			SetProps.Add(TEXT("v_align"));
		}
		if (SlotProperties->TryGetStringField(TEXT("size_rule"), StrVal))
		{
			FSlateChildSize SizeRule = HSlot->GetSize();
			if (StrVal.Equals(TEXT("Auto"), ESearchCase::IgnoreCase))
				SizeRule.SizeRule = ESlateSizeRule::Automatic;
			else if (StrVal.Equals(TEXT("Fill"), ESearchCase::IgnoreCase))
				SizeRule.SizeRule = ESlateSizeRule::Fill;
			HSlot->SetSize(SizeRule);
			SetProps.Add(TEXT("size_rule"));
		}
		if (SlotProperties->TryGetNumberField(TEXT("fill_weight"), NumVal))
		{
			FSlateChildSize SizeRule = HSlot->GetSize();
			SizeRule.Value = static_cast<float>(NumVal);
			HSlot->SetSize(SizeRule);
			SetProps.Add(TEXT("fill_weight"));
		}
		const TSharedPtr<FJsonObject>* PadObj;
		if (SlotProperties->TryGetObjectField(TEXT("padding"), PadObj))
		{
			HSlot->SetPadding(ExtractPadding(*PadObj));
			SetProps.Add(TEXT("padding"));
		}
	}
	else if (UScrollBoxSlot* SBSlot = Cast<UScrollBoxSlot>(Slot))
	{
		if (SlotProperties->TryGetStringField(TEXT("h_align"), StrVal))
		{
			SBSlot->SetHorizontalAlignment(StringToHAlign(StrVal));
			SetProps.Add(TEXT("h_align"));
		}
		if (SlotProperties->TryGetStringField(TEXT("v_align"), StrVal))
		{
			SBSlot->SetVerticalAlignment(StringToVAlign(StrVal));
			SetProps.Add(TEXT("v_align"));
		}
		const TSharedPtr<FJsonObject>* PadObj;
		if (SlotProperties->TryGetObjectField(TEXT("padding"), PadObj))
		{
			SBSlot->SetPadding(ExtractPadding(*PadObj));
			SetProps.Add(TEXT("padding"));
		}
	}
	else if (UGridSlot* GSlot = Cast<UGridSlot>(Slot))
	{
		if (SlotProperties->TryGetNumberField(TEXT("row"), NumVal))
		{
			GSlot->SetRow(static_cast<int32>(NumVal));
			SetProps.Add(TEXT("row"));
		}
		if (SlotProperties->TryGetNumberField(TEXT("column"), NumVal))
		{
			GSlot->SetColumn(static_cast<int32>(NumVal));
			SetProps.Add(TEXT("column"));
		}
		if (SlotProperties->TryGetNumberField(TEXT("row_span"), NumVal))
		{
			GSlot->SetRowSpan(static_cast<int32>(NumVal));
			SetProps.Add(TEXT("row_span"));
		}
		if (SlotProperties->TryGetNumberField(TEXT("column_span"), NumVal))
		{
			GSlot->SetColumnSpan(static_cast<int32>(NumVal));
			SetProps.Add(TEXT("column_span"));
		}
		if (SlotProperties->TryGetStringField(TEXT("h_align"), StrVal))
		{
			GSlot->SetHorizontalAlignment(StringToHAlign(StrVal));
			SetProps.Add(TEXT("h_align"));
		}
		if (SlotProperties->TryGetStringField(TEXT("v_align"), StrVal))
		{
			GSlot->SetVerticalAlignment(StringToVAlign(StrVal));
			SetProps.Add(TEXT("v_align"));
		}
		const TSharedPtr<FJsonObject>* PadObj;
		if (SlotProperties->TryGetObjectField(TEXT("padding"), PadObj))
		{
			GSlot->SetPadding(ExtractPadding(*PadObj));
			SetProps.Add(TEXT("padding"));
		}
	}
	else
	{
		return ErrorResult(FString::Printf(TEXT("Unsupported slot type: %s"), *Slot->GetClass()->GetName()));
	}

	if (SetProps.Num() == 0)
	{
		return ErrorResult(TEXT("No recognized slot properties were set. Check property names and slot type."));
	}

	WBP->MarkPackageDirty();

	TSharedPtr<FJsonObject> Result = SuccessResult(
		FString::Printf(TEXT("Set %d slot properties on %s: %s"),
			SetProps.Num(), *WidgetName, *FString::Join(SetProps, TEXT(", "))));
	return Result;
}

// ============================================================================
// ReparentWidget
// ============================================================================

TSharedPtr<FJsonObject> FWidgetEditor::ReparentWidget(UWidgetBlueprint* WBP, const FString& WidgetName, const FString& NewParentName)
{
	if (!WBP || !WBP->WidgetTree)
	{
		return ErrorResult(TEXT("Invalid Widget Blueprint or missing WidgetTree"));
	}

	UWidget* Widget = WBP->WidgetTree->FindWidget(FName(*WidgetName));
	if (!Widget)
	{
		return ErrorResult(FString::Printf(TEXT("Widget not found: %s"), *WidgetName));
	}

	if (Widget == WBP->WidgetTree->RootWidget)
	{
		return ErrorResult(TEXT("Cannot reparent the root widget"));
	}

	UWidget* NewParentWidget = WBP->WidgetTree->FindWidget(FName(*NewParentName));
	if (!NewParentWidget)
	{
		return ErrorResult(FString::Printf(TEXT("New parent widget not found: %s"), *NewParentName));
	}

	UPanelWidget* NewParent = Cast<UPanelWidget>(NewParentWidget);
	if (!NewParent)
	{
		return ErrorResult(FString::Printf(TEXT("New parent '%s' is not a panel widget (is %s)"),
			*NewParentName, *NewParentWidget->GetClass()->GetName()));
	}

	UPanelWidget* Ancestor = Cast<UPanelWidget>(NewParent);
	while (Ancestor)
	{
		if (Ancestor == Widget)
		{
			return ErrorResult(FString::Printf(
				TEXT("Cannot reparent '%s' to '%s' — would create a cycle (new parent is a descendant)"),
				*WidgetName, *NewParentName));
		}
		Ancestor = Ancestor->GetParent();
	}

	UPanelWidget* OldParent = Widget->GetParent();
	FString OldParentName = OldParent ? OldParent->GetName() : TEXT("None");

	if (OldParent)
	{
		OldParent->RemoveChild(Widget);
	}

	UPanelSlot* NewSlot = NewParent->AddChild(Widget);
	if (!NewSlot)
	{
		if (OldParent)
		{
			OldParent->AddChild(Widget);
		}
		return ErrorResult(TEXT("Failed to add widget to new parent"));
	}

	WBP->MarkPackageDirty();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WBP);

	TSharedPtr<FJsonObject> Result = SuccessResult(
		FString::Printf(TEXT("Reparented '%s' from '%s' to '%s'"), *WidgetName, *OldParentName, *NewParentName));
	Result->SetStringField(TEXT("widget_name"), WidgetName);
	Result->SetStringField(TEXT("old_parent"), OldParentName);
	Result->SetStringField(TEXT("new_parent"), NewParentName);
	Result->SetStringField(TEXT("new_slot_type"), GetSlotTypeName(NewSlot));
	return Result;
}

// ============================================================================
// ReorderChild
// ============================================================================

TSharedPtr<FJsonObject> FWidgetEditor::ReorderChild(UWidgetBlueprint* WBP, const FString& ParentName, const FString& ChildName, int32 NewIndex)
{
	if (!WBP || !WBP->WidgetTree)
	{
		return ErrorResult(TEXT("Invalid Widget Blueprint or missing WidgetTree"));
	}

	UWidget* ParentWidget = WBP->WidgetTree->FindWidget(FName(*ParentName));
	if (!ParentWidget)
	{
		return ErrorResult(FString::Printf(TEXT("Parent widget not found: %s"), *ParentName));
	}

	UPanelWidget* Parent = Cast<UPanelWidget>(ParentWidget);
	if (!Parent)
	{
		return ErrorResult(FString::Printf(TEXT("'%s' is not a panel widget (is %s)"),
			*ParentName, *ParentWidget->GetClass()->GetName()));
	}

	UWidget* Child = WBP->WidgetTree->FindWidget(FName(*ChildName));
	if (!Child)
	{
		return ErrorResult(FString::Printf(TEXT("Child widget not found: %s"), *ChildName));
	}

	int32 CurrentIndex = Parent->GetChildIndex(Child);
	if (CurrentIndex == INDEX_NONE)
	{
		return ErrorResult(FString::Printf(TEXT("'%s' is not a child of '%s'"), *ChildName, *ParentName));
	}

	int32 ClampedIndex = FMath::Clamp(NewIndex, 0, Parent->GetChildrenCount() - 1);
	Parent->ShiftChild(ClampedIndex, Child);

	WBP->MarkPackageDirty();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WBP);

	TSharedPtr<FJsonObject> Result = SuccessResult(
		FString::Printf(TEXT("Reordered '%s' in '%s' from index %d to %d"), *ChildName, *ParentName, CurrentIndex, ClampedIndex));
	Result->SetStringField(TEXT("parent_name"), ParentName);
	Result->SetStringField(TEXT("child_name"), ChildName);
	Result->SetNumberField(TEXT("new_index"), ClampedIndex);
	return Result;
}

// ============================================================================
// CloneWidget
// ============================================================================

static UWidget* CloneWidgetRecursive(UWidgetBlueprint* WBP, UWidget* Source, const FString& NewName)
{
	if (!Source) return nullptr;

	UWidget* Clone = WBP->WidgetTree->ConstructWidget<UWidget>(Source->GetClass(), FName(*NewName));
	if (!Clone) return nullptr;

	UEngine::CopyPropertiesForUnrelatedObjects(Source, Clone);

	UPanelWidget* SourcePanel = Cast<UPanelWidget>(Source);
	UPanelWidget* ClonePanel = Cast<UPanelWidget>(Clone);
	if (SourcePanel && ClonePanel)
	{
		for (int32 i = 0; i < SourcePanel->GetChildrenCount(); ++i)
		{
			UWidget* SourceChild = SourcePanel->GetChildAt(i);
			if (!SourceChild) continue;

			FString ChildCloneName = FString::Printf(TEXT("%s_%s"), *NewName, *SourceChild->GetName());
			if (WBP->WidgetTree->FindWidget(FName(*ChildCloneName)))
			{
				ChildCloneName = MakeUniqueObjectName(WBP->WidgetTree, SourceChild->GetClass(), FName(*ChildCloneName)).ToString();
			}

			UWidget* ChildClone = CloneWidgetRecursive(WBP, SourceChild, ChildCloneName);
			if (ChildClone)
			{
				ClonePanel->AddChild(ChildClone);
			}
		}
	}

	return Clone;
}

TSharedPtr<FJsonObject> FWidgetEditor::CloneWidget(UWidgetBlueprint* WBP, const FString& SourceWidgetName, const FString& NewName, const FString& TargetParentName)
{
	if (!WBP || !WBP->WidgetTree)
	{
		return ErrorResult(TEXT("Invalid Widget Blueprint or missing WidgetTree"));
	}

	UWidget* Source = WBP->WidgetTree->FindWidget(FName(*SourceWidgetName));
	if (!Source)
	{
		return ErrorResult(FString::Printf(TEXT("Source widget not found: %s"), *SourceWidgetName));
	}

	if (WBP->WidgetTree->FindWidget(FName(*NewName)))
	{
		return ErrorResult(FString::Printf(TEXT("Widget with name '%s' already exists"), *NewName));
	}

	UPanelWidget* TargetParent = nullptr;
	if (TargetParentName.IsEmpty())
	{
		TargetParent = Source->GetParent();
	}
	else
	{
		UWidget* TargetWidget = WBP->WidgetTree->FindWidget(FName(*TargetParentName));
		if (!TargetWidget)
		{
			return ErrorResult(FString::Printf(TEXT("Target parent not found: %s"), *TargetParentName));
		}
		TargetParent = Cast<UPanelWidget>(TargetWidget);
		if (!TargetParent)
		{
			return ErrorResult(FString::Printf(TEXT("Target parent '%s' is not a panel widget (is %s)"),
				*TargetParentName, *TargetWidget->GetClass()->GetName()));
		}
	}

	if (!TargetParent)
	{
		return ErrorResult(TEXT("No valid target parent (source widget has no parent and no target specified)"));
	}

	int32 WidgetsBefore = 0;
	TArray<UWidget*> AllBefore;
	WBP->WidgetTree->GetAllWidgets(AllBefore);
	WidgetsBefore = AllBefore.Num();

	UWidget* Clone = CloneWidgetRecursive(WBP, Source, NewName);
	if (!Clone)
	{
		return ErrorResult(TEXT("Failed to clone widget"));
	}

	UPanelSlot* Slot = TargetParent->AddChild(Clone);
	if (!Slot)
	{
		return ErrorResult(TEXT("Failed to add cloned widget to target parent"));
	}

	TArray<UWidget*> AllAfter;
	WBP->WidgetTree->GetAllWidgets(AllAfter);
	int32 WidgetsCloned = AllAfter.Num() - WidgetsBefore;

	WBP->MarkPackageDirty();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WBP);

	TSharedPtr<FJsonObject> Result = SuccessResult(
		FString::Printf(TEXT("Cloned '%s' as '%s' (%d widgets)"), *SourceWidgetName, *NewName, WidgetsCloned));
	Result->SetStringField(TEXT("source_name"), SourceWidgetName);
	Result->SetStringField(TEXT("clone_name"), NewName);
	Result->SetNumberField(TEXT("widgets_cloned"), WidgetsCloned);
	Result->SetStringField(TEXT("target_parent"), TargetParent->GetName());
	return Result;
}

// ============================================================================
// ListWidgetEvents
// ============================================================================

TSharedPtr<FJsonObject> FWidgetEditor::ListWidgetEvents(const FString& AssetPath, const FString& WidgetName)
{
	FString LoadError;
	UWidgetBlueprint* WBP = LoadWidgetBlueprint(AssetPath, LoadError);
	if (!WBP)
	{
		return ErrorResult(LoadError);
	}

	if (!WBP->WidgetTree)
	{
		return ErrorResult(TEXT("Widget Blueprint has no WidgetTree"));
	}

	UWidget* Widget = WBP->WidgetTree->FindWidget(FName(*WidgetName));
	if (!Widget)
	{
		return ErrorResult(FString::Printf(TEXT("Widget not found: %s"), *WidgetName));
	}

	TArray<TSharedPtr<FJsonValue>> EventsArray;
	UClass* WidgetClass = Widget->GetClass();

	for (TFieldIterator<FMulticastDelegateProperty> It(WidgetClass); It; ++It)
	{
		FMulticastDelegateProperty* DelegateProp = *It;
		if (!DelegateProp)
		{
			continue;
		}

		TSharedPtr<FJsonObject> EventObj = MakeShared<FJsonObject>();
		EventObj->SetStringField(TEXT("name"), DelegateProp->GetName());
		EventObj->SetStringField(TEXT("owner_class"), DelegateProp->GetOwnerClass()->GetName());

		if (DelegateProp->SignatureFunction)
		{
			TArray<TSharedPtr<FJsonValue>> ParamsArray;
			for (TFieldIterator<FProperty> ParamIt(DelegateProp->SignatureFunction); ParamIt; ++ParamIt)
			{
				FProperty* Param = *ParamIt;
				if (Param->HasAnyPropertyFlags(CPF_Parm) && !Param->HasAnyPropertyFlags(CPF_ReturnParm))
				{
					TSharedPtr<FJsonObject> ParamObj = MakeShared<FJsonObject>();
					ParamObj->SetStringField(TEXT("name"), Param->GetName());
					ParamObj->SetStringField(TEXT("type"), Param->GetCPPType());
					ParamsArray.Add(MakeShared<FJsonValueObject>(ParamObj));
				}
			}
			EventObj->SetArrayField(TEXT("parameters"), ParamsArray);
		}

		EventsArray.Add(MakeShared<FJsonValueObject>(EventObj));
	}

	TSharedPtr<FJsonObject> Result = SuccessResult(
		FString::Printf(TEXT("Found %d events on %s (%s)"), EventsArray.Num(), *WidgetName, *WidgetClass->GetName()));
	Result->SetStringField(TEXT("widget_name"), WidgetName);
	Result->SetStringField(TEXT("widget_class"), WidgetClass->GetName());
	Result->SetArrayField(TEXT("events"), EventsArray);
	return Result;
}

// ============================================================================
// BindEvent
// ============================================================================

TSharedPtr<FJsonObject> FWidgetEditor::BindEvent(UWidgetBlueprint* WBP, const FString& WidgetName,
	const FString& EventName, const FString& FunctionName)
{
	if (!WBP || !WBP->WidgetTree)
	{
		return ErrorResult(TEXT("Invalid Widget Blueprint or missing WidgetTree"));
	}

	UWidget* Widget = WBP->WidgetTree->FindWidget(FName(*WidgetName));
	if (!Widget)
	{
		return ErrorResult(FString::Printf(TEXT("Widget not found: %s"), *WidgetName));
	}

	FMulticastDelegateProperty* DelegateProp = FindFProperty<FMulticastDelegateProperty>(
		Widget->GetClass(), FName(*EventName));
	if (!DelegateProp)
	{
		return ErrorResult(FString::Printf(TEXT("Delegate event '%s' not found on widget class %s"),
			*EventName, *Widget->GetClass()->GetName()));
	}

	UClass* GenClass = WBP->SkeletonGeneratedClass ? WBP->SkeletonGeneratedClass : WBP->GeneratedClass;
	if (!GenClass)
	{
		FKismetEditorUtilities::CompileBlueprint(WBP);
		GenClass = WBP->SkeletonGeneratedClass ? WBP->SkeletonGeneratedClass : WBP->GeneratedClass;
	}
	if (!GenClass)
	{
		return ErrorResult(TEXT("Could not get generated class for Widget Blueprint"));
	}

	FObjectProperty* ComponentProp = FindFProperty<FObjectProperty>(GenClass, FName(*WidgetName));
	if (!ComponentProp)
	{
		return ErrorResult(FString::Printf(
			TEXT("Widget '%s' is not a variable in the Widget Blueprint. Make sure 'Is Variable' is checked."),
			*WidgetName));
	}

	if (FKismetEditorUtilities::FindBoundEventForComponent(WBP, DelegateProp->GetFName(), FName(*WidgetName)))
	{
		return ErrorResult(FString::Printf(
			TEXT("Event '%s' is already bound for widget '%s'"), *EventName, *WidgetName));
	}

	if (WBP->UbergraphPages.Num() == 0)
	{
		return ErrorResult(TEXT("Widget Blueprint has no event graph (UbergraphPages is empty)"));
	}

	UEdGraph* Graph = WBP->UbergraphPages[0];

	int32 NodeX = 0;
	int32 NodeY = 0;
	for (UEdGraphNode* ExistingNode : Graph->Nodes)
	{
		if (ExistingNode)
		{
			NodeY = FMath::Max(NodeY, ExistingNode->NodePosY + 200);
		}
	}

	UK2Node_ComponentBoundEvent* EventNode = NewObject<UK2Node_ComponentBoundEvent>(Graph);
	EventNode->InitializeComponentBoundEventParams(ComponentProp, DelegateProp);
	EventNode->NodePosX = NodeX;
	EventNode->NodePosY = NodeY;

	Graph->AddNode(EventNode, false, false);
	EventNode->AllocateDefaultPins();
	EventNode->ReconstructNode();

	FKismetEditorUtilities::CompileBlueprint(WBP);
	WBP->MarkPackageDirty();

	TSharedPtr<FJsonObject> Result = SuccessResult(
		FString::Printf(TEXT("Bound event '%s' on widget '%s'"), *EventName, *WidgetName));
	Result->SetStringField(TEXT("widget_name"), WidgetName);
	Result->SetStringField(TEXT("event_name"), EventName);
	Result->SetStringField(TEXT("node_name"), EventNode->GetName());
	Result->SetNumberField(TEXT("node_x"), NodeX);
	Result->SetNumberField(TEXT("node_y"), NodeY);
	if (EventNode->GetFunctionName() != NAME_None)
	{
		Result->SetStringField(TEXT("function_name"), EventNode->GetFunctionName().ToString());
	}
	return Result;
}

// ============================================================================
// Save
// ============================================================================

TSharedPtr<FJsonObject> FWidgetEditor::SaveWidgetBlueprint(UWidgetBlueprint* WBP)
{
	if (!WBP)
	{
		return ErrorResult(TEXT("Null Widget Blueprint"));
	}

	FString AssetPath = WBP->GetPathName();
	bool bSaved = UEditorAssetLibrary::SaveAsset(AssetPath, false);
	if (bSaved)
	{
		return SuccessResult(FString::Printf(TEXT("Saved: %s"), *AssetPath));
	}
	return ErrorResult(FString::Printf(TEXT("Failed to save: %s"), *AssetPath));
}

// ============================================================================
// Batch
// ============================================================================

TSharedPtr<FJsonObject> FWidgetEditor::DispatchBatchOp(UWidgetBlueprint* WBP, const TSharedPtr<FJsonObject>& OpData)
{
	FString OpName;
	OpData->TryGetStringField(TEXT("op"), OpName);
	OpName = OpName.ToLower();

	if (OpName == TEXT("add_widget"))
	{
		FString WidgetClass, WidgetName, ParentName;
		if (!OpData->TryGetStringField(TEXT("widget_class"), WidgetClass) || WidgetClass.IsEmpty())
		{
			return ErrorResult(TEXT("add_widget: missing widget_class"));
		}
		if (!OpData->TryGetStringField(TEXT("widget_name"), WidgetName) || WidgetName.IsEmpty())
		{
			return ErrorResult(TEXT("add_widget: missing widget_name"));
		}
		OpData->TryGetStringField(TEXT("parent_name"), ParentName);
		return AddWidget(WBP, WidgetClass, WidgetName, ParentName);
	}
	else if (OpName == TEXT("remove_widget"))
	{
		FString WidgetName;
		if (!OpData->TryGetStringField(TEXT("widget_name"), WidgetName) || WidgetName.IsEmpty())
		{
			return ErrorResult(TEXT("remove_widget: missing widget_name"));
		}
		return RemoveWidget(WBP, WidgetName);
	}
	else if (OpName == TEXT("set_property"))
	{
		FString WidgetName;
		if (!OpData->TryGetStringField(TEXT("widget_name"), WidgetName) || WidgetName.IsEmpty())
		{
			return ErrorResult(TEXT("set_property: missing widget_name"));
		}
		const TSharedPtr<FJsonObject>* PropsObj;
		if (!OpData->TryGetObjectField(TEXT("properties"), PropsObj))
		{
			return ErrorResult(TEXT("set_property: missing properties object"));
		}
		return SetWidgetProperty(WBP, WidgetName, *PropsObj);
	}
	else if (OpName == TEXT("set_slot"))
	{
		FString WidgetName;
		if (!OpData->TryGetStringField(TEXT("widget_name"), WidgetName) || WidgetName.IsEmpty())
		{
			return ErrorResult(TEXT("set_slot: missing widget_name"));
		}
		const TSharedPtr<FJsonObject>* SlotObj;
		if (!OpData->TryGetObjectField(TEXT("slot_properties"), SlotObj))
		{
			return ErrorResult(TEXT("set_slot: missing slot_properties object"));
		}
		return SetSlotProperty(WBP, WidgetName, *SlotObj);
	}
	else if (OpName == TEXT("set_brush"))
	{
		FString WidgetName;
		if (!OpData->TryGetStringField(TEXT("widget_name"), WidgetName) || WidgetName.IsEmpty())
		{
			return ErrorResult(TEXT("set_brush: missing widget_name"));
		}
		const TSharedPtr<FJsonObject>* BrushObj;
		if (!OpData->TryGetObjectField(TEXT("brush"), BrushObj))
		{
			return ErrorResult(TEXT("set_brush: missing brush object"));
		}
		FString BrushProp;
		OpData->TryGetStringField(TEXT("brush_property"), BrushProp);
		return SetBrush(WBP, WidgetName, BrushProp, *BrushObj);
	}
	else if (OpName == TEXT("reparent_widget"))
	{
		FString WidgetName, NewParentName;
		if (!OpData->TryGetStringField(TEXT("widget_name"), WidgetName) || WidgetName.IsEmpty())
		{
			return ErrorResult(TEXT("reparent_widget: missing widget_name"));
		}
		if (!OpData->TryGetStringField(TEXT("new_parent_name"), NewParentName) || NewParentName.IsEmpty())
		{
			return ErrorResult(TEXT("reparent_widget: missing new_parent_name"));
		}
		return ReparentWidget(WBP, WidgetName, NewParentName);
	}
	else if (OpName == TEXT("reorder_child"))
	{
		FString ParentName, ChildName;
		double IndexVal = 0;
		if (!OpData->TryGetStringField(TEXT("parent_name"), ParentName) || ParentName.IsEmpty())
		{
			return ErrorResult(TEXT("reorder_child: missing parent_name"));
		}
		if (!OpData->TryGetStringField(TEXT("child_name"), ChildName) || ChildName.IsEmpty())
		{
			return ErrorResult(TEXT("reorder_child: missing child_name"));
		}
		if (!OpData->TryGetNumberField(TEXT("new_index"), IndexVal))
		{
			return ErrorResult(TEXT("reorder_child: missing new_index"));
		}
		return ReorderChild(WBP, ParentName, ChildName, static_cast<int32>(IndexVal));
	}
	else if (OpName == TEXT("clone_widget"))
	{
		FString SourceName, CloneName, TargetParent;
		if (!OpData->TryGetStringField(TEXT("widget_name"), SourceName) || SourceName.IsEmpty())
		{
			return ErrorResult(TEXT("clone_widget: missing widget_name"));
		}
		if (!OpData->TryGetStringField(TEXT("new_name"), CloneName) || CloneName.IsEmpty())
		{
			return ErrorResult(TEXT("clone_widget: missing new_name"));
		}
		OpData->TryGetStringField(TEXT("target_parent"), TargetParent);
		return CloneWidget(WBP, SourceName, CloneName, TargetParent);
	}

	return ErrorResult(FString::Printf(
		TEXT("Unknown batch op: '%s'. Valid: add_widget, remove_widget, set_property, set_slot, set_brush, reparent_widget, reorder_child, clone_widget"),
		*OpName));
}

TSharedPtr<FJsonObject> FWidgetEditor::ExecuteBatch(UWidgetBlueprint* WBP, const TArray<TSharedPtr<FJsonValue>>& Operations)
{
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();

	if (!WBP)
	{
		Result->SetBoolField(TEXT("success"), false);
		Result->SetStringField(TEXT("error"), TEXT("Null Widget Blueprint"));
		return Result;
	}

	TArray<TSharedPtr<FJsonValue>> ResultsArray;
	int32 OKCount = 0;
	int32 ErrCount = 0;

	for (int32 i = 0; i < Operations.Num(); ++i)
	{
		if (!Operations[i].IsValid() || Operations[i]->Type != EJson::Object)
		{
			TSharedPtr<FJsonObject> SkipResult = ErrorResult(
				FString::Printf(TEXT("[%d] not a JSON object"), i));
			ResultsArray.Add(MakeShared<FJsonValueObject>(SkipResult));
			ErrCount++;
			continue;
		}

		TSharedPtr<FJsonObject> OpData = Operations[i]->AsObject();
		FString OpName;
		if (!OpData->TryGetStringField(TEXT("op"), OpName) || OpName.IsEmpty())
		{
			TSharedPtr<FJsonObject> SkipResult = ErrorResult(
				FString::Printf(TEXT("[%d] missing 'op' field"), i));
			ResultsArray.Add(MakeShared<FJsonValueObject>(SkipResult));
			ErrCount++;
			continue;
		}

		TSharedPtr<FJsonObject> OpResult = DispatchBatchOp(WBP, OpData);
		ResultsArray.Add(MakeShared<FJsonValueObject>(OpResult));

		bool bSuccess = false;
		OpResult->TryGetBoolField(TEXT("success"), bSuccess);
		if (bSuccess)
		{
			OKCount++;
		}
		else
		{
			ErrCount++;
		}
	}

	Result->SetBoolField(TEXT("success"), ErrCount == 0);
	Result->SetArrayField(TEXT("results"), ResultsArray);
	Result->SetNumberField(TEXT("ok_count"), OKCount);
	Result->SetNumberField(TEXT("error_count"), ErrCount);
	Result->SetNumberField(TEXT("total"), Operations.Num());

	return Result;
}

// ============================================================================
// Animation Helpers
// ============================================================================

static UWidgetAnimation* FindAnimationByName(UWidgetBlueprint* WBP, const FString& AnimationName)
{
	if (!WBP) return nullptr;
	for (UWidgetAnimation* Anim : WBP->Animations)
	{
		if (!Anim) continue;
		FString DisplayName = Anim->GetDisplayLabel().IsEmpty() ? Anim->GetName() : Anim->GetDisplayLabel();
		if (DisplayName.Equals(AnimationName, ESearchCase::IgnoreCase) ||
			Anim->GetName().Equals(AnimationName, ESearchCase::IgnoreCase))
		{
			return Anim;
		}
	}
	return nullptr;
}

static bool ResolveAnimTrackProperty(const FString& TrackType, FName& OutName, FString& OutPath)
{
	FString Lower = TrackType.ToLower();
	if (Lower == TEXT("opacity"))        { OutName = FName("RenderOpacity"); OutPath = TEXT("RenderOpacity"); return true; }
	if (Lower == TEXT("translation_x"))  { OutName = FName("Translation.X"); OutPath = TEXT("RenderTransform.Translation.X"); return true; }
	if (Lower == TEXT("translation_y"))  { OutName = FName("Translation.Y"); OutPath = TEXT("RenderTransform.Translation.Y"); return true; }
	if (Lower == TEXT("scale_x"))        { OutName = FName("Scale.X"); OutPath = TEXT("RenderTransform.Scale.X"); return true; }
	if (Lower == TEXT("scale_y"))        { OutName = FName("Scale.Y"); OutPath = TEXT("RenderTransform.Scale.Y"); return true; }
	if (Lower == TEXT("rotation"))       { OutName = FName("Angle"); OutPath = TEXT("RenderTransform.Angle"); return true; }
	return false;
}

static EMovieSceneKeyInterpolation ParseKeyInterpolation(const FString& InterpStr)
{
	FString Lower = InterpStr.ToLower();
	if (Lower == TEXT("linear"))   return EMovieSceneKeyInterpolation::Linear;
	if (Lower == TEXT("constant")) return EMovieSceneKeyInterpolation::Constant;
	return EMovieSceneKeyInterpolation::SmartAuto;
}

// ============================================================================
// Property Binding
// ============================================================================

TSharedPtr<FJsonObject> FWidgetEditor::BindProperty(UWidgetBlueprint* WBP, const FString& WidgetName,
	const FString& PropertyName, const FString& FunctionName)
{
	if (!WBP || !WBP->WidgetTree)
	{
		return ErrorResult(TEXT("Invalid Widget Blueprint or missing WidgetTree"));
	}

	UWidget* Widget = WBP->WidgetTree->FindWidget(FName(*WidgetName));
	if (!Widget)
	{
		return ErrorResult(FString::Printf(TEXT("Widget '%s' not found in tree"), *WidgetName));
	}

	UFunction* Func = WBP->GeneratedClass ? WBP->GeneratedClass->FindFunctionByName(FName(*FunctionName)) : nullptr;
	if (!Func && WBP->SkeletonGeneratedClass)
	{
		Func = WBP->SkeletonGeneratedClass->FindFunctionByName(FName(*FunctionName));
	}

	FDelegateEditorBinding NewBinding;
	NewBinding.ObjectName = WidgetName;
	NewBinding.PropertyName = FName(*PropertyName);
	NewBinding.FunctionName = FName(*FunctionName);
	NewBinding.Kind = EBindingKind::Function;

	int32 ExistingIdx = WBP->Bindings.IndexOfByPredicate([&](const FDelegateEditorBinding& B)
	{
		return B.ObjectName == WidgetName && B.PropertyName == FName(*PropertyName);
	});

	if (ExistingIdx != INDEX_NONE)
	{
		WBP->Bindings[ExistingIdx] = NewBinding;
	}
	else
	{
		WBP->Bindings.Add(NewBinding);
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WBP);
	WBP->MarkPackageDirty();

	TSharedPtr<FJsonObject> Result = SuccessResult(
		FString::Printf(TEXT("Bound property '%s' on widget '%s' to function '%s'"),
			*PropertyName, *WidgetName, *FunctionName));
	Result->SetStringField(TEXT("widget_name"), WidgetName);
	Result->SetStringField(TEXT("property_name"), PropertyName);
	Result->SetStringField(TEXT("function_name"), FunctionName);
	Result->SetStringField(TEXT("kind"), TEXT("Function"));
	Result->SetBoolField(TEXT("updated_existing"), ExistingIdx != INDEX_NONE);
	return Result;
}

TSharedPtr<FJsonObject> FWidgetEditor::UnbindProperty(UWidgetBlueprint* WBP, const FString& WidgetName,
	const FString& PropertyName)
{
	if (!WBP)
	{
		return ErrorResult(TEXT("Invalid Widget Blueprint"));
	}

	int32 RemovedCount = WBP->Bindings.RemoveAll([&](const FDelegateEditorBinding& B)
	{
		return B.ObjectName == WidgetName && B.PropertyName == FName(*PropertyName);
	});

	if (RemovedCount == 0)
	{
		return ErrorResult(FString::Printf(TEXT("No binding found for property '%s' on widget '%s'"),
			*PropertyName, *WidgetName));
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WBP);
	WBP->MarkPackageDirty();

	TSharedPtr<FJsonObject> Result = SuccessResult(
		FString::Printf(TEXT("Removed binding for property '%s' on widget '%s'"),
			*PropertyName, *WidgetName));
	Result->SetStringField(TEXT("widget_name"), WidgetName);
	Result->SetStringField(TEXT("property_name"), PropertyName);
	return Result;
}

TSharedPtr<FJsonObject> FWidgetEditor::ListBindings(const FString& AssetPath)
{
	FString LoadError;
	UWidgetBlueprint* WBP = LoadWidgetBlueprint(AssetPath, LoadError);
	if (!WBP)
	{
		return ErrorResult(LoadError);
	}

	TArray<TSharedPtr<FJsonValue>> BindingsArray;
	for (const FDelegateEditorBinding& Binding : WBP->Bindings)
	{
		TSharedPtr<FJsonObject> BindObj = MakeShared<FJsonObject>();
		BindObj->SetStringField(TEXT("widget_name"), Binding.ObjectName);
		BindObj->SetStringField(TEXT("property_name"), Binding.PropertyName.ToString());
		BindObj->SetStringField(TEXT("function_name"), Binding.FunctionName.ToString());
		BindObj->SetStringField(TEXT("source_property"), Binding.SourceProperty.ToString());
		BindObj->SetStringField(TEXT("kind"), Binding.Kind == EBindingKind::Function ? TEXT("Function") : TEXT("Property"));
		BindingsArray.Add(MakeShared<FJsonValueObject>(BindObj));
	}

	TSharedPtr<FJsonObject> Result = SuccessResult(
		FString::Printf(TEXT("Found %d bindings"), BindingsArray.Num()));
	Result->SetStringField(TEXT("asset_path"), AssetPath);
	Result->SetArrayField(TEXT("bindings"), BindingsArray);
	Result->SetNumberField(TEXT("count"), BindingsArray.Num());
	return Result;
}

// ============================================================================
// ListAnimations
// ============================================================================

TSharedPtr<FJsonObject> FWidgetEditor::ListAnimations(const FString& AssetPath)
{
	FString LoadError;
	UWidgetBlueprint* WBP = LoadWidgetBlueprint(AssetPath, LoadError);
	if (!WBP)
	{
		return ErrorResult(LoadError);
	}

	TArray<TSharedPtr<FJsonValue>> AnimArray;
	for (UWidgetAnimation* Anim : WBP->Animations)
	{
		if (!Anim) continue;
		TSharedPtr<FJsonObject> AnimObj = MakeShared<FJsonObject>();
		FString DisplayName = Anim->GetDisplayLabel().IsEmpty() ? Anim->GetName() : Anim->GetDisplayLabel();
		AnimObj->SetStringField(TEXT("name"), DisplayName);
		AnimObj->SetNumberField(TEXT("start_time"), Anim->GetStartTime());
		AnimObj->SetNumberField(TEXT("end_time"), Anim->GetEndTime());
		AnimObj->SetNumberField(TEXT("length"), Anim->GetEndTime() - Anim->GetStartTime());
		AnimObj->SetNumberField(TEXT("bindings_count"), Anim->AnimationBindings.Num());
		AnimArray.Add(MakeShared<FJsonValueObject>(AnimObj));
	}

	TSharedPtr<FJsonObject> Result = SuccessResult(
		FString::Printf(TEXT("Found %d animations"), AnimArray.Num()));
	Result->SetArrayField(TEXT("animations"), AnimArray);
	return Result;
}

// ============================================================================
// InspectAnimation
// ============================================================================

TSharedPtr<FJsonObject> FWidgetEditor::InspectAnimation(const FString& AssetPath, const FString& AnimationName)
{
	FString LoadError;
	UWidgetBlueprint* WBP = LoadWidgetBlueprint(AssetPath, LoadError);
	if (!WBP)
	{
		return ErrorResult(LoadError);
	}

	UWidgetAnimation* Anim = FindAnimationByName(WBP, AnimationName);
	if (!Anim)
	{
		return ErrorResult(FString::Printf(TEXT("Animation not found: %s"), *AnimationName));
	}

	TSharedPtr<FJsonObject> Result = SuccessResult(TEXT("Animation inspected"));
	FString DisplayName = Anim->GetDisplayLabel().IsEmpty() ? Anim->GetName() : Anim->GetDisplayLabel();
	Result->SetStringField(TEXT("name"), DisplayName);
	Result->SetNumberField(TEXT("start_time"), Anim->GetStartTime());
	Result->SetNumberField(TEXT("end_time"), Anim->GetEndTime());

	UMovieScene* MovieScene = Anim->GetMovieScene();

	TArray<TSharedPtr<FJsonValue>> BindingsArray;
	for (const FWidgetAnimationBinding& Binding : Anim->AnimationBindings)
	{
		TSharedPtr<FJsonObject> BindObj = MakeShared<FJsonObject>();
		BindObj->SetStringField(TEXT("widget_name"), Binding.WidgetName.ToString());
		BindObj->SetStringField(TEXT("slot_widget_name"), Binding.SlotWidgetName.ToString());
		BindObj->SetBoolField(TEXT("is_root_widget"), Binding.bIsRootWidget);
		BindObj->SetStringField(TEXT("guid"), Binding.AnimationGuid.ToString());

		TArray<TSharedPtr<FJsonValue>> TracksArray;
		if (MovieScene)
		{
			const TArray<FMovieSceneBinding>& MovieBindings = MovieScene->GetBindings();
			for (const FMovieSceneBinding& MovieBinding : MovieBindings)
			{
				if (MovieBinding.GetObjectGuid() != Binding.AnimationGuid) continue;

				for (UMovieSceneTrack* Track : MovieBinding.GetTracks())
				{
					if (!Track) continue;
					TSharedPtr<FJsonObject> TrackObj = MakeShared<FJsonObject>();
					TrackObj->SetStringField(TEXT("class"), Track->GetClass()->GetName());

					if (UMovieScenePropertyTrack* PropTrack = Cast<UMovieScenePropertyTrack>(Track))
					{
						TrackObj->SetStringField(TEXT("property_name"), PropTrack->GetPropertyName().ToString());
						TrackObj->SetStringField(TEXT("property_path"), PropTrack->GetPropertyPath().ToString());
					}

					TArray<TSharedPtr<FJsonValue>> SectionsArray;
					for (UMovieSceneSection* Section : Track->GetAllSections())
					{
						if (!Section) continue;
						TSharedPtr<FJsonObject> SectionObj = MakeShared<FJsonObject>();

						FMovieSceneChannelProxy& Proxy = Section->GetChannelProxy();
						TArrayView<FMovieSceneFloatChannel*> FloatChannels = Proxy.GetChannels<FMovieSceneFloatChannel>();
						SectionObj->SetNumberField(TEXT("float_channel_count"), FloatChannels.Num());

						if (FloatChannels.Num() > 0)
						{
							FMovieSceneFloatChannel* Channel = FloatChannels[0];
							TMovieSceneChannelData<const FMovieSceneFloatValue> ChannelData = Channel->GetData();
							TArrayView<const FFrameNumber> Times = ChannelData.GetTimes();
							TArrayView<const FMovieSceneFloatValue> Values = ChannelData.GetValues();

							FFrameRate TickRes = MovieScene->GetTickResolution();
							TArray<TSharedPtr<FJsonValue>> KeysArray;
							for (int32 k = 0; k < Times.Num(); ++k)
							{
								TSharedPtr<FJsonObject> KeyObj = MakeShared<FJsonObject>();
								KeyObj->SetNumberField(TEXT("time"), TickRes.AsSeconds(Times[k]));
								KeyObj->SetNumberField(TEXT("value"), Values[k].Value);

								FString InterpStr;
								switch (Values[k].InterpMode)
								{
								case RCIM_Linear:   InterpStr = TEXT("linear"); break;
								case RCIM_Constant: InterpStr = TEXT("constant"); break;
								case RCIM_Cubic:    InterpStr = TEXT("cubic"); break;
								default:            InterpStr = TEXT("unknown"); break;
								}
								KeyObj->SetStringField(TEXT("interp"), InterpStr);
								KeysArray.Add(MakeShared<FJsonValueObject>(KeyObj));
							}
							SectionObj->SetArrayField(TEXT("keyframes"), KeysArray);
						}
						SectionsArray.Add(MakeShared<FJsonValueObject>(SectionObj));
					}
					TrackObj->SetArrayField(TEXT("sections"), SectionsArray);
					TracksArray.Add(MakeShared<FJsonValueObject>(TrackObj));
				}
				break;
			}
		}
		BindObj->SetArrayField(TEXT("tracks"), TracksArray);
		BindingsArray.Add(MakeShared<FJsonValueObject>(BindObj));
	}

	Result->SetArrayField(TEXT("bindings"), BindingsArray);
	return Result;
}

// ============================================================================
// CreateAnimation
// ============================================================================

TSharedPtr<FJsonObject> FWidgetEditor::CreateAnimation(UWidgetBlueprint* WBP, const FString& AnimationName, float Length)
{
	if (!WBP)
	{
		return ErrorResult(TEXT("Invalid Widget Blueprint"));
	}

	if (FindAnimationByName(WBP, AnimationName))
	{
		return ErrorResult(FString::Printf(TEXT("Animation '%s' already exists"), *AnimationName));
	}

	UWidgetAnimation* NewAnim = NewObject<UWidgetAnimation>(WBP, FName(*AnimationName), RF_Transactional);
	NewAnim->SetDisplayLabel(AnimationName);

	UMovieScene* MovieScene = NewObject<UMovieScene>(NewAnim, FName("MovieScene"), RF_Transactional);
	NewAnim->MovieScene = MovieScene;

	FFrameRate TickRes = MovieScene->GetTickResolution();
	int32 EndFrameTick = FMath::RoundToInt32(Length * TickRes.AsDecimal());
	MovieScene->SetPlaybackRange(FFrameNumber(0), EndFrameTick);

	WBP->Animations.Add(NewAnim);

	WBP->MarkPackageDirty();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WBP);

	TSharedPtr<FJsonObject> Result = SuccessResult(
		FString::Printf(TEXT("Created animation '%s' (%.2fs)"), *AnimationName, Length));
	Result->SetStringField(TEXT("animation_name"), AnimationName);
	Result->SetNumberField(TEXT("length"), Length);
	return Result;
}

// ============================================================================
// AddAnimationTrack
// ============================================================================

TSharedPtr<FJsonObject> FWidgetEditor::AddAnimationTrack(UWidgetBlueprint* WBP, const FString& AnimationName,
	const FString& WidgetName, const FString& TrackType, const TArray<TSharedPtr<FJsonValue>>& Keyframes)
{
	if (!WBP || !WBP->WidgetTree)
	{
		return ErrorResult(TEXT("Invalid Widget Blueprint or missing WidgetTree"));
	}

	UWidgetAnimation* Anim = FindAnimationByName(WBP, AnimationName);
	if (!Anim)
	{
		return ErrorResult(FString::Printf(TEXT("Animation not found: %s"), *AnimationName));
	}

	UMovieScene* MovieScene = Anim->GetMovieScene();
	if (!MovieScene)
	{
		return ErrorResult(TEXT("Animation has no MovieScene"));
	}

	UWidget* Widget = WBP->WidgetTree->FindWidget(FName(*WidgetName));
	if (!Widget)
	{
		return ErrorResult(FString::Printf(TEXT("Widget not found: %s"), *WidgetName));
	}

	FName PropName;
	FString PropPath;
	if (!ResolveAnimTrackProperty(TrackType, PropName, PropPath))
	{
		return ErrorResult(FString::Printf(
			TEXT("Unknown track type: '%s'. Valid: opacity, translation_x, translation_y, scale_x, scale_y, rotation"),
			*TrackType));
	}

	FGuid WidgetGuid;
	bool bFoundExisting = false;
	for (const FWidgetAnimationBinding& Binding : Anim->AnimationBindings)
	{
		if (Binding.WidgetName == FName(*WidgetName))
		{
			WidgetGuid = Binding.AnimationGuid;
			bFoundExisting = true;
			break;
		}
	}

	if (!bFoundExisting)
	{
		WidgetGuid = MovieScene->AddPossessable(WidgetName, UWidget::StaticClass());

		FWidgetAnimationBinding NewBinding;
		NewBinding.WidgetName = FName(*WidgetName);
		NewBinding.AnimationGuid = WidgetGuid;
		NewBinding.bIsRootWidget = false;
		Anim->AnimationBindings.Add(NewBinding);
	}

	UMovieSceneFloatTrack* FloatTrack = MovieScene->AddTrack<UMovieSceneFloatTrack>(WidgetGuid);
	if (!FloatTrack)
	{
		return ErrorResult(TEXT("Failed to create float track"));
	}
	FloatTrack->SetPropertyNameAndPath(PropName, PropPath);

	UMovieSceneSection* Section = FloatTrack->CreateNewSection();
	if (!Section)
	{
		return ErrorResult(TEXT("Failed to create track section"));
	}

	FFrameRate TickRes = MovieScene->GetTickResolution();
	TRange<FFrameNumber> PlaybackRange = MovieScene->GetPlaybackRange();
	Section->SetRange(PlaybackRange);
	FloatTrack->AddSection(*Section);

	FMovieSceneChannelProxy& Proxy = Section->GetChannelProxy();
	TArrayView<FMovieSceneFloatChannel*> FloatChannels = Proxy.GetChannels<FMovieSceneFloatChannel>();
	if (FloatChannels.Num() == 0)
	{
		return ErrorResult(TEXT("Section has no float channels"));
	}

	FMovieSceneFloatChannel* Channel = FloatChannels[0];
	int32 KeyCount = 0;

	for (const TSharedPtr<FJsonValue>& KeyVal : Keyframes)
	{
		if (!KeyVal.IsValid() || KeyVal->Type != EJson::Object) continue;
		TSharedPtr<FJsonObject> KeyObj = KeyVal->AsObject();

		double Time = 0.0;
		double Value = 0.0;
		KeyObj->TryGetNumberField(TEXT("time"), Time);
		KeyObj->TryGetNumberField(TEXT("value"), Value);

		FString InterpStr;
		KeyObj->TryGetStringField(TEXT("interp"), InterpStr);

		int32 FrameTick = FMath::RoundToInt32(Time * TickRes.AsDecimal());
		FFrameNumber FrameNum(FrameTick);

		EMovieSceneKeyInterpolation Interp = ParseKeyInterpolation(InterpStr);
		AddKeyToChannel(Channel, FrameNum, static_cast<float>(Value), Interp);
		KeyCount++;
	}

	WBP->MarkPackageDirty();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WBP);

	TSharedPtr<FJsonObject> Result = SuccessResult(
		FString::Printf(TEXT("Added %s track to '%s' on widget '%s' with %d keyframes"),
			*TrackType, *AnimationName, *WidgetName, KeyCount));
	Result->SetStringField(TEXT("track_type"), TrackType);
	Result->SetStringField(TEXT("property_path"), PropPath);
	Result->SetNumberField(TEXT("keyframe_count"), KeyCount);
	return Result;
}
