#include "MetricProfileEditorToolkit.h"

#include "MetricPreset.h"
#include "MetricProfileDetailsCustomization.h"
#include "Editor.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "PropertyCustomizationHelpers.h"
#include "AssetRegistry/AssetData.h"
#include "UObject/UnrealType.h"
#include "ScopedTransaction.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Styling/AppStyle.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "MetricProfileEditorToolkit"

namespace MetricProfileEditor
{
    static const FName AppIdentifier(TEXT("MetricProfileEditorApp"));
    static const FName ProfileTabId(TEXT("MetricProfileEditor.Profile"));
    static TMap<TWeakObjectPtr<UMetricPreset>, FMetricProfileEditorToolkit*> OpenEditors;
    static TMap<TWeakObjectPtr<UMetricPreset>, FName> PendingSections;
}

FMetricProfileEditorToolkit::FMetricProfileEditorToolkit()
{
    EditorState = MakeShared<FMetricProfileEditorState>();

    const FText Core = LOCTEXT("GroupCore", "CORE");
    const FText Space = LOCTEXT("GroupSpace", "SPACE & FIT");
    const FText Traversal = LOCTEXT("GroupTraversal", "MOVEMENT & TRAVERSAL");
    const FText Camera = LOCTEXT("GroupCamera", "CAMERA & INTERACTION");
    const FText Combat = LOCTEXT("GroupCombat", "COMBAT & EXPOSURE");
    const FText VisualIdentity = LOCTEXT("GroupVisualIdentity", "VISUALIZATION COLORS");
    const FText Visual = LOCTEXT("GroupVisual", "REPRESENTATION");

    Sections = {
        { GET_MEMBER_NAME_CHECKED(UMetricPreset, VisualTheme), LOCTEXT("VisualTheme", "Visualization Colors"), VisualIdentity, LOCTEXT("VisualThemeDesc", "Defines every semantic color used by this Metric Profile. World visualization and HUD/result colors are paired: changing a state or aspect here changes both surfaces together. Each analysis exposes only the colors that are meaningful to its visualization and states."), LOCTEXT("VisualIdentityFidelity", "Visual identity"), false },
        { GET_MEMBER_NAME_CHECKED(UMetricPreset, Hull), LOCTEXT("Hull", "Hull"), Core, LOCTEXT("HullDesc", "Defines the gameplay entity's reference shape and dimensions used by Metric analyses."), LOCTEXT("ReferenceFidelity", "Reference"), false },
        { GET_MEMBER_NAME_CHECKED(UMetricPreset, Sensors), LOCTEXT("Sensors", "Sensor Heights"), Core, LOCTEXT("SensorsDesc", "Reference points for primary and low-profile sensing or eye-height comparisons."), LOCTEXT("ReferenceFidelity2", "Reference"), true },
        { GET_MEMBER_NAME_CHECKED(UMetricPreset, Clearance), LOCTEXT("Clearance", "Clearance"), Space, LOCTEXT("ClearanceDesc", "Tests forward usable space with the active Metric hull."), LOCTEXT("MeasuredFidelity", "Measured"), true },
        { GET_MEMBER_NAME_CHECKED(UMetricPreset, OverheadClearance), LOCTEXT("Overhead", "Overhead Clearance"), Space, LOCTEXT("OverheadDesc", "Measures vertical headroom above the current placement."), LOCTEXT("MeasuredFidelity2", "Measured"), true },
        { GET_MEMBER_NAME_CHECKED(UMetricPreset, GroundValidation), LOCTEXT("Ground", "Ground Validation"), Space, LOCTEXT("GroundDesc", "Samples floor continuity, slope, and step changes across the space ahead."), LOCTEXT("MeasuredFidelity3", "Measured"), true },
        { GET_MEMBER_NAME_CHECKED(UMetricPreset, FitAnalysis), LOCTEXT("Fit", "Fit Analysis"), Space, LOCTEXT("FitDesc", "Estimates whether the profile can continuously fit through the sampled corridor."), LOCTEXT("EstimatedFidelity", "Estimated"), true },
        { GET_MEMBER_NAME_CHECKED(UMetricPreset, ThresholdAnalysis), LOCTEXT("Threshold", "Threshold Analysis"), Space, LOCTEXT("ThresholdDesc", "Evaluates a short doorway or transition for width, height, floor, and step limits."), LOCTEXT("EstimatedFidelity2", "Estimated"), true },
        { GET_MEMBER_NAME_CHECKED(UMetricPreset, TurnClearance), LOCTEXT("Turn", "Turn Clearance"), Space, LOCTEXT("TurnDesc", "Sweeps the profile around a configured turning arc to estimate turning room."), LOCTEXT("ModeledFidelity", "Modeled"), true },
        { GET_MEMBER_NAME_CHECKED(UMetricPreset, LedgeAnalysis), LOCTEXT("Ledge", "Ledge Analysis"), Space, LOCTEXT("LedgeDesc", "Finds nearby ledges and evaluates the drop against the profile's safe-drop threshold."), LOCTEXT("MeasuredFidelity4", "Measured"), true },
        { GET_MEMBER_NAME_CHECKED(UMetricPreset, LocomotionBands), LOCTEXT("Locomotion", "Locomotion Bands"), Traversal, LOCTEXT("LocomotionDesc", "Shows distance bands derived from configured movement speed and time."), LOCTEXT("ModeledFidelity2", "Modeled"), true },
        { GET_MEMBER_NAME_CHECKED(UMetricPreset, StoppingDistance), LOCTEXT("Stopping", "Stopping Distance"), Traversal, LOCTEXT("StoppingDesc", "Estimates required stopping runway from speed, deceleration, and reaction time."), LOCTEXT("ModeledFidelity3", "Modeled"), true },
        { GET_MEMBER_NAME_CHECKED(UMetricPreset, Trajectory), LOCTEXT("Trajectory", "Trajectory"), Traversal, LOCTEXT("TrajectoryDesc", "Models a ballistic arc, swept envelope, landing validity, and apex headroom."), LOCTEXT("ModeledFidelity4", "Modeled"), true },
        { GET_MEMBER_NAME_CHECKED(UMetricPreset, JumpRunway), LOCTEXT("JumpRunway", "Jump Runway"), Traversal, LOCTEXT("JumpRunwayDesc", "Checks continuous supported runway before takeoff and after landing."), LOCTEXT("MeasuredFidelity5", "Measured"), true },
        { GET_MEMBER_NAME_CHECKED(UMetricPreset, AlternateStance), LOCTEXT("Alternate", "Alternate Stance Fit"), Traversal, LOCTEXT("AlternateDesc", "Compares standing fit against a second configured stance height."), LOCTEXT("MeasuredFidelity6", "Measured"), true },
        { GET_MEMBER_NAME_CHECKED(UMetricPreset, MantleVault), LOCTEXT("Mantle", "Mantle / Vault Envelope"), Traversal, LOCTEXT("MantleDesc", "Estimates whether a nearby obstacle sits inside the configured mantle or vault envelope."), LOCTEXT("EstimatedFidelity3", "Estimated"), true },
        { GET_MEMBER_NAME_CHECKED(UMetricPreset, RecoverySpace), LOCTEXT("Recovery", "Recovery / Dodge Space"), Traversal, LOCTEXT("RecoveryDesc", "Measures clear movement space forward, backward, left, and right using the active hull."), LOCTEXT("MeasuredFidelity7", "Measured"), true },
        { GET_MEMBER_NAME_CHECKED(UMetricPreset, CameraProbe), LOCTEXT("CameraProbe", "Camera Probe"), Camera, LOCTEXT("CameraProbeDesc", "Models a third-person camera boom and reports compression against scene collision."), LOCTEXT("ModeledFidelity5", "Modeled"), true },
        { GET_MEMBER_NAME_CHECKED(UMetricPreset, CameraSightline), LOCTEXT("Sightline", "Camera Sightline"), Camera, LOCTEXT("SightlineDesc", "Tests camera-to-subject visibility and configured framing tolerance."), LOCTEXT("MeasuredModeledFidelity", "Measured / Modeled"), true },
        { GET_MEMBER_NAME_CHECKED(UMetricPreset, ShoulderClearance), LOCTEXT("Shoulder", "Shoulder Camera Clearance"), Camera, LOCTEXT("ShoulderDesc", "Compares left and right shoulder-camera clearance from a configured pivot."), LOCTEXT("MeasuredFidelity8", "Measured"), true },
        { GET_MEMBER_NAME_CHECKED(UMetricPreset, InteractionReach), LOCTEXT("Interaction", "Interaction Reach"), Camera, LOCTEXT("InteractionDesc", "Tests whether scene geometry is reachable from a configured interaction origin."), LOCTEXT("MeasuredFidelity9", "Measured"), true },
        { GET_MEMBER_NAME_CHECKED(UMetricPreset, SensorField), LOCTEXT("SensorField", "Sensor Field"), Combat, LOCTEXT("SensorFieldDesc", "Samples a configured field of view and reports blocked versus clear sensing directions."), LOCTEXT("EstimatedFidelity4", "Estimated"), true },
        { GET_MEMBER_NAME_CHECKED(UMetricPreset, EngagementRanges), LOCTEXT("Ranges", "Engagement Ranges"), Combat, LOCTEXT("RangesDesc", "Displays authored short, medium, and long gameplay range references."), LOCTEXT("ReferenceFidelity3", "Reference"), true },
        { GET_MEMBER_NAME_CHECKED(UMetricPreset, CoverExposure), LOCTEXT("Cover", "Cover / Exposure"), Combat, LOCTEXT("CoverDesc", "Samples low, torso, and head visibility against a threat direction."), LOCTEXT("MeasuredFidelity10", "Measured"), true },
        { GET_MEMBER_NAME_CHECKED(UMetricPreset, CombatDensity), LOCTEXT("Density", "Combat Spacing Density"), Combat, LOCTEXT("DensityDesc", "Estimates how many profile-sized personal-space cells fit inside the configured combat area."), LOCTEXT("EstimatedFidelity5", "Estimated"), true },
        { GET_MEMBER_NAME_CHECKED(UMetricPreset, Crossfire), LOCTEXT("Crossfire", "Crossfire Exposure"), Combat, LOCTEXT("CrossfireDesc", "Samples exposure from multiple threat directions around the profile."), LOCTEXT("EstimatedFidelity6", "Estimated"), true },
        { GET_MEMBER_NAME_CHECKED(UMetricPreset, Representation), LOCTEXT("Representation", "Representation"), Visual, LOCTEXT("RepresentationDesc", "Optional visual-only Static Mesh representation layered onto the Metric profile."), LOCTEXT("VisualOnlyFidelity", "Visual only"), true },
    };

    EditorState->SelectedProperty = GET_MEMBER_NAME_CHECKED(UMetricPreset, Hull);
}

FMetricProfileEditorToolkit::~FMetricProfileEditorToolkit()
{
    if (Preset)
    {
        MetricProfileEditor::OpenEditors.Remove(Preset.Get());
        MetricProfileEditor::PendingSections.Remove(Preset.Get());
    }

    if (GEditor)
    {
        GEditor->UnregisterForUndo(this);
    }
}

void FMetricProfileEditorToolkit::Initialize(UMetricPreset* InPreset, EToolkitMode::Type InMode, const TSharedPtr<IToolkitHost>& InToolkitHost)
{
    Preset = InPreset;
    check(Preset);
    Preset->SetFlags(RF_Transactional);

    if (const FName* PendingSection = MetricProfileEditor::PendingSections.Find(Preset.Get()))
    {
        if (FindSection(*PendingSection))
        {
            EditorState->SelectedProperty = *PendingSection;
        }
    }

    if (GEditor)
    {
        GEditor->RegisterForUndo(this);
    }

    const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout(TEXT("Standalone_MetricProfileEditor_v2"))
        ->AddArea(
            FTabManager::NewPrimaryArea()
            ->SetOrientation(Orient_Vertical)
            ->Split(
                FTabManager::NewStack()
                ->AddTab(MetricProfileEditor::ProfileTabId, ETabState::OpenedTab)
                ->SetHideTabWell(true)));

    InitAssetEditor(
        InMode,
        InToolkitHost,
        MetricProfileEditor::AppIdentifier,
        Layout,
        true,
        true,
        Preset);

    MetricProfileEditor::OpenEditors.Add(Preset.Get(), this);
    MetricProfileEditor::PendingSections.Remove(Preset.Get());
    RegenerateMenusAndToolbars();
}

void FMetricProfileEditorToolkit::OpenAndFocusProfileSection(UMetricPreset* InPreset, FName PropertyName)
{
    if (!InPreset || !GEditor)
    {
        return;
    }

    if (FMetricProfileEditorToolkit** ExistingEditor = MetricProfileEditor::OpenEditors.Find(InPreset))
    {
        if (*ExistingEditor)
        {
            (*ExistingEditor)->SelectSection(PropertyName);
        }
    }
    else
    {
        MetricProfileEditor::PendingSections.Add(InPreset, PropertyName);
    }

    if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
    {
        AssetEditorSubsystem->OpenEditorForAsset(InPreset);
    }

    // Asset opening is synchronous for the Metric asset definition. Apply once more
    // in case the editor was created by the call above.
    if (FMetricProfileEditorToolkit** OpenedEditor = MetricProfileEditor::OpenEditors.Find(InPreset))
    {
        if (*OpenedEditor)
        {
            (*OpenedEditor)->SelectSection(PropertyName);
        }
    }
}

void FMetricProfileEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu", "Metric Profile Editor"));
    FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

    InTabManager->RegisterTabSpawner(
        MetricProfileEditor::ProfileTabId,
        FOnSpawnTab::CreateSP(this, &FMetricProfileEditorToolkit::SpawnProfileTab))
        .SetDisplayName(LOCTEXT("ProfileTab", "Profile"))
        .SetGroup(WorkspaceMenuCategory.ToSharedRef());
}

void FMetricProfileEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    InTabManager->UnregisterTabSpawner(MetricProfileEditor::ProfileTabId);
    FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
}

FName FMetricProfileEditorToolkit::GetToolkitFName() const
{
    return FName(TEXT("MetricProfileEditor"));
}

FText FMetricProfileEditorToolkit::GetBaseToolkitName() const
{
    return LOCTEXT("ToolkitName", "Metric Profile Editor");
}

FString FMetricProfileEditorToolkit::GetWorldCentricTabPrefix() const
{
    return TEXT("Metric Profile ");
}

FLinearColor FMetricProfileEditorToolkit::GetWorldCentricTabColorScale() const
{
    return FLinearColor(1.0f, 0.58f, 0.05f, 0.5f);
}

void FMetricProfileEditorToolkit::AddReferencedObjects(FReferenceCollector& Collector)
{
    Collector.AddReferencedObject(Preset);
    Collector.AddReferencedObject(ComparisonPreset);
}

FString FMetricProfileEditorToolkit::GetReferencerName() const
{
    return TEXT("FMetricProfileEditorToolkit");
}

void FMetricProfileEditorToolkit::PostUndo(bool bSuccess)
{
    if (bSuccess && DetailsView.IsValid())
    {
        DetailsView->ForceRefresh();
        RebuildSectionHeader();
    }
}

void FMetricProfileEditorToolkit::PostRedo(bool bSuccess)
{
    PostUndo(bSuccess);
}

TSharedRef<SDockTab> FMetricProfileEditorToolkit::SpawnProfileTab(const FSpawnTabArgs& Args)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::PanelTab)
        [
            BuildProfileEditor()
        ];
}

TSharedRef<SWidget> FMetricProfileEditorToolkit::BuildProfileEditor()
{
    FDetailsViewArgs DetailsArgs;
    DetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
    DetailsArgs.bUpdatesFromSelection = false;
    DetailsArgs.bLockable = false;
    DetailsArgs.bAllowSearch = false;
    DetailsArgs.bHideSelectionTip = true;
    DetailsArgs.bShowPropertyMatrixButton = false;
    DetailsArgs.bShowOptions = false;
    DetailsArgs.bAllowFavoriteSystem = false;
    DetailsArgs.bShowScrollBar = true;
    DetailsArgs.ColumnWidth = 0.46f;

    FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
    DetailsView = PropertyEditor.CreateDetailView(DetailsArgs);
    DetailsView->RegisterInstancedCustomPropertyLayout(
        UMetricPreset::StaticClass(),
        FOnGetDetailCustomizationInstance::CreateStatic(&FMetricProfileDetailsCustomization::MakeInstance, EditorState.ToSharedRef()));
    DetailsView->OnFinishedChangingProperties().AddLambda([this](const FPropertyChangedEvent&)
    {
        RebuildSectionHeader();
    });
    DetailsView->SetObject(Preset, true);

    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
        .Padding(0.0f)
        [
            SNew(SSplitter)
            + SSplitter::Slot()
            .Value(0.28f)
            .MinSize(260.0f)
            [
                BuildSidebar()
            ]
            + SSplitter::Slot()
            .Value(0.72f)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SAssignNew(SectionHeaderBox, SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        BuildSectionHeader()
                    ]
                ]
                + SVerticalBox::Slot()
                .FillHeight(1.0f)
                .Padding(14.0f, 4.0f, 14.0f, 14.0f)
                [
                    DetailsView.ToSharedRef()
                ]
            ]
        ];
}

TSharedRef<SWidget> FMetricProfileEditorToolkit::BuildSidebar()
{
    TSharedRef<SVerticalBox> Content = SNew(SVerticalBox);

    Content->AddSlot()
        .AutoHeight()
        .Padding(14.0f, 14.0f, 14.0f, 6.0f)
        [
            SNew(STextBlock)
            .Text(this, &FMetricProfileEditorToolkit::GetProfileTitle)
            .Font(FAppStyle::GetFontStyle(TEXT("NormalFontBold")))
        ];

    Content->AddSlot()
        .AutoHeight()
        .Padding(14.0f, 0.0f, 14.0f, 12.0f)
        [
            SNew(STextBlock)
            .Text(this, &FMetricProfileEditorToolkit::GetProfileSummary)
            .Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
            .ColorAndOpacity(FSlateColor::UseSubduedForeground())
        ];

    Content->AddSlot()
        .AutoHeight()
        .Padding(14.0f, 0.0f, 14.0f, 8.0f)
        [
            SNew(STextBlock)
            .Text(this, &FMetricProfileEditorToolkit::GetValidationSummary)
            .Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
            .ColorAndOpacity(this, &FMetricProfileEditorToolkit::GetValidationSummaryColor)
        ];

    Content->AddSlot()
        .AutoHeight()
        .Padding(14.0f, 0.0f, 14.0f, 4.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("CompareProfileLabel", "COMPARE PROFILE"))
            .Font(FAppStyle::GetFontStyle(TEXT("SmallFontBold")))
            .ColorAndOpacity(FSlateColor::UseSubduedForeground())
        ];

    Content->AddSlot()
        .AutoHeight()
        .Padding(10.0f, 0.0f, 10.0f, 3.0f)
        [
            SNew(SObjectPropertyEntryBox)
            .AllowedClass(UMetricPreset::StaticClass())
            .ObjectPath_Lambda([this]()
            {
                return ComparisonPreset ? ComparisonPreset->GetPathName() : FString();
            })
            .OnObjectChanged_Lambda([this](const FAssetData& AssetData)
            {
                UMetricPreset* NewComparison = Cast<UMetricPreset>(AssetData.GetAsset());
                SetComparisonPreset(NewComparison);
            })
        ];

    Content->AddSlot()
        .AutoHeight()
        .Padding(14.0f, 0.0f, 14.0f, 5.0f)
        [
            SNew(STextBlock)
            .Text(this, &FMetricProfileEditorToolkit::GetComparisonSummary)
            .Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
            .ColorAndOpacity(FSlateColor::UseSubduedForeground())
        ];

    Content->AddSlot()
        .AutoHeight()
        .Padding(10.0f, 0.0f, 10.0f, 8.0f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            [
                SNew(SCheckBox)
                .IsEnabled_Lambda([this]() { return ComparisonPreset != nullptr; })
                .IsChecked_Lambda([this]() { return bShowComparisonChangesOnly ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
                .OnCheckStateChanged_Lambda([this](ECheckBoxState State)
                {
                    bShowComparisonChangesOnly = State == ECheckBoxState::Checked;
                    if (bShowComparisonChangesOnly && EditorState.IsValid() && !IsSectionDifferentFromComparison(EditorState->SelectedProperty))
                    {
                        NavigateComparisonChange(1);
                    }
                })
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("ShowChangedOnly", "Show Changed Only"))
                    .Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
                ]
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SButton)
                .ButtonStyle(FAppStyle::Get(), TEXT("SimpleButton"))
                .IsEnabled(this, &FMetricProfileEditorToolkit::CanNavigateComparisonChanges)
                .ToolTipText(LOCTEXT("PreviousChangeTip", "Select the previous profile section with changed values."))
                .Text(LOCTEXT("PreviousChange", "Previous"))
                .OnClicked_Lambda([this]()
                {
                    NavigateComparisonChange(-1);
                    return FReply::Handled();
                })
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(2.0f, 0.0f, 0.0f, 0.0f)
            [
                SNew(SButton)
                .ButtonStyle(FAppStyle::Get(), TEXT("SimpleButton"))
                .IsEnabled(this, &FMetricProfileEditorToolkit::CanNavigateComparisonChanges)
                .ToolTipText(LOCTEXT("NextChangeTip", "Select the next profile section with changed values."))
                .Text(LOCTEXT("NextChange", "Next"))
                .OnClicked_Lambda([this]()
                {
                    NavigateComparisonChange(1);
                    return FReply::Handled();
                })
            ]
        ];

    Content->AddSlot()
        .AutoHeight()
        .Padding(10.0f, 0.0f, 10.0f, 8.0f)
        [
            SNew(SSearchBox)
            .HintText(LOCTEXT("SearchProfileFeatures", "Search profile features..."))
            .OnTextChanged_Lambda([this](const FText& NewText)
            {
                SearchText = NewText;
            })
        ];

    Content->AddSlot().AutoHeight().Padding(10.0f, 0.0f, 10.0f, 8.0f)[SNew(SSeparator)];

    FText PreviousGroup;
    for (const FSection& Section : Sections)
    {
        if (!Section.Group.EqualTo(PreviousGroup))
        {
            PreviousGroup = Section.Group;
            const FText GroupText = Section.Group;
            Content->AddSlot()
                .AutoHeight()
                .Padding(14.0f, 10.0f, 14.0f, 3.0f)
                [
                    SNew(SHorizontalBox)
                    .Visibility_Lambda([this, GroupText]()
                    {
                        return GroupMatchesSearch(GroupText) ? EVisibility::Visible : EVisibility::Collapsed;
                    })
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    .VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                        .Text(GroupText)
                        .Font(FAppStyle::GetFontStyle(TEXT("NormalFontBold")))
                        .ColorAndOpacity(FSlateColor::UseSubduedForeground())
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    [
                        SNew(SHorizontalBox)
                        .Visibility_Lambda([this, GroupText]()
                        {
                            return GroupHasToggleableSections(GroupText) ? EVisibility::Visible : EVisibility::Collapsed;
                        })
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        [
                            SNew(SButton)
                            .ButtonStyle(FAppStyle::Get(), TEXT("SimpleButton"))
                            .ContentPadding(FMargin(5.0f, 1.0f))
                            .ToolTipText(LOCTEXT("EnableGroupTip", "Enable every analysis in this category."))
                            .Text(LOCTEXT("EnableGroup", "On"))
                            .OnClicked_Lambda([this, GroupText]()
                            {
                                SetGroupEnabled(GroupText, true);
                                return FReply::Handled();
                            })
                        ]
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        [
                            SNew(SButton)
                            .ButtonStyle(FAppStyle::Get(), TEXT("SimpleButton"))
                            .ContentPadding(FMargin(5.0f, 1.0f))
                            .ToolTipText(LOCTEXT("DisableGroupTip", "Disable every analysis in this category."))
                            .Text(LOCTEXT("DisableGroup", "Off"))
                            .OnClicked_Lambda([this, GroupText]()
                            {
                                SetGroupEnabled(GroupText, false);
                                return FReply::Handled();
                            })
                        ]
                    ]
                ];
        }

        Content->AddSlot()
            .AutoHeight()
            .Padding(8.0f, 1.0f)
            [
                BuildSidebarSectionButton(Section)
            ];
    }

    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Header")))
        .Padding(0.0f)
        [
            SNew(SScrollBox)
            + SScrollBox::Slot()
            [
                Content
            ]
        ];
}

TSharedRef<SWidget> FMetricProfileEditorToolkit::BuildSidebarSectionButton(const FSection& Section)
{
    const FName PropertyName = Section.PropertyName;

    return SNew(SButton)
        .Visibility_Lambda([this, PropertyName]()
        {
            const FSection* CurrentSection = FindSection(PropertyName);
            return CurrentSection && SectionMatchesSearch(*CurrentSection) ? EVisibility::Visible : EVisibility::Collapsed;
        })
        .ButtonStyle(FAppStyle::Get(), TEXT("SimpleButton"))
        .ContentPadding(FMargin(8.0f, 5.0f))
        .ToolTipText(Section.Description)
        .ButtonColorAndOpacity_Lambda([this, PropertyName]()
        {
            return IsSectionSelected(PropertyName)
                ? FSlateColor(FLinearColor(0.20f, 0.36f, 0.56f, 0.65f))
                : FSlateColor(FLinearColor::Transparent);
        })
        .OnClicked_Lambda([this, PropertyName]()
        {
            SelectSection(PropertyName);
            return FReply::Handled();
        })
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(Section.Label)
                .Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(6.0f, 0.0f, 0.0f, 0.0f)
            [
                SNew(STextBlock)
                .Text_Lambda([this, PropertyName]() { return GetSectionComparisonMarker(PropertyName); })
                .Font(FAppStyle::GetFontStyle(TEXT("SmallFontBold")))
                .ColorAndOpacity_Lambda([this, PropertyName]() { return GetSectionComparisonMarkerColor(PropertyName); })
                .ToolTipText(LOCTEXT("ComparisonMarkerTip", "Changed values in this section compared with the selected reference profile."))
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(6.0f, 0.0f, 0.0f, 0.0f)
            [
                SNew(STextBlock)
                .Text_Lambda([this, PropertyName]() { return GetSectionValidationMarker(PropertyName); })
                .Font(FAppStyle::GetFontStyle(TEXT("SmallFontBold")))
                .ColorAndOpacity_Lambda([this, PropertyName]() { return GetSectionValidationMarkerColor(PropertyName); })
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(8.0f, 0.0f, 0.0f, 0.0f)
            [
                SNew(STextBlock)
                .Text_Lambda([this, PropertyName, Section]()
                {
                    if (!Section.bToggleable)
                    {
                        return LOCTEXT("Always", "Always");
                    }
                    return IsSectionEnabled(PropertyName) ? LOCTEXT("On", "On") : LOCTEXT("Off", "Off");
                })
                .Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
                .ColorAndOpacity_Lambda([this, PropertyName, Section]()
                {
                    if (!Section.bToggleable)
                    {
                        return FSlateColor::UseSubduedForeground();
                    }
                    return IsSectionEnabled(PropertyName)
                        ? FSlateColor(FLinearColor(0.35f, 0.90f, 0.48f, 1.0f))
                        : FSlateColor::UseSubduedForeground();
                })
            ]
        ];
}

TSharedRef<SWidget> FMetricProfileEditorToolkit::BuildSectionHeader()
{
    const FSection* Section = FindSection(EditorState->SelectedProperty);
    if (!Section)
    {
        return SNew(SBox);
    }

    const FName PropertyName = Section->PropertyName;
    const bool bToggleable = Section->bToggleable;

    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Header")))
        .Padding(FMargin(16.0f, 12.0f))
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SNew(STextBlock)
                    .Text(Section->Label)
                    .Font(FAppStyle::GetFontStyle(TEXT("NormalFontBold")))
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(Section->Fidelity)
                    .Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
                    .ColorAndOpacity(FSlateColor::UseSubduedForeground())
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(16.0f, 0.0f, 0.0f, 0.0f)
                [
                    SNew(SButton)
                    .ButtonStyle(FAppStyle::Get(), TEXT("SimpleButton"))
                    .IsEnabled(this, &FMetricProfileEditorToolkit::CanResetSelectedSectionToDefaults)
                    .ToolTipText(LOCTEXT("ResetSectionDefaultsTip", "Restore this section to the Metric Profile class defaults. Supports Undo."))
                    .Text(LOCTEXT("ResetSectionDefaults", "Reset to Default"))
                    .OnClicked(this, &FMetricProfileEditorToolkit::ResetSelectedSectionToDefaults)
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(8.0f, 0.0f, 0.0f, 0.0f)
                [
                    SNew(SButton)
                    .ButtonStyle(FAppStyle::Get(), TEXT("SimpleButton"))
                    .Visibility_Lambda([this]()
                    {
                        return ComparisonPreset ? EVisibility::Visible : EVisibility::Collapsed;
                    })
                    .IsEnabled(this, &FMetricProfileEditorToolkit::CanCopySelectedSectionFromComparison)
                    .ToolTipText(LOCTEXT("CopyComparisonSectionTip", "Copy this entire section from the selected comparison profile. Supports Undo."))
                    .Text(LOCTEXT("CopyComparisonSection", "Copy from Compare"))
                    .OnClicked(this, &FMetricProfileEditorToolkit::CopySelectedSectionFromComparison)
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(16.0f, 0.0f, 0.0f, 0.0f)
                [
                    SNew(SCheckBox)
                    .Visibility(bToggleable ? EVisibility::Visible : EVisibility::Collapsed)
                    .IsChecked_Lambda([this, PropertyName]()
                    {
                        return IsSectionEnabled(PropertyName) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
                    })
                    .OnCheckStateChanged_Lambda([this, PropertyName](ECheckBoxState NewState)
                    {
                        SetSectionEnabled(PropertyName, NewState == ECheckBoxState::Checked);
                    })
                    [
                        SNew(STextBlock)
                        .Text(LOCTEXT("Enabled", "Enabled in Profile"))
                    ]
                ]
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 5.0f, 0.0f, 0.0f)
            [
                SNew(STextBlock)
                .Text(Section->Description)
                .AutoWrapText(true)
                .ColorAndOpacity(FSlateColor::UseSubduedForeground())
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 10.0f, 0.0f, 0.0f)
            [
                BuildSelectedSectionComparisonPanel()
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 8.0f, 0.0f, 0.0f)
            [
                SNew(SBorder)
                .Visibility(this, &FMetricProfileEditorToolkit::GetSelectedSectionValidationVisibility)
                .BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Header")))
                .Padding(FMargin(8.0f, 6.0f))
                [
                    SNew(STextBlock)
                    .Text(this, &FMetricProfileEditorToolkit::GetSelectedSectionValidationText)
                    .AutoWrapText(true)
                    .ColorAndOpacity(FLinearColor(1.0f, 0.66f, 0.22f, 1.0f))
                ]
            ]
        ];
}

TSharedRef<SWidget> FMetricProfileEditorToolkit::BuildSelectedSectionComparisonPanel()
{
    if (!ComparisonPreset || !EditorState.IsValid())
    {
        return SNew(SBox).Visibility(EVisibility::Collapsed);
    }

    TArray<FComparisonRow> Rows;
    GatherSectionComparisonRows(EditorState->SelectedProperty, Rows);

    TSharedRef<SVerticalBox> Panel = SNew(SVerticalBox);

    Panel->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 5.0f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                SNew(STextBlock)
                .Text(FText::Format(
                    LOCTEXT("SelectedComparisonTitle", "Changed Values ({0})"),
                    FText::AsNumber(Rows.Num())))
                .Font(FAppStyle::GetFontStyle(TEXT("SmallFontBold")))
                .ColorAndOpacity(FSlateColor::UseSubduedForeground())
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(STextBlock)
                .Text(ComparisonPreset->DisplayName.IsEmpty() ? FText::FromString(ComparisonPreset->GetName()) : ComparisonPreset->DisplayName)
                .Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
                .ColorAndOpacity(FSlateColor::UseSubduedForeground())
            ]
        ];

    if (Rows.IsEmpty())
    {
        Panel->AddSlot()
            .AutoHeight()
            [
                SNew(STextBlock)
                .Text(LOCTEXT("SelectedComparisonMatch", "This section matches the comparison profile."))
                .ColorAndOpacity(FSlateColor::UseSubduedForeground())
            ];
    }
    else
    {
        TSharedRef<SGridPanel> Grid = SNew(SGridPanel).FillColumn(0, 0.30f).FillColumn(1, 0.32f).FillColumn(2, 0.32f);

        Grid->AddSlot(0, 0)
        [
            SNew(STextBlock).Text(LOCTEXT("ComparisonPropertyHeader", "Property")).Font(FAppStyle::GetFontStyle(TEXT("SmallFontBold")))
        ];
        Grid->AddSlot(1, 0).Padding(8.0f, 0.0f)
        [
            SNew(STextBlock).Text(LOCTEXT("ComparisonCurrentHeader", "Current")).Font(FAppStyle::GetFontStyle(TEXT("SmallFontBold")))
        ];
        Grid->AddSlot(2, 0).Padding(8.0f, 0.0f)
        [
            SNew(STextBlock).Text(LOCTEXT("ComparisonReferenceHeader", "Compare")).Font(FAppStyle::GetFontStyle(TEXT("SmallFontBold")))
        ];
        Grid->AddSlot(3, 0)
        [
            SNew(STextBlock).Text(FText::GetEmpty())
        ];

        int32 RowIndex = 1;
        for (const FComparisonRow& Row : Rows)
        {
            const TArray<FName> PropertyPath = Row.PropertyPath;

            Grid->AddSlot(0, RowIndex).Padding(0.0f, 3.0f)
            [
                SNew(STextBlock)
                .Text(Row.Label)
                .Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
            ];
            Grid->AddSlot(1, RowIndex).Padding(8.0f, 3.0f)
            [
                SNew(STextBlock)
                .Text(Row.CurrentValue)
                .Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
                .ColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.88f, 0.88f, 1.0f)))
            ];
            Grid->AddSlot(2, RowIndex).Padding(8.0f, 3.0f)
            [
                SNew(STextBlock)
                .Text(Row.ComparisonValue)
                .Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
                .ColorAndOpacity(FSlateColor::UseForeground())
            ];
            Grid->AddSlot(3, RowIndex).Padding(4.0f, 1.0f)
            [
                SNew(SButton)
                .ButtonStyle(FAppStyle::Get(), TEXT("SimpleButton"))
                .ContentPadding(FMargin(6.0f, 1.0f))
                .ToolTipText(LOCTEXT("CopyComparisonValueTip", "Copy only this value from the comparison profile. Supports Undo."))
                .Text(LOCTEXT("CopyComparisonValue", "Copy"))
                .OnClicked_Lambda([this, PropertyPath]()
                {
                    return CopyComparisonPropertyValue(PropertyPath);
                })
            ];
            ++RowIndex;
        }

        Panel->AddSlot()
            .AutoHeight()
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
                .Padding(FMargin(8.0f, 6.0f))
                [
                    Grid
                ]
            ];
    }

    return SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Header")))
        .Padding(FMargin(8.0f, 6.0f))
        [
            Panel
        ];
}

void FMetricProfileEditorToolkit::RebuildSectionHeader()
{
    if (!SectionHeaderBox.IsValid())
    {
        return;
    }

    SectionHeaderBox->ClearChildren();
    SectionHeaderBox->AddSlot().AutoHeight()[BuildSectionHeader()];
}

void FMetricProfileEditorToolkit::SelectSection(FName PropertyName)
{
    if (EditorState->SelectedProperty == PropertyName)
    {
        return;
    }

    EditorState->SelectedProperty = PropertyName;
    RebuildSectionHeader();

    if (DetailsView.IsValid())
    {
        DetailsView->ForceRefresh();
    }
}

const FMetricProfileEditorToolkit::FSection* FMetricProfileEditorToolkit::FindSection(FName PropertyName) const
{
    return Sections.FindByPredicate([PropertyName](const FSection& Section)
    {
        return Section.PropertyName == PropertyName;
    });
}

bool FMetricProfileEditorToolkit::IsSectionSelected(FName PropertyName) const
{
    return EditorState.IsValid() && EditorState->SelectedProperty == PropertyName;
}

bool FMetricProfileEditorToolkit::IsSectionEnabled(FName PropertyName) const
{
    if (!Preset)
    {
        return false;
    }

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, VisualTheme)) return true;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, Hull)) return true;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, Clearance)) return Preset->Clearance.bShowClearanceProbe;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, OverheadClearance)) return Preset->OverheadClearance.bShowOverheadClearance;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, Sensors)) return Preset->Sensors.bShowSensorHeights;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, SensorField)) return Preset->SensorField.bShowSensorField;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, EngagementRanges)) return Preset->EngagementRanges.bShowEngagementRanges;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, LocomotionBands)) return Preset->LocomotionBands.bShowLocomotionBands;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, Trajectory)) return Preset->Trajectory.bShowTrajectory;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, CameraProbe)) return Preset->CameraProbe.bShowCameraProbe;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, CameraSightline)) return Preset->CameraSightline.bShowCameraSightline;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, GroundValidation)) return Preset->GroundValidation.bShowGroundValidation;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, FitAnalysis)) return Preset->FitAnalysis.bShowFitAnalysis;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, ThresholdAnalysis)) return Preset->ThresholdAnalysis.bShowThresholdAnalysis;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, TurnClearance)) return Preset->TurnClearance.bShowTurnClearance;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, LedgeAnalysis)) return Preset->LedgeAnalysis.bShowLedgeAnalysis;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, StoppingDistance)) return Preset->StoppingDistance.bShowStoppingDistance;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, CoverExposure)) return Preset->CoverExposure.bShowCoverExposure;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, JumpRunway)) return Preset->JumpRunway.bShowJumpRunway;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, InteractionReach)) return Preset->InteractionReach.bShowInteractionReach;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, ShoulderClearance)) return Preset->ShoulderClearance.bShowShoulderClearance;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, CombatDensity)) return Preset->CombatDensity.bShowCombatDensity;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, AlternateStance)) return Preset->AlternateStance.bShowAlternateStance;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, MantleVault)) return Preset->MantleVault.bShowMantleVault;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, RecoverySpace)) return Preset->RecoverySpace.bShowRecoverySpace;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, Crossfire)) return Preset->Crossfire.bShowCrossfire;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, Representation)) return Preset->Representation.bShowStaticMesh;
    return false;
}

void FMetricProfileEditorToolkit::SetSectionEnabled(FName PropertyName, bool bEnabled)
{
    if (!Preset || PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, Hull) || IsSectionEnabled(PropertyName) == bEnabled)
    {
        return;
    }

    const FScopedTransaction Transaction(LOCTEXT("ToggleMetricAnalysis", "Toggle Metric Profile Analysis"));
    Preset->Modify();
    SetSectionEnabledValue(PropertyName, bEnabled);
    Preset->PostEditChange();
    Preset->MarkPackageDirty();

    if (DetailsView.IsValid())
    {
        DetailsView->ForceRefresh();
    }
    RebuildSectionHeader();
}

bool FMetricProfileEditorToolkit::SetSectionEnabledValue(FName PropertyName, bool bEnabled)
{
    if (!Preset)
    {
        return false;
    }

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, Clearance)) Preset->Clearance.bShowClearanceProbe = bEnabled;
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, OverheadClearance)) Preset->OverheadClearance.bShowOverheadClearance = bEnabled;
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, Sensors)) Preset->Sensors.bShowSensorHeights = bEnabled;
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, SensorField)) Preset->SensorField.bShowSensorField = bEnabled;
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, EngagementRanges)) Preset->EngagementRanges.bShowEngagementRanges = bEnabled;
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, LocomotionBands)) Preset->LocomotionBands.bShowLocomotionBands = bEnabled;
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, Trajectory)) Preset->Trajectory.bShowTrajectory = bEnabled;
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, CameraProbe)) Preset->CameraProbe.bShowCameraProbe = bEnabled;
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, CameraSightline)) Preset->CameraSightline.bShowCameraSightline = bEnabled;
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, GroundValidation)) Preset->GroundValidation.bShowGroundValidation = bEnabled;
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, FitAnalysis)) Preset->FitAnalysis.bShowFitAnalysis = bEnabled;
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, ThresholdAnalysis)) Preset->ThresholdAnalysis.bShowThresholdAnalysis = bEnabled;
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, TurnClearance)) Preset->TurnClearance.bShowTurnClearance = bEnabled;
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, LedgeAnalysis)) Preset->LedgeAnalysis.bShowLedgeAnalysis = bEnabled;
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, StoppingDistance)) Preset->StoppingDistance.bShowStoppingDistance = bEnabled;
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, CoverExposure)) Preset->CoverExposure.bShowCoverExposure = bEnabled;
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, JumpRunway)) Preset->JumpRunway.bShowJumpRunway = bEnabled;
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, InteractionReach)) Preset->InteractionReach.bShowInteractionReach = bEnabled;
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, ShoulderClearance)) Preset->ShoulderClearance.bShowShoulderClearance = bEnabled;
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, CombatDensity)) Preset->CombatDensity.bShowCombatDensity = bEnabled;
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, AlternateStance)) Preset->AlternateStance.bShowAlternateStance = bEnabled;
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, MantleVault)) Preset->MantleVault.bShowMantleVault = bEnabled;
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, RecoverySpace)) Preset->RecoverySpace.bShowRecoverySpace = bEnabled;
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, Crossfire)) Preset->Crossfire.bShowCrossfire = bEnabled;
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(UMetricPreset, Representation)) Preset->Representation.bShowStaticMesh = bEnabled;
    else return false;

    return true;
}

void FMetricProfileEditorToolkit::SetGroupEnabled(const FText& Group, bool bEnabled)
{
    if (!Preset)
    {
        return;
    }

    const FScopedTransaction Transaction(bEnabled
        ? LOCTEXT("EnableMetricCategory", "Enable Metric Profile Category")
        : LOCTEXT("DisableMetricCategory", "Disable Metric Profile Category"));
    Preset->Modify();

    bool bChanged = false;
    for (const FSection& Section : Sections)
    {
        if (Section.bToggleable && Section.Group.EqualTo(Group) && IsSectionEnabled(Section.PropertyName) != bEnabled)
        {
            bChanged |= SetSectionEnabledValue(Section.PropertyName, bEnabled);
        }
    }

    if (!bChanged)
    {
        return;
    }

    Preset->PostEditChange();
    Preset->MarkPackageDirty();
    if (DetailsView.IsValid())
    {
        DetailsView->ForceRefresh();
    }
    RebuildSectionHeader();
}

bool FMetricProfileEditorToolkit::GroupHasToggleableSections(const FText& Group) const
{
    return Sections.ContainsByPredicate([&Group](const FSection& Section)
    {
        return Section.bToggleable && Section.Group.EqualTo(Group);
    });
}

bool FMetricProfileEditorToolkit::SectionMatchesSearch(const FSection& Section) const
{
    if (bShowComparisonChangesOnly && ComparisonPreset && !IsSectionDifferentFromComparison(Section.PropertyName))
    {
        return false;
    }

    if (SearchText.IsEmpty())
    {
        return true;
    }

    const FString Needle = SearchText.ToString();
    return Section.Label.ToString().Contains(Needle, ESearchCase::IgnoreCase)
        || Section.Group.ToString().Contains(Needle, ESearchCase::IgnoreCase)
        || Section.Description.ToString().Contains(Needle, ESearchCase::IgnoreCase)
        || Section.Fidelity.ToString().Contains(Needle, ESearchCase::IgnoreCase);
}

bool FMetricProfileEditorToolkit::CanNavigateComparisonChanges() const
{
    return ComparisonPreset && CountComparisonDifferences() > 0;
}

void FMetricProfileEditorToolkit::NavigateComparisonChange(int32 Direction)
{
    if (!CanNavigateComparisonChanges() || !EditorState.IsValid() || Direction == 0)
    {
        return;
    }

    int32 StartIndex = Sections.IndexOfByPredicate([this](const FSection& Section)
    {
        return Section.PropertyName == EditorState->SelectedProperty;
    });
    if (StartIndex == INDEX_NONE)
    {
        StartIndex = 0;
    }

    for (int32 Offset = 1; Offset <= Sections.Num(); ++Offset)
    {
        const int32 CandidateIndex = (StartIndex + Direction * Offset + Sections.Num() * 2) % Sections.Num();
        const FSection& Candidate = Sections[CandidateIndex];
        if (IsSectionDifferentFromComparison(Candidate.PropertyName))
        {
            SelectSection(Candidate.PropertyName);
            return;
        }
    }
}

bool FMetricProfileEditorToolkit::GroupMatchesSearch(const FText& Group) const
{
    return Sections.ContainsByPredicate([this, &Group](const FSection& Section)
    {
        return Section.Group.EqualTo(Group) && SectionMatchesSearch(Section);
    });
}

void FMetricProfileEditorToolkit::GatherValidationIssues(TArray<FValidationIssue>& OutIssues) const
{
    OutIssues.Reset();
    if (!Preset)
    {
        return;
    }

    auto AddWarning = [&OutIssues](FName PropertyName, const FText& Message)
    {
        OutIssues.Add({ PropertyName, Message, false });
    };
    auto AddError = [&OutIssues](FName PropertyName, const FText& Message)
    {
        OutIssues.Add({ PropertyName, Message, true });
    };

    if (Preset->Hull.Height <= 0.0f || Preset->Hull.Width <= 0.0f || Preset->Hull.Length <= 0.0f)
    {
        AddError(GET_MEMBER_NAME_CHECKED(UMetricPreset, Hull), LOCTEXT("ValidationHullDimensions", "Hull dimensions must all be greater than zero."));
    }
    if (Preset->Sensors.bShowSensorHeights && Preset->Sensors.LowProfileSensorZOffset > Preset->Sensors.PrimarySensorZOffset)
    {
        AddWarning(GET_MEMBER_NAME_CHECKED(UMetricPreset, Sensors), LOCTEXT("ValidationSensorOrder", "Low-profile sensor height is above the primary sensor height."));
    }
    if (Preset->EngagementRanges.bShowEngagementRanges
        && !(Preset->EngagementRanges.ShortRange <= Preset->EngagementRanges.MediumRange
            && Preset->EngagementRanges.MediumRange <= Preset->EngagementRanges.LongRange))
    {
        AddWarning(GET_MEMBER_NAME_CHECKED(UMetricPreset, EngagementRanges), LOCTEXT("ValidationRangeOrder", "Engagement ranges should be ordered Short <= Medium <= Long."));
    }
    if (Preset->LocomotionBands.bShowLocomotionBands
        && !(Preset->LocomotionBands.NearTime <= Preset->LocomotionBands.MidTime
            && Preset->LocomotionBands.MidTime <= Preset->LocomotionBands.FarTime))
    {
        AddWarning(GET_MEMBER_NAME_CHECKED(UMetricPreset, LocomotionBands), LOCTEXT("ValidationLocomotionOrder", "Locomotion times should be ordered Near <= Mid <= Far."));
    }
    if (Preset->Trajectory.bShowTrajectory && Preset->Trajectory.SampleInterval >= Preset->Trajectory.MaxSimulationTime)
    {
        AddError(GET_MEMBER_NAME_CHECKED(UMetricPreset, Trajectory), LOCTEXT("ValidationTrajectorySampling", "Trajectory sample interval must be smaller than maximum simulation time."));
    }
    if (Preset->CameraProbe.bShowCameraProbe && Preset->CameraProbe.MinDistance > Preset->CameraProbe.TargetDistance)
    {
        AddWarning(GET_MEMBER_NAME_CHECKED(UMetricPreset, CameraProbe), LOCTEXT("ValidationCameraDistance", "Camera minimum distance is greater than the target distance, so the camera will always be considered too close."));
    }
    if (Preset->CoverExposure.bShowCoverExposure
        && !(Preset->CoverExposure.LowSampleHeight < Preset->CoverExposure.TorsoSampleHeight
            && Preset->CoverExposure.TorsoSampleHeight < Preset->CoverExposure.HeadSampleHeight))
    {
        AddWarning(GET_MEMBER_NAME_CHECKED(UMetricPreset, CoverExposure), LOCTEXT("ValidationCoverHeights", "Cover sample heights should be ordered Low < Torso < Head."));
    }
    if (Preset->AlternateStance.bShowAlternateStance && Preset->AlternateStance.AlternateHeight >= Preset->Hull.Height)
    {
        AddWarning(GET_MEMBER_NAME_CHECKED(UMetricPreset, AlternateStance), LOCTEXT("ValidationAlternateHeight", "Alternate stance height is not lower than the standing hull height."));
    }
    if (Preset->MantleVault.bShowMantleVault && Preset->MantleVault.MinObstacleHeight > Preset->MantleVault.MaxObstacleHeight)
    {
        AddError(GET_MEMBER_NAME_CHECKED(UMetricPreset, MantleVault), LOCTEXT("ValidationMantleRange", "Minimum obstacle height is greater than maximum obstacle height."));
    }
}

FText FMetricProfileEditorToolkit::GetValidationSummary() const
{
    TArray<FValidationIssue> Issues;
    GatherValidationIssues(Issues);
    if (Issues.IsEmpty())
    {
        return LOCTEXT("ValidationClean", "Profile validation: no issues");
    }

    int32 ErrorCount = 0;
    for (const FValidationIssue& Issue : Issues)
    {
        ErrorCount += Issue.bError ? 1 : 0;
    }
    const int32 WarningCount = Issues.Num() - ErrorCount;
    return FText::Format(LOCTEXT("ValidationSummary", "Profile validation: {0} error(s), {1} warning(s)"), FText::AsNumber(ErrorCount), FText::AsNumber(WarningCount));
}

FSlateColor FMetricProfileEditorToolkit::GetValidationSummaryColor() const
{
    TArray<FValidationIssue> Issues;
    GatherValidationIssues(Issues);
    if (Issues.IsEmpty())
    {
        return FSlateColor(FLinearColor(0.42f, 0.78f, 0.52f, 1.0f));
    }
    const bool bHasError = Issues.ContainsByPredicate([](const FValidationIssue& Issue) { return Issue.bError; });
    return bHasError
        ? FSlateColor(FLinearColor(0.95f, 0.34f, 0.34f, 1.0f))
        : FSlateColor(FLinearColor(1.0f, 0.66f, 0.22f, 1.0f));
}

FText FMetricProfileEditorToolkit::GetSelectedSectionValidationText() const
{
    TArray<FValidationIssue> Issues;
    GatherValidationIssues(Issues);
    TArray<FText> Messages;
    for (const FValidationIssue& Issue : Issues)
    {
        if (Issue.PropertyName == EditorState->SelectedProperty)
        {
            Messages.Add(FText::Format(Issue.bError ? LOCTEXT("ValidationErrorLine", "Error: {0}") : LOCTEXT("ValidationWarningLine", "Warning: {0}"), Issue.Message));
        }
    }
    FString Combined;
    for (int32 Index = 0; Index < Messages.Num(); ++Index)
    {
        if (Index > 0)
        {
            Combined += TEXT("\n");
        }
        Combined += Messages[Index].ToString();
    }
    return FText::FromString(Combined);
}

EVisibility FMetricProfileEditorToolkit::GetSelectedSectionValidationVisibility() const
{
    return GetSelectedSectionValidationText().IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
}


FText FMetricProfileEditorToolkit::GetSectionValidationMarker(FName PropertyName) const
{
    TArray<FValidationIssue> Issues;
    GatherValidationIssues(Issues);
    const bool bHasIssue = Issues.ContainsByPredicate([PropertyName](const FValidationIssue& Issue)
    {
        return Issue.PropertyName == PropertyName;
    });
    return bHasIssue ? LOCTEXT("ValidationMarker", "!") : FText::GetEmpty();
}

FSlateColor FMetricProfileEditorToolkit::GetSectionValidationMarkerColor(FName PropertyName) const
{
    TArray<FValidationIssue> Issues;
    GatherValidationIssues(Issues);
    bool bHasWarning = false;
    for (const FValidationIssue& Issue : Issues)
    {
        if (Issue.PropertyName == PropertyName)
        {
            if (Issue.bError)
            {
                return FSlateColor(FLinearColor(0.95f, 0.34f, 0.34f, 1.0f));
            }
            bHasWarning = true;
        }
    }
    return bHasWarning
        ? FSlateColor(FLinearColor(1.0f, 0.66f, 0.22f, 1.0f))
        : FSlateColor::UseSubduedForeground();
}

int32 FMetricProfileEditorToolkit::CountEnabledAnalyses() const
{
    int32 Count = 0;
    for (const FSection& Section : Sections)
    {
        if (Section.bToggleable && IsSectionEnabled(Section.PropertyName))
        {
            ++Count;
        }
    }
    return Count;
}

int32 FMetricProfileEditorToolkit::CountToggleableAnalyses() const
{
    int32 Count = 0;
    for (const FSection& Section : Sections)
    {
        Count += Section.bToggleable ? 1 : 0;
    }
    return Count;
}


void FMetricProfileEditorToolkit::SetComparisonPreset(UMetricPreset* InPreset)
{
    ComparisonPreset = (InPreset && InPreset != Preset.Get()) ? InPreset : nullptr;
    if (!ComparisonPreset)
    {
        bShowComparisonChangesOnly = false;
    }
    RebuildSectionHeader();
}

bool FMetricProfileEditorToolkit::IsSectionDifferentFromComparison(FName PropertyName) const
{
    if (!Preset || !ComparisonPreset)
    {
        return false;
    }

    const FProperty* Property = FindFProperty<FProperty>(UMetricPreset::StaticClass(), PropertyName);
    return Property && !Property->Identical_InContainer(Preset.Get(), ComparisonPreset.Get());
}

int32 FMetricProfileEditorToolkit::CountComparisonDifferences() const
{
    if (!ComparisonPreset)
    {
        return 0;
    }

    int32 Count = 0;
    for (const FSection& Section : Sections)
    {
        Count += IsSectionDifferentFromComparison(Section.PropertyName) ? 1 : 0;
    }
    return Count;
}

int32 FMetricProfileEditorToolkit::CountComparisonPropertyDifferences() const
{
    if (!ComparisonPreset)
    {
        return 0;
    }

    int32 Count = 0;
    for (const FSection& Section : Sections)
    {
        Count += CountSectionPropertyDifferences(Section.PropertyName);
    }
    return Count;
}

int32 FMetricProfileEditorToolkit::CountSectionPropertyDifferences(FName PropertyName) const
{
    TArray<FComparisonRow> Rows;
    GatherSectionComparisonRows(PropertyName, Rows);
    return Rows.Num();
}

void FMetricProfileEditorToolkit::GatherSectionComparisonRows(FName PropertyName, TArray<FComparisonRow>& OutRows) const
{
    OutRows.Reset();
    if (!Preset || !ComparisonPreset)
    {
        return;
    }

    const FProperty* RootProperty = FindFProperty<FProperty>(UMetricPreset::StaticClass(), PropertyName);
    if (!RootProperty || RootProperty->Identical_InContainer(Preset.Get(), ComparisonPreset.Get()))
    {
        return;
    }

    const FStructProperty* RootStruct = CastField<FStructProperty>(RootProperty);
    if (!RootStruct || !RootStruct->Struct)
    {
        FComparisonRow& Row = OutRows.AddDefaulted_GetRef();
        Row.Label = RootProperty->GetDisplayNameText();
        Row.CurrentValue = ExportPropertyValue(RootProperty, Preset.Get());
        Row.ComparisonValue = ExportPropertyValue(RootProperty, ComparisonPreset.Get());
        return;
    }

    const void* CurrentContainer = RootProperty->ContainerPtrToValuePtr<void>(Preset.Get());
    const void* ComparisonContainer = RootProperty->ContainerPtrToValuePtr<void>(ComparisonPreset.Get());
    TArray<FName> Path;
    GatherStructComparisonRows(RootStruct->Struct, CurrentContainer, ComparisonContainer, Path, FString(), OutRows);
}

void FMetricProfileEditorToolkit::GatherStructComparisonRows(const UStruct* StructType, const void* CurrentContainer, const void* ComparisonContainer, TArray<FName>& PropertyPath, const FString& LabelPrefix, TArray<FComparisonRow>& OutRows) const
{
    if (!StructType || !CurrentContainer || !ComparisonContainer)
    {
        return;
    }

    for (TFieldIterator<FProperty> It(StructType, EFieldIteratorFlags::ExcludeSuper); It; ++It)
    {
        const FProperty* Property = *It;
        if (!Property || !Property->HasAnyPropertyFlags(CPF_Edit))
        {
            continue;
        }

        const void* CurrentValue = Property->ContainerPtrToValuePtr<void>(CurrentContainer);
        const void* ComparisonValue = Property->ContainerPtrToValuePtr<void>(ComparisonContainer);
        if (Property->Identical(CurrentValue, ComparisonValue))
        {
            continue;
        }

        PropertyPath.Add(Property->GetFName());
        const FString DisplayName = Property->GetDisplayNameText().ToString();
        const FString FullLabel = LabelPrefix.IsEmpty() ? DisplayName : LabelPrefix + TEXT(" / ") + DisplayName;

        const FStructProperty* StructProperty = CastField<FStructProperty>(Property);
        const bool bMetricStruct = StructProperty && StructProperty->Struct && StructProperty->Struct->GetName().StartsWith(TEXT("Metric"));
        if (bMetricStruct)
        {
            GatherStructComparisonRows(StructProperty->Struct, CurrentValue, ComparisonValue, PropertyPath, FullLabel, OutRows);
        }
        else
        {
            FComparisonRow& Row = OutRows.AddDefaulted_GetRef();
            Row.PropertyPath = PropertyPath;
            Row.Label = FText::FromString(FullLabel);
            Row.CurrentValue = ExportPropertyValue(Property, CurrentContainer);
            Row.ComparisonValue = ExportPropertyValue(Property, ComparisonContainer);
        }

        PropertyPath.Pop();
    }
}

FText FMetricProfileEditorToolkit::ExportPropertyValue(const FProperty* Property, const void* Container) const
{
    if (!Property || !Container)
    {
        return FText::GetEmpty();
    }

    FString Value;
    Property->ExportText_InContainer(0, Value, Container, Container, nullptr, PPF_PropertyWindow);
    if (Value.Len() > 96)
    {
        Value = Value.Left(93) + TEXT("...");
    }
    return FText::FromString(Value);
}

FText FMetricProfileEditorToolkit::GetComparisonSummary() const
{
    if (!ComparisonPreset)
    {
        return LOCTEXT("ComparisonNone", "Choose another Metric Profile to compare values section-by-section.");
    }

    const int32 SectionDifferenceCount = CountComparisonDifferences();
    const int32 PropertyDifferenceCount = CountComparisonPropertyDifferences();
    return FText::Format(
        LOCTEXT("ComparisonSummary", "{0} changed value(s) across {1} section(s) compared with {2}."),
        FText::AsNumber(PropertyDifferenceCount),
        FText::AsNumber(SectionDifferenceCount),
        ComparisonPreset->DisplayName.IsEmpty() ? FText::FromString(ComparisonPreset->GetName()) : ComparisonPreset->DisplayName);
}

FText FMetricProfileEditorToolkit::GetSectionComparisonMarker(FName PropertyName) const
{
    const int32 DifferenceCount = CountSectionPropertyDifferences(PropertyName);
    if (DifferenceCount <= 0)
    {
        return FText::GetEmpty();
    }

    return DifferenceCount == 1
        ? LOCTEXT("ComparisonOneChange", "1 change")
        : FText::Format(LOCTEXT("ComparisonManyChanges", "{0} changes"), FText::AsNumber(DifferenceCount));
}

FSlateColor FMetricProfileEditorToolkit::GetSectionComparisonMarkerColor(FName /*PropertyName*/) const
{
    return FSlateColor::UseSubduedForeground();
}

bool FMetricProfileEditorToolkit::CanResetSelectedSectionToDefaults() const
{
    if (!Preset || !EditorState.IsValid())
    {
        return false;
    }

    const UMetricPreset* Defaults = GetDefault<UMetricPreset>();
    const FProperty* Property = FindFProperty<FProperty>(UMetricPreset::StaticClass(), EditorState->SelectedProperty);
    return Defaults && Property && !Property->Identical_InContainer(Preset.Get(), Defaults);
}

FReply FMetricProfileEditorToolkit::ResetSelectedSectionToDefaults()
{
    if (!CanResetSelectedSectionToDefaults())
    {
        return FReply::Handled();
    }

    const UMetricPreset* Defaults = GetDefault<UMetricPreset>();
    FProperty* Property = FindFProperty<FProperty>(UMetricPreset::StaticClass(), EditorState->SelectedProperty);
    if (!Defaults || !Property)
    {
        return FReply::Handled();
    }

    const FScopedTransaction Transaction(LOCTEXT("ResetMetricProfileSection", "Reset Metric Profile Section to Default"));
    Preset->Modify();
    Property->CopyCompleteValue_InContainer(Preset.Get(), Defaults);
    Preset->PostEditChange();
    Preset->MarkPackageDirty();

    if (DetailsView.IsValid())
    {
        DetailsView->ForceRefresh();
    }
    RebuildSectionHeader();
    return FReply::Handled();
}

bool FMetricProfileEditorToolkit::CanCopySelectedSectionFromComparison() const
{
    return ComparisonPreset && EditorState.IsValid() && IsSectionDifferentFromComparison(EditorState->SelectedProperty);
}

FReply FMetricProfileEditorToolkit::CopySelectedSectionFromComparison()
{
    if (!Preset || !ComparisonPreset || !EditorState.IsValid())
    {
        return FReply::Handled();
    }

    FProperty* Property = FindFProperty<FProperty>(UMetricPreset::StaticClass(), EditorState->SelectedProperty);
    if (!Property || Property->Identical_InContainer(Preset.Get(), ComparisonPreset.Get()))
    {
        return FReply::Handled();
    }

    const FScopedTransaction Transaction(LOCTEXT("CopyMetricProfileSection", "Copy Metric Profile Section"));
    Preset->Modify();
    Property->CopyCompleteValue_InContainer(Preset.Get(), ComparisonPreset.Get());
    Preset->PostEditChange();
    Preset->MarkPackageDirty();

    if (DetailsView.IsValid())
    {
        DetailsView->ForceRefresh();
    }
    RebuildSectionHeader();
    return FReply::Handled();
}

FReply FMetricProfileEditorToolkit::CopyComparisonPropertyValue(TArray<FName> PropertyPath)
{
    if (!Preset || !ComparisonPreset || !EditorState.IsValid() || PropertyPath.IsEmpty())
    {
        return FReply::Handled();
    }

    FProperty* RootProperty = FindFProperty<FProperty>(UMetricPreset::StaticClass(), EditorState->SelectedProperty);
    FStructProperty* RootStruct = CastField<FStructProperty>(RootProperty);
    if (!RootProperty || !RootStruct || !RootStruct->Struct)
    {
        return FReply::Handled();
    }

    void* CurrentContainer = RootProperty->ContainerPtrToValuePtr<void>(Preset.Get());
    const void* ComparisonContainer = RootProperty->ContainerPtrToValuePtr<void>(ComparisonPreset.Get());
    UStruct* CurrentStructType = RootStruct->Struct;

    FProperty* LeafProperty = nullptr;
    void* CurrentLeafContainer = nullptr;
    const void* ComparisonLeafContainer = nullptr;

    for (int32 Index = 0; Index < PropertyPath.Num(); ++Index)
    {
        FProperty* Property = FindFProperty<FProperty>(CurrentStructType, PropertyPath[Index]);
        if (!Property)
        {
            return FReply::Handled();
        }

        if (Index == PropertyPath.Num() - 1)
        {
            LeafProperty = Property;
            CurrentLeafContainer = CurrentContainer;
            ComparisonLeafContainer = ComparisonContainer;
            break;
        }

        FStructProperty* StructProperty = CastField<FStructProperty>(Property);
        if (!StructProperty || !StructProperty->Struct)
        {
            return FReply::Handled();
        }

        CurrentContainer = Property->ContainerPtrToValuePtr<void>(CurrentContainer);
        ComparisonContainer = Property->ContainerPtrToValuePtr<void>(ComparisonContainer);
        CurrentStructType = StructProperty->Struct;
    }

    if (!LeafProperty || !CurrentLeafContainer || !ComparisonLeafContainer || LeafProperty->Identical_InContainer(CurrentLeafContainer, ComparisonLeafContainer))
    {
        return FReply::Handled();
    }

    const FScopedTransaction Transaction(LOCTEXT("CopyMetricProfileValue", "Copy Metric Profile Value"));
    Preset->Modify();
    LeafProperty->CopyCompleteValue_InContainer(CurrentLeafContainer, ComparisonLeafContainer);
    Preset->PostEditChange();
    Preset->MarkPackageDirty();

    if (DetailsView.IsValid())
    {
        DetailsView->ForceRefresh();
    }
    RebuildSectionHeader();
    return FReply::Handled();
}

FText FMetricProfileEditorToolkit::GetProfileTitle() const
{
    if (!Preset)
    {
        return LOCTEXT("UnknownProfile", "Metric Profile");
    }
    return Preset->DisplayName.IsEmpty() ? FText::FromString(Preset->GetName()) : Preset->DisplayName;
}

FText FMetricProfileEditorToolkit::GetProfileSummary() const
{
    return FText::Format(
        LOCTEXT("ProfileSummary", "{0} of {1} profile features enabled"),
        FText::AsNumber(CountEnabledAnalyses()),
        FText::AsNumber(CountToggleableAnalyses()));
}

#undef LOCTEXT_NAMESPACE
