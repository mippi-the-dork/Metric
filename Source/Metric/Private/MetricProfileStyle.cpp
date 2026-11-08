#include "MetricProfileStyle.h"

#include "UObject/UnrealType.h"

namespace
{
    FLinearColor WarmVariant(const FLinearColor& In)
    {
        FLinearColor Out;
        Out.R = FMath::Clamp(In.R * 1.08f + 0.16f, 0.0f, 1.0f);
        Out.G = FMath::Clamp(In.G * 0.78f + 0.10f, 0.0f, 1.0f);
        Out.B = FMath::Clamp(In.B * 0.58f + 0.04f, 0.0f, 1.0f);
        Out.A = In.A;
        return Out;
    }

    void TransformColorsRecursive(UScriptStruct* StructType, void* StructData)
    {
        if (!StructType || !StructData)
        {
            return;
        }

        for (TFieldIterator<FProperty> It(StructType); It; ++It)
        {
            if (FStructProperty* StructProperty = CastField<FStructProperty>(*It))
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

void MetricProfileStyle::ApplyWarmTheme(FMetricVisualizationTheme& Theme)
{
    TransformColorsRecursive(FMetricVisualizationTheme::StaticStruct(), &Theme);
}
