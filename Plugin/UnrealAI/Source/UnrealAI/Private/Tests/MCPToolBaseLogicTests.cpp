// Copyright Natali Caggiano. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "MCP/MCPToolBase.h"
#include "Dom/JsonObject.h"

#if WITH_DEV_AUTOMATION_TESTS

class FMCPToolBaseTestAccess : public FMCPToolBase
{
public:
	using FMCPToolBase::ResolveOperationAlias;
	using FMCPToolBase::ResolveParamAliases;
	using FMCPToolBase::UnknownOperationError;

	FMCPToolInfo GetInfo() const override { return FMCPToolInfo(); }
	FMCPToolResult Execute(const TSharedRef<FJsonObject>& Params) override { return FMCPToolResult::Error(TEXT("stub")); }
};

// ===== ResolveOperationAlias Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FToolBaseLogic_AliasResolves,
	"UnrealClaude.ToolBaseLogic.AliasResolves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FToolBaseLogic_AliasResolves::RunTest(const FString& Parameters)
{
	TMap<FString, FString> Aliases;
	Aliases.Add(TEXT("info"), TEXT("get_info"));
	FString Result = FMCPToolBaseTestAccess::ResolveOperationAlias(TEXT("info"), Aliases);
	TestEqual("Alias resolves to target", Result, TEXT("get_info"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FToolBaseLogic_UnknownOpPassThrough,
	"UnrealClaude.ToolBaseLogic.UnknownOpPassThrough",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FToolBaseLogic_UnknownOpPassThrough::RunTest(const FString& Parameters)
{
	TMap<FString, FString> Aliases;
	Aliases.Add(TEXT("info"), TEXT("get_info"));
	FString Result = FMCPToolBaseTestAccess::ResolveOperationAlias(TEXT("nonexistent"), Aliases);
	TestEqual("Unknown op passes through unchanged", Result, TEXT("nonexistent"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FToolBaseLogic_EmptyOpPassThrough,
	"UnrealClaude.ToolBaseLogic.EmptyOpPassThrough",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FToolBaseLogic_EmptyOpPassThrough::RunTest(const FString& Parameters)
{
	TMap<FString, FString> Aliases;
	Aliases.Add(TEXT("info"), TEXT("get_info"));
	FString Result = FMCPToolBaseTestAccess::ResolveOperationAlias(TEXT(""), Aliases);
	TestEqual("Empty op passes through", Result, TEXT(""));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FToolBaseLogic_AliasCaseSensitive,
	"UnrealClaude.ToolBaseLogic.AliasCaseSensitive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FToolBaseLogic_AliasCaseSensitive::RunTest(const FString& Parameters)
{
	TMap<FString, FString> Aliases;
	Aliases.Add(TEXT("info"), TEXT("get_info"));
	FString Result = FMCPToolBaseTestAccess::ResolveOperationAlias(TEXT("INFO"), Aliases);
	TestEqual("Case mismatch passes through", Result, TEXT("INFO"));
	return true;
}

// ===== ResolveParamAliases Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FToolBaseLogic_ParamAliasRenames,
	"UnrealClaude.ToolBaseLogic.ParamAliasRenames",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FToolBaseLogic_ParamAliasRenames::RunTest(const FString& Parameters)
{
	auto Params = MakeShared<FJsonObject>();
	Params->SetStringField(TEXT("asset_path"), TEXT("/Game/Test"));
	TMap<FString, FString> ParamAliases;
	ParamAliases.Add(TEXT("asset_path"), TEXT("blueprint_path"));
	FMCPToolBaseTestAccess::ResolveParamAliases(Params, ParamAliases);
	TestTrue("Target field exists", Params->HasField(TEXT("blueprint_path")));
	TestEqual("Target has correct value", Params->GetStringField(TEXT("blueprint_path")), TEXT("/Game/Test"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FToolBaseLogic_ParamAliasNoOverwrite,
	"UnrealClaude.ToolBaseLogic.ParamAliasNoOverwrite",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FToolBaseLogic_ParamAliasNoOverwrite::RunTest(const FString& Parameters)
{
	auto Params = MakeShared<FJsonObject>();
	Params->SetStringField(TEXT("asset_path"), TEXT("/Game/Old"));
	Params->SetStringField(TEXT("blueprint_path"), TEXT("/Game/Existing"));
	TMap<FString, FString> ParamAliases;
	ParamAliases.Add(TEXT("asset_path"), TEXT("blueprint_path"));
	FMCPToolBaseTestAccess::ResolveParamAliases(Params, ParamAliases);
	TestEqual("Existing field not overwritten", Params->GetStringField(TEXT("blueprint_path")), TEXT("/Game/Existing"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FToolBaseLogic_EmptyAliasMapNoOp,
	"UnrealClaude.ToolBaseLogic.EmptyAliasMapNoOp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FToolBaseLogic_EmptyAliasMapNoOp::RunTest(const FString& Parameters)
{
	auto Params = MakeShared<FJsonObject>();
	Params->SetStringField(TEXT("asset_path"), TEXT("/Game/Test"));
	TMap<FString, FString> EmptyAliases;
	FMCPToolBaseTestAccess::ResolveParamAliases(Params, EmptyAliases);
	TestTrue("Original field still exists", Params->HasField(TEXT("asset_path")));
	TestEqual("Original value unchanged", Params->GetStringField(TEXT("asset_path")), TEXT("/Game/Test"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FToolBaseLogic_MultipleParamAliases,
	"UnrealClaude.ToolBaseLogic.MultipleParamAliases",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FToolBaseLogic_MultipleParamAliases::RunTest(const FString& Parameters)
{
	auto Params = MakeShared<FJsonObject>();
	Params->SetStringField(TEXT("asset_path"), TEXT("/Game/BP"));
	Params->SetStringField(TEXT("asset_class"), TEXT("Blueprint"));
	TMap<FString, FString> ParamAliases;
	ParamAliases.Add(TEXT("asset_path"), TEXT("blueprint_path"));
	ParamAliases.Add(TEXT("asset_class"), TEXT("class_filter"));
	FMCPToolBaseTestAccess::ResolveParamAliases(Params, ParamAliases);
	TestTrue("First alias applied", Params->HasField(TEXT("blueprint_path")));
	TestTrue("Second alias applied", Params->HasField(TEXT("class_filter")));
	return true;
}

// ===== UnknownOperationError Tests =====

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FToolBaseLogic_ErrorIncludesValidOps,
	"UnrealClaude.ToolBaseLogic.ErrorIncludesValidOps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FToolBaseLogic_ErrorIncludesValidOps::RunTest(const FString& Parameters)
{
	TArray<FString> ValidOps = {TEXT("inspect"), TEXT("modify"), TEXT("delete")};
	FMCPToolResult Result = FMCPToolBaseTestAccess::UnknownOperationError(TEXT("bogus"), ValidOps);
	TestFalse("Result is error", Result.bSuccess);
	TestTrue("Contains 'inspect'", Result.Message.Contains(TEXT("inspect")));
	TestTrue("Contains 'modify'", Result.Message.Contains(TEXT("modify")));
	TestTrue("Contains 'delete'", Result.Message.Contains(TEXT("delete")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FToolBaseLogic_CloseTypoGetsSuggestion,
	"UnrealClaude.ToolBaseLogic.CloseTypoGetsSuggestion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FToolBaseLogic_CloseTypoGetsSuggestion::RunTest(const FString& Parameters)
{
	TArray<FString> ValidOps = {TEXT("inspect"), TEXT("modify")};
	FMCPToolResult Result = FMCPToolBaseTestAccess::UnknownOperationError(TEXT("inspct"), ValidOps);
	TestTrue("Suggests close match", Result.Message.Contains(TEXT("Did you mean")));
	TestTrue("Suggests 'inspect'", Result.Message.Contains(TEXT("inspect")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FToolBaseLogic_DistantTypoNoSuggestion,
	"UnrealClaude.ToolBaseLogic.DistantTypoNoSuggestion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FToolBaseLogic_DistantTypoNoSuggestion::RunTest(const FString& Parameters)
{
	TArray<FString> ValidOps = {TEXT("inspect"), TEXT("modify")};
	FMCPToolResult Result = FMCPToolBaseTestAccess::UnknownOperationError(TEXT("xyzzy"), ValidOps);
	TestFalse("No suggestion for distant typo", Result.Message.Contains(TEXT("Did you mean")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FToolBaseLogic_ExactMatchSuggestion,
	"UnrealClaude.ToolBaseLogic.ExactMatchSuggestion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FToolBaseLogic_ExactMatchSuggestion::RunTest(const FString& Parameters)
{
	TArray<FString> ValidOps = {TEXT("inspect"), TEXT("modify")};
	FMCPToolResult Result = FMCPToolBaseTestAccess::UnknownOperationError(TEXT("inspect"), ValidOps);
	TestTrue("Exact match still gets suggestion", Result.Message.Contains(TEXT("Did you mean")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FToolBaseLogic_EmptyValidOpsNoCrash,
	"UnrealClaude.ToolBaseLogic.EmptyValidOpsNoCrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter
)

bool FToolBaseLogic_EmptyValidOpsNoCrash::RunTest(const FString& Parameters)
{
	TArray<FString> EmptyOps;
	FMCPToolResult Result = FMCPToolBaseTestAccess::UnknownOperationError(TEXT("anything"), EmptyOps);
	TestFalse("Result is error", Result.bSuccess);
	TestFalse("No suggestion with empty ops", Result.Message.Contains(TEXT("Did you mean")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
