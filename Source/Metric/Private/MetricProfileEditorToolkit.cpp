#include "MetricProfileEditorToolkit.h"

#include "MetricPreset.h"
#include "MetricProfileDetailsCustomization.h"
#include "Editor.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
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
}

FMetricProfileEditorToolkit::FMetricProfileEditorToolkit()
{
    EditorState = MakeShared<FMetricProfileEditorState>();

    const FText Core = LOCTEXT("GroupCore", "CORE");
    const FText Space = LOCTEXT("GroupSpace", "SPACE & FIT");
    const FText Traversal = LOCTEXT("GroupTraversal", "MOVEMENT & TRAVERSAL");
    const FText Camera = LOCTEXT("GroupCamera", "CAMERA & INTERACTION");
    const FText Combat = LOCTEXT("GroupCombat", "COMBAT & EXPOSURE");
    const FText Visual = LOCTEXT("GroupVisual", "REPRESENTATION");

    Sections = {
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

    RegenerateMenusAndToolbars();
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

    Content->AddSlot().AutoHeight().Padding(10.0f, 0.0f, 10.0f, 8.0f)[SNew(SSeparator)];

    FText PreviousGroup;
    for (const FSection& Section : Sections)
    {
        if (!Section.Group.EqualTo(PreviousGroup))
        {
            PreviousGroup = Section.Group;
            Content->AddSlot()
                .AutoHeight()
                .Padding(14.0f, 10.0f, 14.0f, 3.0f)
                [
                    SNew(STextBlock)
                    .Text(Section.Group)
                    .Font(FAppStyle::GetFontStyle(TEXT("NormalFontBold")))
                    .ColorAndOpacity(FSlateColor::UseSubduedForeground())
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

    Preset->PostEditChange();
    Preset->MarkPackageDirty();

    if (DetailsView.IsValid())
    {
        DetailsView->ForceRefresh();
    }
    RebuildSectionHeader();
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
