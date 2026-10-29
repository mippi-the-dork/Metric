#pragma once

#include "CoreMinimal.h"
#include "EditorUndoClient.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "UObject/GCObject.h"

class IDetailsView;
class UMetricPreset;
class SVerticalBox;
class SSearchBox;

struct FMetricProfileEditorState
{
    FName SelectedProperty = NAME_None;
};

class FMetricProfileEditorToolkit final : public FAssetEditorToolkit, public FGCObject, public FEditorUndoClient
{
public:
    FMetricProfileEditorToolkit();
    virtual ~FMetricProfileEditorToolkit() override;

    void Initialize(UMetricPreset* InPreset, EToolkitMode::Type InMode, const TSharedPtr<IToolkitHost>& InToolkitHost);

    /** Open/focus a Metric Profile editor and select the requested profile section. */
    static void OpenAndFocusProfileSection(UMetricPreset* InPreset, FName PropertyName);

    virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
    virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;

    virtual FName GetToolkitFName() const override;
    virtual FText GetBaseToolkitName() const override;
    virtual FString GetWorldCentricTabPrefix() const override;
    virtual FLinearColor GetWorldCentricTabColorScale() const override;

    virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
    virtual FString GetReferencerName() const override;
    virtual void PostUndo(bool bSuccess) override;
    virtual void PostRedo(bool bSuccess) override;

private:
    struct FValidationIssue
    {
        FName PropertyName;
        FText Message;
        bool bError = false;
    };

    struct FSection
    {
        FName PropertyName;
        FText Label;
        FText Group;
        FText Description;
        FText Fidelity;
        bool bToggleable = true;
    };

    struct FComparisonRow
    {
        TArray<FName> PropertyPath;
        FText Label;
        FText CurrentValue;
        FText ComparisonValue;
    };

    TSharedRef<SDockTab> SpawnProfileTab(const FSpawnTabArgs& Args);
    TSharedRef<SWidget> BuildProfileEditor();
    TSharedRef<SWidget> BuildSidebar();
    TSharedRef<SWidget> BuildSectionHeader();
    TSharedRef<SWidget> BuildSelectedSectionComparisonPanel();
    TSharedRef<SWidget> BuildSidebarSectionButton(const FSection& Section);
    void RebuildSectionHeader();
    void SelectSection(FName PropertyName);
    const FSection* FindSection(FName PropertyName) const;
    bool IsSectionSelected(FName PropertyName) const;
    bool IsSectionEnabled(FName PropertyName) const;
    void SetSectionEnabled(FName PropertyName, bool bEnabled);
    bool SetSectionEnabledValue(FName PropertyName, bool bEnabled);
    void SetGroupEnabled(const FText& Group, bool bEnabled);
    bool GroupHasToggleableSections(const FText& Group) const;
    bool GroupMatchesSearch(const FText& Group) const;
    bool SectionMatchesSearch(const FSection& Section) const;
    bool CanNavigateComparisonChanges() const;
    void NavigateComparisonChange(int32 Direction);
    void GatherValidationIssues(TArray<FValidationIssue>& OutIssues) const;
    FText GetValidationSummary() const;
    FSlateColor GetValidationSummaryColor() const;
    FText GetSelectedSectionValidationText() const;
    FText GetSectionValidationMarker(FName PropertyName) const;
    FSlateColor GetSectionValidationMarkerColor(FName PropertyName) const;
    EVisibility GetSelectedSectionValidationVisibility() const;
    int32 CountEnabledAnalyses() const;
    int32 CountToggleableAnalyses() const;
    FText GetProfileTitle() const;
    FText GetProfileSummary() const;
    FText GetComparisonSummary() const;
    bool IsSectionDifferentFromComparison(FName PropertyName) const;
    int32 CountComparisonDifferences() const;
    int32 CountComparisonPropertyDifferences() const;
    int32 CountSectionPropertyDifferences(FName PropertyName) const;
    void GatherSectionComparisonRows(FName PropertyName, TArray<FComparisonRow>& OutRows) const;
    void GatherStructComparisonRows(const UStruct* StructType, const void* CurrentContainer, const void* ComparisonContainer, TArray<FName>& PropertyPath, const FString& LabelPrefix, TArray<FComparisonRow>& OutRows) const;
    FText ExportPropertyValue(const FProperty* Property, const void* Container) const;
    FText GetSectionComparisonMarker(FName PropertyName) const;
    FSlateColor GetSectionComparisonMarkerColor(FName PropertyName) const;
    bool CanResetSelectedSectionToDefaults() const;
    FReply ResetSelectedSectionToDefaults();
    bool CanCopySelectedSectionFromComparison() const;
    FReply CopySelectedSectionFromComparison();
    FReply CopyComparisonPropertyValue(TArray<FName> PropertyPath);
    void SetComparisonPreset(UMetricPreset* InPreset);

    TObjectPtr<UMetricPreset> Preset = nullptr;
    TObjectPtr<UMetricPreset> ComparisonPreset = nullptr;
    TSharedPtr<IDetailsView> DetailsView;
    TSharedPtr<FMetricProfileEditorState> EditorState;
    TSharedPtr<SVerticalBox> SectionHeaderBox;
    FText SearchText;
    bool bShowComparisonChangesOnly = false;
    TArray<FSection> Sections;
};
