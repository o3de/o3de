/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/Material/ConvertEmissiveUnitFunctor.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace Render
    {
        void ConvertEmissiveUnitFunctor::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ConvertEmissiveUnitFunctor, AZ::RPI::MaterialFunctor>()
                    ->Version(6)
                    ->Field("intensityPropertyIndex", &ConvertEmissiveUnitFunctor::m_intensityPropertyIndex)
                    ->Field("lightUnitPropertyIndex", &ConvertEmissiveUnitFunctor::m_lightUnitPropertyIndex)
                    ->Field("shaderInputIndex", &ConvertEmissiveUnitFunctor::m_shaderInputIndex)
                    ->Field("ev100Index", &ConvertEmissiveUnitFunctor::m_ev100Index)
                    ->Field("nitIndex", &ConvertEmissiveUnitFunctor::m_nitIndex)
                    ->Field("ev100Min", &ConvertEmissiveUnitFunctor::m_ev100Min)
                    ->Field("ev100Max", &ConvertEmissiveUnitFunctor::m_ev100Max)
                    ->Field("nitMin", &ConvertEmissiveUnitFunctor::m_nitMin)
                    ->Field("nitMax", &ConvertEmissiveUnitFunctor::m_nitMax)
                    ;
            }
        }

        void ConvertEmissiveUnitFunctor::Process(RPI::MaterialFunctorAPI::RuntimeContext& context)
        {
            // Convert unit on runtime
            float sourceValue = context.GetMaterialPropertyValue<float>(m_intensityPropertyIndex);
            uint32_t lightUnit = context.GetMaterialPropertyValue<uint32_t>(m_lightUnitPropertyIndex);
            float targetValue = GetProcessedValue(sourceValue, lightUnit);
            context.GetShaderResourceGroup()->SetConstant(m_shaderInputIndex, targetValue);
        }

        void ConvertEmissiveUnitFunctor::Process(RPI::MaterialFunctorAPI::EditorContext& context)
        {
            // Update Editor based on selected light unit
            uint32_t lightUnit = context.GetMaterialPropertyValue<uint32_t>(m_lightUnitPropertyIndex);

            if (lightUnit == m_ev100Index)
            {
                context.SetMaterialPropertyMinValue(m_intensityPropertyIndex, m_ev100Min);
                context.SetMaterialPropertyMaxValue(m_intensityPropertyIndex, m_ev100Max);
            }
            else if (lightUnit == m_nitIndex)
            {
                context.SetMaterialPropertyMinValue(m_intensityPropertyIndex, m_nitMin);
                context.SetMaterialPropertyMaxValue(m_intensityPropertyIndex, m_nitMax);
            }
            else
            {
                AZ_Assert(false, "ConvertEmissiveUnitFunctor doesn't recognize light unit.")
            }
        }

        float ConvertEmissiveUnitFunctor::GetProcessedValue(float originalEmissiveIntensity, uint32_t lightUnitIndex) const
        {
            PhotometricUnit sourceType;
            if (lightUnitIndex == m_ev100Index)
            {
                sourceType = PhotometricUnit::Ev100Luminance;
            }
            else if (lightUnitIndex == m_nitIndex)
            {
                sourceType = PhotometricUnit::Nit;
            }
            else
            {
                sourceType = PhotometricUnit::Unknown;
                AZ_Assert(false, "ConvertEmissiveUnitFunctor doesn't recognize light unit.")
            }

            return PhotometricValue::ConvertIntensityBetweenUnits(sourceType, PhotometricUnit::Nit, originalEmissiveIntensity);
        }
    }
}
