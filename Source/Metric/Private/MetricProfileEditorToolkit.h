#pragma once

#include "CoreMinimal.h"
#include "EditorUndoClient.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "UObject/GCObject.h"

class IDetailsView;
class UMetricPreset;
class SVerticalBox;

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
    struct FSection
    {
        FName PropertyName;
        FText Label;
        FText Group;
        FText Description;
        FText Fidelity;
        bool bToggleable = true;
    };

    TSharedRef<SDockTab> SpawnProfileTab(const FSpawnTabArgs& Args);
    TSharedRef<SWidget> BuildProfileEditor();
    TSharedRef<SWidget> BuildSidebar();
    TSharedRef<SWidget> BuildSectionHeader();
    TSharedRef<SWidget> BuildSidebarSectionButton(const FSection& Section);
    void RebuildSectionHeader();
    void SelectSection(FName PropertyName);
    const FSection* FindSection(FName PropertyName) const;
    bool IsSectionSelected(FName PropertyName) const;
    bool IsSectionEnabled(FName PropertyName) const;
    void SetSectionEnabled(FName PropertyName, bool bEnabled);
    int32 CountEnabledAnalyses() const;
    int32 CountToggleableAnalyses() const;
    FText GetProfileTitle() const;
    FText GetProfileSummary() const;

    TObjectPtr<UMetricPreset> Preset = nullptr;
    TSharedPtr<IDetailsView> DetailsView;
    TSharedPtr<FMetricProfileEditorState> EditorState;
    TSharedPtr<SVerticalBox> SectionHeaderBox;
    TArray<FSection> Sections;
};
