#include "MetricPreset.h"

#include "UObject/UnrealType.h"

namespace
{
    static FLinearColor WarmVariant(const FLinearColor& In)
    {
        // Preserve relative luminance/state separation while shifting the
        // canonical Heavy profile toward a warmer family.
        FLinearColor Out;
        Out.R = FMath::Clamp(In.R * 1.08f + 0.16f, 0.0f, 1.0f);
        Out.G = FMath::Clamp(In.G * 0.78f + 0.10f, 0.0f, 1.0f);
        Out.B = FMath::Clamp(In.B * 0.58f + 0.04f, 0.0f, 1.0f);
        Out.A = In.A;
        return Out;
    }

    static void TransformColorsRecursive(UScriptStruct* StructType, void* StructData)
    {
        if (!StructType || !StructData)
        {
            return;
        }

        for (TFieldIterator<FProperty> It(StructType); It; ++It)
        {
            FProperty* Property = *It;
            if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
            {
                void* ValuePtr = StructProperty->ContainerPtrToValuePtr<void>(StructData);
                if (StructProperty->Struct == TBaseStructure<FLinearColor>::Get())
                {
                    FLinearColor* Color = static_cast<FLinearColor*>(ValuePtr);
                    *Color = WarmVariant(*Color);
                }
                else
                {
                    TransformColorsRecursive(StructProperty->Struct, ValuePtr);
                }
            }
        }
    }
}

void UMetricPreset::PostLoad()
{
    Super::PostLoad();

    if (VisualizationThemeVersion >= 2)
    {
        return;
    }

    // 0.23.0 granularized Profile Colors so every visualization/HUD semantic
    // has its own editable color. Existing assets inherit the new defaults.
    // The canonical Heavy starter receives the same semantic palette shifted
    // into a warmer family so Human and Heavy remain visually distinct.
    const bool bCanonicalHeavy = GetName().Equals(TEXT("DA_Metric_Heavy"), ESearchCase::IgnoreCase)
        || DisplayName.ToString().Equals(TEXT("Heavy Reference"), ESearchCase::IgnoreCase);

    if (bCanonicalHeavy)
    {
        TransformColorsRecursive(FMetricVisualizationTheme::StaticStruct(), &VisualTheme);
    }

    VisualizationThemeVersion = 2;
}
