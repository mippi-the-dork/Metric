#include "MetricActorOverview.h"

#include "MetricActor.h"
#include "MetricProfile.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "MetricActorOverview"

namespace
{
    class SMetricActorOverview;
    TWeakObjectPtr<AMetricActor> RequestedActor;
    TWeakPtr<SMetricActorOverview> ActiveOverview;
    enum class EMetricOverviewFilter : uint8
    {
        All,
        Failed,
        Warning,
        Passing,
        Disabled,
        MissingProfile
    };

    struct FMetricActorRowData
    {
        TWeakObjectPtr<AMetricActor> Actor;
    };

    FText GetMetricProfileDisplayName(const UMetricProfile* Profile)
    {
        if (!Profile)
        {
            return LOCTEXT("NoProfile", "None");
        }
        return Profile->DisplayName.IsEmpty() ? FText::FromString(Profile->GetName()) : Profile->DisplayName;
    }

    class SMetricActorOverviewRow : public SMultiColumnTableRow<TSharedPtr<FMetricActorRowData>>
    {
    public:
        SLATE_BEGIN_ARGS(SMetricActorOverviewRow) {}
            SLATE_ARGUMENT(TSharedPtr<FMetricActorRowData>, Item)
        SLATE_END_ARGS()

        void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& Owner)
        {
            Item = InArgs._Item;
            SMultiColumnTableRow<TSharedPtr<FMetricActorRowData>>::Construct(
                SMultiColumnTableRow<TSharedPtr<FMetricActorRowData>>::FArguments().Padding(2.0f), Owner);
        }

        virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
        {
            AMetricActor* Actor = Item.IsValid() ? Item->Actor.Get() : nullptr;
            if (!Actor) return SNew(STextBlock).Text(LOCTEXT("InvalidActor", "Invalid"));

            if (ColumnName == "Actor") return SNew(STextBlock).Text(FText::FromString(Actor->GetActorLabel()));
            if (ColumnName == "Enabled") return SNew(STextBlock).Text(Actor->bEnabled ? LOCTEXT("Yes", "Yes") : LOCTEXT("No", "No"));
            if (ColumnName == "Profile") return SNew(STextBlock).Text(GetMetricProfileDisplayName(Actor->Profile));
            if (ColumnName == "Result") return SNew(STextBlock).Text(FText::FromString(Actor->GetOverallResultText()));
            if (ColumnName == "Environment")
            {
                const TCHAR* Name = Actor->EnvironmentSource == EMetricEnvironmentSource::Collision ? TEXT("Collision")
                    : (Actor->EnvironmentSource == EMetricEnvironmentSource::Hybrid ? TEXT("Hybrid") : TEXT("Visual Geometry"));
                return SNew(STextBlock).Text(FText::FromString(Name));
            }
            if (ColumnName == "Frame")
            {
                const TCHAR* Name = Actor->AnalysisFrame == EMetricAnalysisFrameMode::World ? TEXT("World Up")
                    : (Actor->AnalysisFrame == EMetricAnalysisFrameMode::SurfaceNormal ? TEXT("Surface Normal")
                    : (Actor->AnalysisFrame == EMetricAnalysisFrameMode::Provider ? TEXT("Gravity / Provider") : TEXT("Custom Up")));
                return SNew(STextBlock).Text(FText::FromString(Name));
            }
            if (ColumnName == "Level") return SNew(STextBlock).Text(FText::FromString(Actor->GetLevel() ? Actor->GetLevel()->GetOuter()->GetName() : TEXT("None")));
            if (ColumnName == "Issues") return SNew(STextBlock).Text(FText::AsNumber(Actor->IssueCount));
            return SNew(STextBlock).Text(FText::GetEmpty());
        }

    private:
        TSharedPtr<FMetricActorRowData> Item;
    };

    class SMetricActorOverview : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SMetricActorOverview) {}
        SLATE_END_ARGS()

        void Construct(const FArguments&)
        {
            Refresh();
            ChildSlot
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(8, 8, 8, 4)
                [
                    SAssignNew(SummaryText, STextBlock)
                    .Text(this, &SMetricActorOverview::GetSummaryText)
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(8, 4)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().FillWidth(1.0f)
                    [
                        SNew(SSearchBox)
                        .HintText(LOCTEXT("SearchHint", "Search actor, profile, level..."))
                        .ToolTipText(LOCTEXT("SearchTip", "Filter Metric Actors by Actor label, Metric Profile asset/name, or level."))
                        .OnTextChanged(this, &SMetricActorOverview::OnSearchChanged)
                    ]
                    + SHorizontalBox::Slot().AutoWidth().Padding(6,0,0,0)
                    [
                        SNew(SComboButton)
                        .ButtonContent()[SNew(STextBlock).Text(this, &SMetricActorOverview::GetProfileFilterText)]
                        .ToolTipText(LOCTEXT("ProfileFilterTip", "Limit the Dashboard to Metric Actors using one Metric Profile."))
                        .OnGetMenuContent(this, &SMetricActorOverview::BuildProfileFilterMenu)
                    ]
                    + SHorizontalBox::Slot().AutoWidth().Padding(6,0,0,0)
                    [
                        SNew(SButton).Text(LOCTEXT("Refresh", "Refresh")).ToolTipText(LOCTEXT("RefreshTip", "Rescan the current editor world and refresh Metric Actor results shown in the Dashboard.")).OnClicked(this, &SMetricActorOverview::OnRefresh)
                    ]
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(8, 2, 8, 6)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth()[MakeFilterButton(LOCTEXT("All", "All"), EMetricOverviewFilter::All)]
                    + SHorizontalBox::Slot().AutoWidth().Padding(4,0)[MakeFilterButton(LOCTEXT("Failed", "Failed"), EMetricOverviewFilter::Failed)]
                    + SHorizontalBox::Slot().AutoWidth()[MakeFilterButton(LOCTEXT("Warning", "Tight / Warning"), EMetricOverviewFilter::Warning)]
                    + SHorizontalBox::Slot().AutoWidth().Padding(4,0)[MakeFilterButton(LOCTEXT("Passing", "Passing"), EMetricOverviewFilter::Passing)]
                    + SHorizontalBox::Slot().AutoWidth()[MakeFilterButton(LOCTEXT("Disabled", "Disabled"), EMetricOverviewFilter::Disabled)]
                    + SHorizontalBox::Slot().AutoWidth().Padding(4,0)[MakeFilterButton(LOCTEXT("Missing", "Missing Profile"), EMetricOverviewFilter::MissingProfile)]
                ]
                + SVerticalBox::Slot().FillHeight(1.0f).Padding(8)
                [
                    SAssignNew(ListView, SListView<TSharedPtr<FMetricActorRowData>>)
                    .ListItemsSource(&FilteredRows)
                    .SelectionMode(ESelectionMode::Single)
                    .OnGenerateRow(this, &SMetricActorOverview::GenerateRow)
                    .OnSelectionChanged(this, &SMetricActorOverview::OnSelectionChanged)
                    .OnMouseButtonDoubleClick(this, &SMetricActorOverview::OnDoubleClick)
                    .HeaderRow
                    (
                        SNew(SHeaderRow)
                        + SHeaderRow::Column("Actor").DefaultLabel(LOCTEXT("ActorColumn", "Actor")).FillWidth(0.28f)
                        + SHeaderRow::Column("Enabled").DefaultLabel(LOCTEXT("EnabledColumn", "Enabled")).FillWidth(0.10f)
                        + SHeaderRow::Column("Profile").DefaultLabel(LOCTEXT("ProfileColumn", "Profile")).FillWidth(0.22f)
                        + SHeaderRow::Column("Result").DefaultLabel(LOCTEXT("ResultColumn", "Overall Result")).FillWidth(0.15f)
                        + SHeaderRow::Column("Environment").DefaultLabel(LOCTEXT("EnvironmentColumn", "Environment Source")).FillWidth(0.15f)
                        + SHeaderRow::Column("Frame").DefaultLabel(LOCTEXT("FrameColumn", "Analysis Frame")).FillWidth(0.13f)
                        + SHeaderRow::Column("Level").DefaultLabel(LOCTEXT("LevelColumn", "Level")).FillWidth(0.15f)
                        + SHeaderRow::Column("Issues").DefaultLabel(LOCTEXT("IssuesColumn", "Issues")).FixedWidth(64.0f)
                    )
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
                [
                    SNew(SBorder)
                    .Padding(8.0f)
                    [
                        SNew(STextBlock)
                        .Text(this, &SMetricActorOverview::GetSelectedBreakdownText)
                        .AutoWrapText(true)
                    ]
                ]
            ];

            if (RequestedActor.IsValid() && ListView.IsValid())
            {
                for (const TSharedPtr<FMetricActorRowData>& Row : FilteredRows)
                {
                    if (Row.IsValid() && Row->Actor.Get() == RequestedActor.Get())
                    {
                        SelectedActor = RequestedActor;
                        ListView->SetSelection(Row, ESelectInfo::Direct);
                        ListView->RequestScrollIntoView(Row);
                        break;
                    }
                }
                RequestedActor.Reset();
            }
        }

        void FocusActor(AMetricActor* Actor)
        {
            if (!Actor || !ListView.IsValid())
            {
                return;
            }

            Refresh();
            for (const TSharedPtr<FMetricActorRowData>& Row : FilteredRows)
            {
                if (Row.IsValid() && Row->Actor.Get() == Actor)
                {
                    SelectedActor = Actor;
                    ListView->SetSelection(Row, ESelectInfo::Direct);
                    ListView->RequestScrollIntoView(Row);
                    return;
                }
            }
        }

    private:
        TArray<TSharedPtr<FMetricActorRowData>> AllRows;
        TArray<TSharedPtr<FMetricActorRowData>> FilteredRows;
        TSharedPtr<SListView<TSharedPtr<FMetricActorRowData>>> ListView;
        TSharedPtr<STextBlock> SummaryText;
        FString SearchText;
        EMetricOverviewFilter Filter = EMetricOverviewFilter::All;
        TWeakObjectPtr<UMetricProfile> ProfileFilter;
        TWeakObjectPtr<AMetricActor> SelectedActor;

        TSharedRef<SWidget> MakeFilterButton(const FText& Label, EMetricOverviewFilter InFilter)
        {
            return SNew(SButton)
                .Text(Label)
                .ToolTipText(FText::Format(LOCTEXT("ResultFilterTip", "Show Metric Actors matching the {0} result filter."), Label))
                .OnClicked_Lambda([this, InFilter]()
                {
                    Filter = InFilter;
                    ApplyFilter();
                    return FReply::Handled();
                });
        }

        void Refresh()
        {
            AllRows.Reset();
            if (GEditor)
            {
                if (UWorld* World = GEditor->GetEditorWorldContext().World())
                {
                    for (TActorIterator<AMetricActor> It(World); It; ++It)
                    {
                        TSharedPtr<FMetricActorRowData> Row = MakeShared<FMetricActorRowData>();
                        Row->Actor = *It;
                        AllRows.Add(Row);
                    }
                }
            }
            AllRows.Sort([](const auto& A, const auto& B)
            {
                const AMetricActor* AA = A.IsValid() ? A->Actor.Get() : nullptr;
                const AMetricActor* BB = B.IsValid() ? B->Actor.Get() : nullptr;
                return AA && BB ? AA->GetActorLabel() < BB->GetActorLabel() : AA != nullptr;
            });
            ApplyFilter();
        }

        void ApplyFilter()
        {
            FilteredRows.Reset();
            for (const TSharedPtr<FMetricActorRowData>& Row : AllRows)
            {
                AMetricActor* Actor = Row.IsValid() ? Row->Actor.Get() : nullptr;
                if (!Actor) continue;

                bool bFilterPass = true;
                switch (Filter)
                {
                case EMetricOverviewFilter::Failed: bFilterPass = Actor->OverallResult == EMetricActorOverallResult::Fail; break;
                case EMetricOverviewFilter::Warning: bFilterPass = Actor->OverallResult == EMetricActorOverallResult::Tight; break;
                case EMetricOverviewFilter::Passing: bFilterPass = Actor->OverallResult == EMetricActorOverallResult::Pass; break;
                case EMetricOverviewFilter::Disabled: bFilterPass = Actor->OverallResult == EMetricActorOverallResult::Disabled; break;
                case EMetricOverviewFilter::MissingProfile: bFilterPass = Actor->OverallResult == EMetricActorOverallResult::MissingProfile; break;
                case EMetricOverviewFilter::All: default: break;
                }
                if (!bFilterPass) continue;

                if (ProfileFilter.IsValid() && Actor->Profile != ProfileFilter.Get()) continue;

                if (!SearchText.IsEmpty())
                {
                    const FString ProfileLabel = Actor->Profile ? GetMetricProfileDisplayName(Actor->Profile).ToString() : FString();
                    const FString ProfileAssetName = Actor->Profile ? Actor->Profile->GetName() : FString();
                    const FString Haystack = FString::Printf(TEXT("%s %s %s %s"), *Actor->GetActorLabel(),
                        *ProfileLabel, *ProfileAssetName,
                        Actor->GetLevel() ? *Actor->GetLevel()->GetOuter()->GetName() : TEXT(""));
                    if (!Haystack.Contains(SearchText, ESearchCase::IgnoreCase)) continue;
                }
                FilteredRows.Add(Row);
            }
            if (ListView.IsValid()) ListView->RequestListRefresh();
        }

        FText GetSummaryText() const
        {
            int32 Pass = 0, Warn = 0, Fail = 0, Disabled = 0, Missing = 0;
            for (const auto& Row : AllRows)
            {
                if (const AMetricActor* Actor = Row.IsValid() ? Row->Actor.Get() : nullptr)
                {
                    switch (Actor->OverallResult)
                    {
                    case EMetricActorOverallResult::Pass: ++Pass; break;
                    case EMetricActorOverallResult::Tight: ++Warn; break;
                    case EMetricActorOverallResult::Fail: ++Fail; break;
                    case EMetricActorOverallResult::Disabled: ++Disabled; break;
                    case EMetricActorOverallResult::MissingProfile: ++Missing; break;
                    }
                }
            }
            return FText::FromString(FString::Printf(TEXT("Metric Actors: %d | %d Pass | %d Tight | %d Fail | %d Disabled | %d Missing Profile"),
                AllRows.Num(), Pass, Warn, Fail, Disabled, Missing));
        }

        FText GetProfileFilterText() const
        {
            return ProfileFilter.IsValid()
                ? FText::Format(LOCTEXT("ProfileFilterActive", "Profile: {0}"), GetMetricProfileDisplayName(ProfileFilter.Get()))
                : LOCTEXT("AllProfiles", "Profile: All");
        }

        TSharedRef<SWidget> BuildProfileFilterMenu()
        {
            FMenuBuilder MenuBuilder(true, nullptr);
            MenuBuilder.AddMenuEntry(LOCTEXT("ProfileAll", "All Profiles"), FText::GetEmpty(), FSlateIcon(),
                FUIAction(FExecuteAction::CreateLambda([this](){ ProfileFilter.Reset(); ApplyFilter(); })));

            TSet<UMetricProfile*> Profiles;
            for (const auto& Row : AllRows)
            {
                if (AMetricActor* Actor = Row.IsValid() ? Row->Actor.Get() : nullptr)
                {
                    if (Actor->Profile) Profiles.Add(Actor->Profile);
                }
            }
            TArray<UMetricProfile*> Sorted = Profiles.Array();
            Sorted.Sort([](const UMetricProfile& A, const UMetricProfile& B)
            {
                return GetMetricProfileDisplayName(&A).ToString() < GetMetricProfileDisplayName(&B).ToString();
            });
            for (UMetricProfile* Profile : Sorted)
            {
                MenuBuilder.AddMenuEntry(GetMetricProfileDisplayName(Profile), FText::FromString(Profile->GetPathName()), FSlateIcon(),
                    FUIAction(FExecuteAction::CreateLambda([this, Profile](){ ProfileFilter = Profile; ApplyFilter(); })));
            }
            return MenuBuilder.MakeWidget();
        }

        FText GetSelectedBreakdownText() const
        {
            const AMetricActor* Actor = SelectedActor.Get();
            if (!Actor)
            {
                if (AllRows.IsEmpty())
                {
                    return LOCTEXT("NoMetricActors", "No Metric Actors are present in the current editor world. Place a Metric Actor to add persistent spatial validation to the level.");
                }
                if (FilteredRows.IsEmpty())
                {
                    return LOCTEXT("NoMatchingMetricActors", "No Metric Actors match the current search and filters.");
                }
                return LOCTEXT("SelectForBreakdown", "Select a Metric Actor to see its analysis breakdown. Double-click a row to frame that Actor in the viewport.");
            }

            FString Text = FString::Printf(TEXT("%s | %s\n%s"), *Actor->GetActorLabel(), *Actor->GetOverallResultText(), *Actor->ResultSummary);
            int32 Added = 0;
            for (const FMetricQueryEvidence& Evidence : Actor->GetEvaluatedState().QueryEvidence)
            {
                if (Evidence.Status == EMetricQueryEvidenceStatus::Fail || Evidence.Status == EMetricQueryEvidenceStatus::Warning)
                {
                    Text += FString::Printf(TEXT("\n%s %s"), Evidence.Status == EMetricQueryEvidenceStatus::Fail ? TEXT("FAIL:") : TEXT("WARN:"), *Evidence.Label);
                    if (++Added >= 8) break;
                }
            }
            if (Added == 0 && Actor->OverallResult == EMetricActorOverallResult::Pass)
            {
                Text += TEXT("\nNo active analysis warnings or failures.");
            }
            return FText::FromString(Text);
        }

        void OnSearchChanged(const FText& Text) { SearchText = Text.ToString(); ApplyFilter(); }
        FReply OnRefresh() { Refresh(); return FReply::Handled(); }

        TSharedRef<ITableRow> GenerateRow(TSharedPtr<FMetricActorRowData> Item, const TSharedRef<STableViewBase>& Owner)
        {
            return SNew(SMetricActorOverviewRow, Owner).Item(Item);
        }

        void OnSelectionChanged(TSharedPtr<FMetricActorRowData> Item, ESelectInfo::Type)
        {
            if (!GEditor || !Item.IsValid() || !Item->Actor.IsValid()) return;
            SelectedActor = Item->Actor;
            GEditor->SelectNone(false, true, false);
            GEditor->SelectActor(Item->Actor.Get(), true, true, true);
        }

        void OnDoubleClick(TSharedPtr<FMetricActorRowData> Item)
        {
            if (!GEditor || !Item.IsValid() || !Item->Actor.IsValid()) return;
            TArray<AActor*> Actors{Item->Actor.Get()};
            GEditor->MoveViewportCamerasToActor(Actors, false);
        }
    };
}

const FName MetricActorOverview::TabName(TEXT("MetricDashboard"));

void MetricActorOverview::Register()
{
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TabName,
        FOnSpawnTab::CreateLambda([](const FSpawnTabArgs&)
        {
            TSharedRef<SMetricActorOverview> Overview = SNew(SMetricActorOverview);
            ActiveOverview = Overview;
            return SNew(SDockTab).TabRole(ETabRole::NomadTab)[Overview];
        }))
        .SetDisplayName(LOCTEXT("TabTitle", "Metric Dashboard"))
        .SetTooltipText(LOCTEXT("TabTooltip", "Review persistent Metric Actors and their current validation results."));
}

void MetricActorOverview::Unregister()
{
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabName);
}

void MetricActorOverview::Open()
{
    FGlobalTabmanager::Get()->TryInvokeTab(TabName);
}

void MetricActorOverview::OpenAndFocusActor(AMetricActor* Actor)
{
    RequestedActor = Actor;
    FGlobalTabmanager::Get()->TryInvokeTab(TabName);

    if (TSharedPtr<SMetricActorOverview> Overview = ActiveOverview.Pin())
    {
        Overview->FocusActor(Actor);
        RequestedActor.Reset();
    }
}

#undef LOCTEXT_NAMESPACE
