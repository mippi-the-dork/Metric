#include "MetricProfileDetailsCustomization.h"

#include "MetricPreset.h"
#include "MetricProfileEditorToolkit.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "IDetailGroup.h"
#include "IDetailPropertyRow.h"
#include "PropertyHandle.h"
#include "UObject/UnrealType.h"

#define LOCTEXT_NAMESPACE "MetricProfileDetailsCustomization"

namespace MetricProfileDetails
{
    static TSharedPtr<IPropertyHandle> Child(const TSharedRef<IPropertyHandle>& Parent, const FName Name)
    {
        TSharedPtr<IPropertyHandle> Handle = Parent->GetChildHandle(Name);
        return Handle.IsValid() && Handle->IsValidHandle() ? Handle : nullptr;
    }

    static void AddColorRow(IDetailGroup& Group, const TSharedRef<IPropertyHandle>& ThemeHandle, const FName PropertyName, const FText& DisplayName)
    {
        if (TSharedPtr<IPropertyHandle> Handle = Child(ThemeHandle, PropertyName))
        {
            Group.AddPropertyRow(Handle.ToSharedRef()).DisplayName(DisplayName);
        }
    }

    static void AddNestedColorRow(IDetailGroup& Group, const TSharedRef<IPropertyHandle>& ThemeHandle, const FName StructPropertyName, const FName ColorPropertyName, const FText& DisplayName)
    {
        if (TSharedPtr<IPropertyHandle> StructHandle = Child(ThemeHandle, StructPropertyName))
        {
            if (TSharedPtr<IPropertyHandle> Handle = Child(StructHandle.ToSharedRef(), ColorPropertyName))
            {
                Group.AddPropertyRow(Handle.ToSharedRef()).DisplayName(DisplayName);
            }
        }
    }

    static IDetailGroup& AddColorGroup(IDetailCategoryBuilder& Category, const FName GroupName, const FText& DisplayName)
    {
        return Category.AddGroup(GroupName, DisplayName, false, false);
    }

    static void BuildVisualizationColors(IDetailCategoryBuilder& Category, const TSharedRef<IPropertyHandle>& ThemeHandle)
    {
        {
            IDetailGroup& Group = AddColorGroup(Category, TEXT("HullColors"), LOCTEXT("HullColors", "Hull"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, Hull), LOCTEXT("Hull", "Hull"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, Forward), LOCTEXT("Forward", "Forward"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, Anchor), LOCTEXT("Anchor", "Anchor"));
        }
        {
            IDetailGroup& Group = AddColorGroup(Category, TEXT("ClearanceColors"), LOCTEXT("ClearanceColors", "Clearance"));
            AddNestedColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, Clearance), GET_MEMBER_NAME_CHECKED(FMetricStatusColors, Pass), LOCTEXT("Pass", "Pass"));
            AddNestedColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, Clearance), GET_MEMBER_NAME_CHECKED(FMetricStatusColors, Tight), LOCTEXT("Tight", "Tight"));
            AddNestedColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, Clearance), GET_MEMBER_NAME_CHECKED(FMetricStatusColors, Fail), LOCTEXT("Fail", "Fail"));
        }
        {
            IDetailGroup& Group = AddColorGroup(Category, TEXT("OverheadColors"), LOCTEXT("OverheadColors", "Overhead Clearance"));
            AddNestedColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, Overhead), GET_MEMBER_NAME_CHECKED(FMetricStatusColors, Pass), LOCTEXT("Pass2", "Pass"));
            AddNestedColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, Overhead), GET_MEMBER_NAME_CHECKED(FMetricStatusColors, Tight), LOCTEXT("Tight2", "Tight"));
            AddNestedColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, Overhead), GET_MEMBER_NAME_CHECKED(FMetricStatusColors, Fail), LOCTEXT("Fail2", "Fail"));
        }
        {
            IDetailGroup& Group = AddColorGroup(Category, TEXT("GroundColors"), LOCTEXT("GroundColors", "Ground Validation"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, GroundPass), LOCTEXT("GroundPass", "Pass"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, GroundStepFail), LOCTEXT("GroundStepFail", "Step Fail"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, GroundSlopeFail), LOCTEXT("GroundSlopeFail", "Slope Fail"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, GroundMissingFloor), LOCTEXT("GroundMissingFloor", "Missing Floor"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, GroundGrid), LOCTEXT("GroundGrid", "Grid"));
        }
        {
            IDetailGroup& Group = AddColorGroup(Category, TEXT("FitColors"), LOCTEXT("FitColors", "Fit Analysis"));
            AddNestedColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, Fit), GET_MEMBER_NAME_CHECKED(FMetricStatusColors, Pass), LOCTEXT("FitPass", "Pass"));
            AddNestedColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, Fit), GET_MEMBER_NAME_CHECKED(FMetricStatusColors, Tight), LOCTEXT("FitTight", "Tight"));
            AddNestedColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, Fit), GET_MEMBER_NAME_CHECKED(FMetricStatusColors, Fail), LOCTEXT("FitFail", "Fail"));
        }
        {
            IDetailGroup& Group = AddColorGroup(Category, TEXT("ThresholdColors"), LOCTEXT("ThresholdColors", "Threshold Analysis"));
            AddNestedColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, Threshold), GET_MEMBER_NAME_CHECKED(FMetricStatusColors, Pass), LOCTEXT("ThresholdPass", "Pass"));
            AddNestedColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, Threshold), GET_MEMBER_NAME_CHECKED(FMetricStatusColors, Tight), LOCTEXT("ThresholdTight", "Tight"));
            AddNestedColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, Threshold), GET_MEMBER_NAME_CHECKED(FMetricStatusColors, Fail), LOCTEXT("ThresholdFail", "Fail"));
        }
        {
            IDetailGroup& Group = AddColorGroup(Category, TEXT("TurnColors"), LOCTEXT("TurnColors", "Turn Clearance"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, TurnClear), LOCTEXT("TurnClear", "Clear"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, TurnBlocked), LOCTEXT("TurnBlocked", "Blocked"));
        }
        {
            IDetailGroup& Group = AddColorGroup(Category, TEXT("LedgeColors"), LOCTEXT("LedgeColors", "Ledge Analysis"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, LedgeSafe), LOCTEXT("LedgeSafe", "Safe"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, LedgeWarning), LOCTEXT("LedgeWarning", "Warning"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, LedgeFail), LOCTEXT("LedgeFail", "Fail"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, LedgeNone), LOCTEXT("LedgeNone", "None"));
        }
        {
            IDetailGroup& Group = AddColorGroup(Category, TEXT("StoppingColors"), LOCTEXT("StoppingColors", "Stopping Distance"));
            AddNestedColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, Stopping), GET_MEMBER_NAME_CHECKED(FMetricStatusColors, Pass), LOCTEXT("StoppingPass", "Pass"));
            AddNestedColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, Stopping), GET_MEMBER_NAME_CHECKED(FMetricStatusColors, Tight), LOCTEXT("StoppingTight", "Tight"));
            AddNestedColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, Stopping), GET_MEMBER_NAME_CHECKED(FMetricStatusColors, Fail), LOCTEXT("StoppingFail", "Fail"));
        }
        {
            IDetailGroup& Group = AddColorGroup(Category, TEXT("SensorHeightColors"), LOCTEXT("SensorHeightColors", "Sensor Heights"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, SensorPrimary), LOCTEXT("SensorPrimary", "Primary"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, SensorLow), LOCTEXT("SensorLow", "Low"));
        }
        {
            IDetailGroup& Group = AddColorGroup(Category, TEXT("SensorFieldColors"), LOCTEXT("SensorFieldColors", "Sensor Field"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, SensorFieldClear), LOCTEXT("SensorFieldClear", "Clear"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, SensorFieldBlocked), LOCTEXT("SensorFieldBlocked", "Blocked"));
        }
        {
            IDetailGroup& Group = AddColorGroup(Category, TEXT("EngagementColors"), LOCTEXT("EngagementColors", "Engagement Ranges"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, EngagementShort), LOCTEXT("EngagementShort", "Short"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, EngagementMedium), LOCTEXT("EngagementMedium", "Medium"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, EngagementLong), LOCTEXT("EngagementLong", "Long"));
        }
        {
            IDetailGroup& Group = AddColorGroup(Category, TEXT("LocomotionColors"), LOCTEXT("LocomotionColors", "Locomotion Bands"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, LocomotionNear), LOCTEXT("LocomotionNear", "Near"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, LocomotionMid), LOCTEXT("LocomotionMid", "Mid"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, LocomotionFar), LOCTEXT("LocomotionFar", "Far"));
        }
        {
            IDetailGroup& Group = AddColorGroup(Category, TEXT("TrajectoryColors"), LOCTEXT("TrajectoryColors", "Trajectory"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, TrajectoryPath), LOCTEXT("TrajectoryPath", "Path"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, TrajectoryLanding), LOCTEXT("TrajectoryLanding", "Landing"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, TrajectoryTight), LOCTEXT("TrajectoryTight", "Tight"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, TrajectoryBlocked), LOCTEXT("TrajectoryBlocked", "Blocked"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, TrajectoryApex), LOCTEXT("TrajectoryApex", "Apex"));
        }
        {
            IDetailGroup& Group = AddColorGroup(Category, TEXT("CameraProbeColors"), LOCTEXT("CameraProbeColors", "Camera Probe"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, CameraClear), LOCTEXT("CameraClear", "Clear"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, CameraCompressed), LOCTEXT("CameraCompressed", "Compressed"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, CameraTooClose), LOCTEXT("CameraTooClose", "Too Close"));
        }
        {
            IDetailGroup& Group = AddColorGroup(Category, TEXT("SightlineColors"), LOCTEXT("SightlineColors", "Camera Sightline"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, SightlineClear), LOCTEXT("SightlineClear", "Clear"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, SightlineFramingFail), LOCTEXT("SightlineFramingFail", "Framing Fail"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, SightlineOccluded), LOCTEXT("SightlineOccluded", "Occluded"));
        }
        {
            IDetailGroup& Group = AddColorGroup(Category, TEXT("CoverColors"), LOCTEXT("CoverColors", "Cover / Exposure"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, CoverCovered), LOCTEXT("CoverCovered", "Covered"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, CoverExposed), LOCTEXT("CoverExposed", "Exposed"));
        }
        {
            IDetailGroup& Group = AddColorGroup(Category, TEXT("JumpRunwayColors"), LOCTEXT("JumpRunwayColors", "Jump Runway"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, JumpRunwayPass), LOCTEXT("JumpRunwayPass", "Pass"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, JumpRunwayFail), LOCTEXT("JumpRunwayFail", "Fail"));
        }
        {
            IDetailGroup& Group = AddColorGroup(Category, TEXT("InteractionColors"), LOCTEXT("InteractionColors", "Interaction Reach"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, InteractionOpen), LOCTEXT("InteractionOpen", "Open"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, InteractionTarget), LOCTEXT("InteractionTarget", "Target"));
        }
        {
            IDetailGroup& Group = AddColorGroup(Category, TEXT("ShoulderColors"), LOCTEXT("ShoulderColors", "Shoulder Camera Clearance"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, ShoulderClear), LOCTEXT("ShoulderClear", "Clear"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, ShoulderBlocked), LOCTEXT("ShoulderBlocked", "Blocked"));
        }
        {
            IDetailGroup& Group = AddColorGroup(Category, TEXT("CombatDensityColors"), LOCTEXT("CombatDensityColors", "Combat Spacing Density"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, CombatDensityArea), LOCTEXT("CombatDensityArea", "Area"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, CombatDensityPersonal), LOCTEXT("CombatDensityPersonal", "Personal Space"));
        }
        {
            IDetailGroup& Group = AddColorGroup(Category, TEXT("AlternateStanceColors"), LOCTEXT("AlternateStanceColors", "Alternate Stance Fit"));
            AddNestedColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, AlternateStance), GET_MEMBER_NAME_CHECKED(FMetricStatusColors, Pass), LOCTEXT("AlternatePass", "Pass"));
            AddNestedColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, AlternateStance), GET_MEMBER_NAME_CHECKED(FMetricStatusColors, Tight), LOCTEXT("AlternateTight", "Tight"));
            AddNestedColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, AlternateStance), GET_MEMBER_NAME_CHECKED(FMetricStatusColors, Fail), LOCTEXT("AlternateFail", "Fail"));
        }
        {
            IDetailGroup& Group = AddColorGroup(Category, TEXT("MantleColors"), LOCTEXT("MantleColors", "Mantle / Vault Envelope"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, MantlePass), LOCTEXT("MantlePass", "Pass"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, MantleFail), LOCTEXT("MantleFail", "Fail"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, MantleNone), LOCTEXT("MantleNone", "None"));
        }
        {
            IDetailGroup& Group = AddColorGroup(Category, TEXT("RecoveryColors"), LOCTEXT("RecoveryColors", "Recovery / Dodge Space"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, RecoveryClear), LOCTEXT("RecoveryClear", "Clear"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, RecoveryBlocked), LOCTEXT("RecoveryBlocked", "Blocked"));
        }
        {
            IDetailGroup& Group = AddColorGroup(Category, TEXT("CrossfireColors"), LOCTEXT("CrossfireColors", "Crossfire Exposure"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, CrossfireCovered), LOCTEXT("CrossfireCovered", "Covered"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, CrossfireExposed), LOCTEXT("CrossfireExposed", "Exposed"));
        }
        {
            IDetailGroup& Group = AddColorGroup(Category, TEXT("BenchmarkColors"), LOCTEXT("BenchmarkColors", "Benchmark"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, PinnedBenchmark), LOCTEXT("PinnedBenchmark", "Pinned Benchmark"));
        }
        {
            IDetailGroup& Group = AddColorGroup(Category, TEXT("NeutralColors"), LOCTEXT("NeutralColors", "Neutral / Reference"));
            AddColorRow(Group, ThemeHandle, GET_MEMBER_NAME_CHECKED(FMetricVisualizationTheme, Neutral), LOCTEXT("Neutral", "Neutral"));
        }
    }
}

TSharedRef<IDetailCustomization> FMetricProfileDetailsCustomization::MakeInstance(TSharedRef<FMetricProfileEditorState> InState)
{
    return MakeShared<FMetricProfileDetailsCustomization>(InState);
}

FMetricProfileDetailsCustomization::FMetricProfileDetailsCustomization(TSharedRef<FMetricProfileEditorState> InState)
    : State(InState)
{
}

void FMetricProfileDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    for (TFieldIterator<FProperty> It(UMetricPreset::StaticClass(), EFieldIteratorFlags::ExcludeSuper); It; ++It)
    {
        DetailBuilder.HideProperty(It->GetFName());
    }

    IDetailCategoryBuilder& IdentityCategory = DetailBuilder.EditCategory(
        TEXT("Profile"),
        LOCTEXT("ProfileCategory", "Profile"),
        ECategoryPriority::Important);

    TSharedRef<IPropertyHandle> DisplayNameHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UMetricPreset, DisplayName));
    if (DisplayNameHandle->IsValidHandle())
    {
        IdentityCategory.AddProperty(DisplayNameHandle);
    }

    if (State->SelectedProperty.IsNone())
    {
        return;
    }

    TSharedRef<IPropertyHandle> SelectedHandle = DetailBuilder.GetProperty(State->SelectedProperty);
    if (!SelectedHandle->IsValidHandle())
    {
        return;
    }

    IDetailCategoryBuilder& SettingsCategory = DetailBuilder.EditCategory(
        TEXT("Selected Metric"),
        LOCTEXT("SelectedMetricCategory", "Selected Metric"),
        ECategoryPriority::Important);

    if (State->SelectedProperty == GET_MEMBER_NAME_CHECKED(UMetricPreset, VisualTheme))
    {
        MetricProfileDetails::BuildVisualizationColors(SettingsCategory, SelectedHandle);
        return;
    }

    SettingsCategory.AddProperty(SelectedHandle).ShouldAutoExpand(true);
}

#undef LOCTEXT_NAMESPACE
