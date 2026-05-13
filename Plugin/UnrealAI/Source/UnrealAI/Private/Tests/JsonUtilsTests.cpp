// Copyright Natali Caggiano. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "JsonUtils.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

#if WITH_DEV_AUTOMATION_TESTS

// ===== Parse Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FJsonUtils_ParseValidJson,
	"UnrealClaude.JsonUtils.ParseValidJson",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FJsonUtils_ParseValidJson::RunTest(const FString& Parameters)
{
	auto Obj = FJsonUtils::Parse(TEXT("{\"name\":\"test\",\"count\":42}"));
	TestTrue("Parse simple object succeeds", Obj.IsValid());
	TestEqual("String field accessible", Obj->GetStringField(TEXT("name")), TEXT("test"));
	TestEqual("Number field accessible", (int32)Obj->GetNumberField(TEXT("count")), 42);

	auto Nested = FJsonUtils::Parse(TEXT("{\"outer\":{\"inner\":\"val\"}}"));
	TestTrue("Parse nested object succeeds", Nested.IsValid());
	TestTrue("Nested object accessible", Nested->GetObjectField(TEXT("outer")).IsValid());

	auto Arr = FJsonUtils::Parse(TEXT("{\"items\":[1,2,3]}"));
	TestTrue("Parse array succeeds", Arr.IsValid());
	TestEqual("Array has 3 elements", Arr->GetArrayField(TEXT("items")).Num(), 3);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FJsonUtils_ParseInvalidJson,
	"UnrealClaude.JsonUtils.ParseInvalidJson",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FJsonUtils_ParseInvalidJson::RunTest(const FString& Parameters)
{
	TestFalse("Empty string returns nullptr", FJsonUtils::Parse(TEXT("")).IsValid());
	TestFalse("Whitespace returns nullptr", FJsonUtils::Parse(TEXT("   ")).IsValid());
	TestFalse("Missing brace returns nullptr", FJsonUtils::Parse(TEXT("{\"a\":1")).IsValid());
	TestFalse("Malformed JSON returns nullptr", FJsonUtils::Parse(TEXT("{bad}")).IsValid());

	return true;
}

// ===== Stringify Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FJsonUtils_StringifyRoundTrip,
	"UnrealClaude.JsonUtils.StringifyRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FJsonUtils_StringifyRoundTrip::RunTest(const FString& Parameters)
{
	auto Original = MakeShared<FJsonObject>();
	Original->SetStringField(TEXT("key"), TEXT("value"));
	Original->SetNumberField(TEXT("num"), 3.14);

	FString Compact = FJsonUtils::Stringify(Original, false);
	auto Parsed = FJsonUtils::Parse(Compact);
	TestTrue("Compact round-trip parseable", Parsed.IsValid());
	TestEqual("String field survives round-trip", Parsed->GetStringField(TEXT("key")), TEXT("value"));

	FString Pretty = FJsonUtils::Stringify(Original, true);
	auto PrettyParsed = FJsonUtils::Parse(Pretty);
	TestTrue("Pretty-print round-trip parseable", PrettyParsed.IsValid());
	TestEqual("Number field survives round-trip", PrettyParsed->GetNumberField(TEXT("num")), 3.14);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FJsonUtils_StringifyNullEmpty,
	"UnrealClaude.JsonUtils.StringifyNullEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FJsonUtils_StringifyNullEmpty::RunTest(const FString& Parameters)
{
	TSharedPtr<FJsonObject> Null = nullptr;
	TestTrue("Stringify nullptr returns empty", FJsonUtils::Stringify(Null).IsEmpty());

	auto Empty = MakeShared<FJsonObject>();
	FString Result = FJsonUtils::Stringify(Empty);
	TestFalse("Stringify empty object returns non-empty", Result.IsEmpty());
	TestTrue("Stringify empty object is parseable", FJsonUtils::Parse(Result).IsValid());

	return true;
}

// ===== GetField Null Safety Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FJsonUtils_GetStringFieldNullSafety,
	"UnrealClaude.JsonUtils.GetStringFieldNullSafety",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FJsonUtils_GetStringFieldNullSafety::RunTest(const FString& Parameters)
{
	FString Val;
	TSharedPtr<FJsonObject> Null = nullptr;
	TestFalse("Nullptr returns false", FJsonUtils::GetStringField(Null, TEXT("x"), Val));

	auto Obj = MakeShared<FJsonObject>();
	TestFalse("Missing field returns false", FJsonUtils::GetStringField(Obj, TEXT("x"), Val));

	Obj->SetStringField(TEXT("x"), TEXT("hello"));
	TestTrue("Present field returns true", FJsonUtils::GetStringField(Obj, TEXT("x"), Val));
	TestEqual("Correct value returned", Val, TEXT("hello"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FJsonUtils_GetNumberFieldNullSafety,
	"UnrealClaude.JsonUtils.GetNumberFieldNullSafety",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FJsonUtils_GetNumberFieldNullSafety::RunTest(const FString& Parameters)
{
	double Val = 0.0;
	TSharedPtr<FJsonObject> Null = nullptr;
	TestFalse("Nullptr returns false", FJsonUtils::GetNumberField(Null, TEXT("x"), Val));

	auto Obj = MakeShared<FJsonObject>();
	TestFalse("Missing field returns false", FJsonUtils::GetNumberField(Obj, TEXT("x"), Val));

	Obj->SetNumberField(TEXT("x"), 99.5);
	TestTrue("Present field returns true", FJsonUtils::GetNumberField(Obj, TEXT("x"), Val));
	TestEqual("Correct value returned", Val, 99.5);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FJsonUtils_GetBoolFieldNullSafety,
	"UnrealClaude.JsonUtils.GetBoolFieldNullSafety",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FJsonUtils_GetBoolFieldNullSafety::RunTest(const FString& Parameters)
{
	bool Val = false;
	TSharedPtr<FJsonObject> Null = nullptr;
	TestFalse("Nullptr returns false", FJsonUtils::GetBoolField(Null, TEXT("x"), Val));

	auto Obj = MakeShared<FJsonObject>();
	TestFalse("Missing field returns false", FJsonUtils::GetBoolField(Obj, TEXT("x"), Val));

	Obj->SetBoolField(TEXT("x"), true);
	TestTrue("Present field returns true", FJsonUtils::GetBoolField(Obj, TEXT("x"), Val));
	TestTrue("Correct value returned", Val);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FJsonUtils_GetArrayFieldNullSafety,
	"UnrealClaude.JsonUtils.GetArrayFieldNullSafety",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FJsonUtils_GetArrayFieldNullSafety::RunTest(const FString& Parameters)
{
	TArray<TSharedPtr<FJsonValue>> Arr;
	TSharedPtr<FJsonObject> Null = nullptr;
	TestFalse("Nullptr returns false", FJsonUtils::GetArrayField(Null, TEXT("x"), Arr));

	auto Obj = MakeShared<FJsonObject>();
	TestFalse("Missing field returns false", FJsonUtils::GetArrayField(Obj, TEXT("x"), Arr));

	TArray<TSharedPtr<FJsonValue>> Items;
	Items.Add(MakeShared<FJsonValueNumber>(1.0));
	Items.Add(MakeShared<FJsonValueNumber>(2.0));
	Obj->SetArrayField(TEXT("x"), Items);
	TestTrue("Present field returns true", FJsonUtils::GetArrayField(Obj, TEXT("x"), Arr));
	TestEqual("Correct array size", Arr.Num(), 2);

	return true;
}

// ===== Array Conversion Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FJsonUtils_StringArrayRoundTrip,
	"UnrealClaude.JsonUtils.StringArrayRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FJsonUtils_StringArrayRoundTrip::RunTest(const FString& Parameters)
{
	TArray<FString> Input = {TEXT("alpha"), TEXT("beta"), TEXT("gamma")};
	auto JsonArr = FJsonUtils::StringArrayToJson(Input);
	auto Output = FJsonUtils::JsonArrayToStrings(JsonArr);
	TestEqual("Array size matches", Output.Num(), Input.Num());
	TestEqual("First element matches", Output[0], TEXT("alpha"));
	TestEqual("Last element matches", Output[2], TEXT("gamma"));

	TArray<FString> EmptyInput;
	auto EmptyJson = FJsonUtils::StringArrayToJson(EmptyInput);
	auto EmptyOutput = FJsonUtils::JsonArrayToStrings(EmptyJson);
	TestEqual("Empty array round-trips", EmptyOutput.Num(), 0);

	return true;
}

// ===== Geometry Conversion Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FJsonUtils_VectorRoundTrip,
	"UnrealClaude.JsonUtils.VectorRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FJsonUtils_VectorRoundTrip::RunTest(const FString& Parameters)
{
	FVector Input(100.5, -200.3, 50.0);
	auto Json = FJsonUtils::VectorToJson(Input);
	FVector Output;
	TestTrue("JsonToVector succeeds", FJsonUtils::JsonToVector(Json, Output));
	TestEqual("X matches", Output.X, Input.X);
	TestEqual("Y matches", Output.Y, Input.Y);
	TestEqual("Z matches", Output.Z, Input.Z);

	FVector Zero = FVector::ZeroVector;
	auto ZeroJson = FJsonUtils::VectorToJson(Zero);
	FVector ZeroOut;
	FJsonUtils::JsonToVector(ZeroJson, ZeroOut);
	TestEqual("Zero vector X", ZeroOut.X, 0.0);

	TSharedPtr<FJsonObject> NullObj = nullptr;
	FVector NullOut;
	TestFalse("Nullptr returns false", FJsonUtils::JsonToVector(NullObj, NullOut));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FJsonUtils_RotatorRoundTrip,
	"UnrealClaude.JsonUtils.RotatorRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FJsonUtils_RotatorRoundTrip::RunTest(const FString& Parameters)
{
	FRotator Input(45.0, -90.0, 180.0);
	auto Json = FJsonUtils::RotatorToJson(Input);
	FRotator Output;
	TestTrue("JsonToRotator succeeds", FJsonUtils::JsonToRotator(Json, Output));
	TestEqual("Pitch matches", Output.Pitch, Input.Pitch);
	TestEqual("Yaw matches", Output.Yaw, Input.Yaw);
	TestEqual("Roll matches", Output.Roll, Input.Roll);

	TSharedPtr<FJsonObject> NullObj = nullptr;
	FRotator NullOut;
	TestFalse("Nullptr returns false", FJsonUtils::JsonToRotator(NullObj, NullOut));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FJsonUtils_ScaleRoundTrip,
	"UnrealClaude.JsonUtils.ScaleRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FJsonUtils_ScaleRoundTrip::RunTest(const FString& Parameters)
{
	auto EmptyObj = MakeShared<FJsonObject>();
	FVector DefaultScale;
	FJsonUtils::JsonToScale(EmptyObj, DefaultScale);
	TestEqual("Default scale X is 1", DefaultScale.X, 1.0);
	TestEqual("Default scale Y is 1", DefaultScale.Y, 1.0);
	TestEqual("Default scale Z is 1", DefaultScale.Z, 1.0);

	FVector Input(2.0, 0.5, 3.0);
	auto Json = FJsonUtils::ScaleToJson(Input);
	FVector Output;
	FJsonUtils::JsonToScale(Json, Output);
	TestEqual("Scale X matches", Output.X, Input.X);
	TestEqual("Scale Y matches", Output.Y, Input.Y);
	TestEqual("Scale Z matches", Output.Z, Input.Z);

	return true;
}

// ===== Response Helpers =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FJsonUtils_CreateSuccessResponse,
	"UnrealClaude.JsonUtils.CreateSuccessResponse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FJsonUtils_CreateSuccessResponse::RunTest(const FString& Parameters)
{
	auto Resp = FJsonUtils::CreateSuccessResponse(TEXT("it worked"));
	bool bSuccess = false;
	FJsonUtils::GetBoolField(Resp, TEXT("success"), bSuccess);
	TestTrue("success is true", bSuccess);
	FString Msg;
	FJsonUtils::GetStringField(Resp, TEXT("message"), Msg);
	TestEqual("message matches", Msg, TEXT("it worked"));

	auto Data = MakeShared<FJsonObject>();
	Data->SetNumberField(TEXT("id"), 5);
	auto WithData = FJsonUtils::CreateSuccessResponse(TEXT("ok"), Data);
	TestTrue("data object present", WithData->HasField(TEXT("data")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FJsonUtils_CreateErrorResponse,
	"UnrealClaude.JsonUtils.CreateErrorResponse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FJsonUtils_CreateErrorResponse::RunTest(const FString& Parameters)
{
	auto Resp = FJsonUtils::CreateErrorResponse(TEXT("something broke"));
	bool bSuccess = true;
	FJsonUtils::GetBoolField(Resp, TEXT("success"), bSuccess);
	TestFalse("success is false", bSuccess);
	FString Err;
	FJsonUtils::GetStringField(Resp, TEXT("error"), Err);
	TestEqual("error matches", Err, TEXT("something broke"));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
