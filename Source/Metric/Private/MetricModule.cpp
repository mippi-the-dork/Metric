#include "MetricModule.h"

#include "MetricCommands.h"
#include "MetricInputProcessor.h"
#include "MetricActorOverview.h"
#include "MetricActor.h"
#include "MetricActorDetailsCustomization.h"
#include "MetricLog.h"
#include "MetricStyle.h"
#include "MetricViewportController.h"
#include "Framework/Application/SlateApplication.h"
#include "Components/ActorComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/UObjectGlobals.h"
#include "ToolMenus.h"
#include "PropertyEditorModule.h"

#define LOCTEXT_NAMESPACE "FMetricModule"

void FMetricModule::StartupModule()
{
    FMetricStyle::Initialize();
    FMetricCommands::Register();
    ViewportController = MakeUnique<FMetricViewportController>();
    MetricActorOverview::Register();

    FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
    PropertyEditorModule.RegisterCustomClassLayout(TEXT("MetricActor"), FOnGetDetailCustomizationInstance::CreateStatic(&FMetricActorDetailsCustomization::MakeInstance));
    PropertyEditorModule.NotifyCustomizationModuleChanged();

    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FMetricModule::RegisterMenus));

    if (GEngine)
    {
        GEngine->OnLevelActorAdded().AddRaw(this, &FMetricModule::HandleLevelActorChanged);
        GEngine->OnLevelActorDeleted().AddRaw(this, &FMetricModule::HandleLevelActorChanged);
        GEngine->OnActorMoved().AddRaw(this, &FMetricModule::HandleLevelActorChanged);
    }
    FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &FMetricModule::HandleObjectPropertyChanged);

    if (FSlateApplication::IsInitialized())
    {
        InputProcessor = MakeShared<FMetricInputProcessor>();
        // Index 0 gives Metric first opportunity to consume the two legacy
        // preview-mesh chords while the Level Editor viewport has focus.
        FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor, 0);
        UE_LOG(LogMetric, Log, TEXT("Metric 0.17.1 input processor registered."));
    }
}


void FMetricModule::RegisterMenus()
{
    FToolMenuOwnerScoped OwnerScoped(this);
    if (UToolMenu* Menu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools")))
    {
        FToolMenuSection& Section = Menu->FindOrAddSection(TEXT("Metric"), LOCTEXT("MetricToolsSection", "Metric"), FToolMenuInsert());
        Section.AddMenuEntry(
            TEXT("MetricDashboard"),
            LOCTEXT("MetricDashboardMenu", "Metric Dashboard"),
            LOCTEXT("MetricDashboardMenuTooltip", "Review persistent Metric Actor validation across the current level."),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateStatic(&MetricActorOverview::Open)));
    }
}

void FMetricModule::HandleLevelActorChanged(AActor* Actor)
{
    if (!Actor || Actor->IsA<AMetricActor>())
    {
        return;
    }

    RequestMetricActorUpdates(Actor->GetWorld(), Actor);
}

void FMetricModule::HandleObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent)
{
    if (!Object)
    {
        return;
    }

    AActor* SourceActor = Cast<AActor>(Object);
    if (!SourceActor)
    {
        if (UActorComponent* Component = Cast<UActorComponent>(Object))
        {
            SourceActor = Component->GetOwner();
        }
    }

    if (!SourceActor || SourceActor->IsA<AMetricActor>())
    {
        return;
    }

    RequestMetricActorUpdates(SourceActor->GetWorld(), SourceActor);
}

void FMetricModule::RequestMetricActorUpdates(UWorld* World, const AActor* SourceActor)
{
    if (!World || (World->WorldType != EWorldType::Editor && World->WorldType != EWorldType::EditorPreview))
    {
        return;
    }

    for (TActorIterator<AMetricActor> It(World); It; ++It)
    {
        AMetricActor* MetricActor = *It;
        if (MetricActor && MetricActor != SourceActor)
        {
            MetricActor->RequestRelevantUpdate();
        }
    }
}

void FMetricModule::ShutdownModule()
{
    if (InputProcessor.IsValid() && FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
    }

    InputProcessor.Reset();
    ViewportController.Reset();

    if (GEngine)
    {
        GEngine->OnLevelActorAdded().RemoveAll(this);
        GEngine->OnLevelActorDeleted().RemoveAll(this);
        GEngine->OnActorMoved().RemoveAll(this);
    }
    FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
    if (FModuleManager::Get().IsModuleLoaded(TEXT("PropertyEditor")))
    {
        FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
        PropertyEditorModule.UnregisterCustomClassLayout(TEXT("MetricActor"));
        PropertyEditorModule.NotifyCustomizationModuleChanged();
    }

    MetricActorOverview::Unregister();
    FMetricCommands::Unregister();
    FMetricStyle::Shutdown();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMetricModule, Metric)
