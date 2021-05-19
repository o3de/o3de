/**
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */


#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/Feature/DisplayMapper/DisplayMapperConfigurationDescriptor.h>
#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        void AcesParameterOverrides::Reflect(ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<AcesParameterOverrides>()
                    ->Version(0)
                    ->Field("OverrideDefaults", &AcesParameterOverrides::m_overrideDefaults)
                    ->Field("AlterSurround", &AcesParameterOverrides::m_alterSurround)
                    ->Field("ApplyDesaturation", &AcesParameterOverrides::m_applyDesaturation)
                    ->Field("ApplyCATD60toD65", &AcesParameterOverrides::m_applyCATD60toD65)
                    ->Field("PresetODT", &AcesParameterOverrides::m_preset)
                    ->Field("CinemaLimitsBlack", &AcesParameterOverrides::m_cinemaLimitsBlack)
                    ->Field("CinemaLimitsWhite", &AcesParameterOverrides::m_cinemaLimitsWhite)
                    ->Field("MinPoint", &AcesParameterOverrides::m_minPoint)
                    ->Field("MidPoint", &AcesParameterOverrides::m_midPoint)
                    ->Field("MaxPoint", &AcesParameterOverrides::m_maxPoint)
                    ->Field("SurroundGamma", &AcesParameterOverrides::m_surroundGamma)
                    ->Field("Gamma", &AcesParameterOverrides::m_gamma);
            }
        }

        void AcesParameterOverrides::LoadPreset()
        {
            DisplayMapperParameters displayMapperParameters;
            AcesDisplayMapperFeatureProcessor::GetAcesDisplayMapperParameters(&displayMapperParameters, m_preset);

            m_alterSurround = (displayMapperParameters.m_OutputDisplayTransformFlags & 0x1) != 0;
            m_applyDesaturation = (displayMapperParameters.m_OutputDisplayTransformFlags & 0x2) != 0;
            m_applyCATD60toD65 = (displayMapperParameters.m_OutputDisplayTransformFlags & 0x4) != 0;
            m_cinemaLimitsBlack = displayMapperParameters.m_cinemaLimits[0];
            m_cinemaLimitsWhite = displayMapperParameters.m_cinemaLimits[1];
            m_minPoint = displayMapperParameters.m_acesSplineParams.minPoint[0];
            m_midPoint = displayMapperParameters.m_acesSplineParams.midPoint[0];
            m_maxPoint = displayMapperParameters.m_acesSplineParams.maxPoint[0];
            m_surroundGamma = displayMapperParameters.m_surroundGamma;
            m_gamma = displayMapperParameters.m_gamma;
        }

        void DisplayMapperConfigurationDescriptor::Reflect(AZ::ReflectContext* context)
        {
            AcesParameterOverrides::Reflect(context);

            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Enum<DisplayMapperOperationType>()
                    ->Value("Aces", DisplayMapperOperationType::Aces)
                    ->Value("AcesLut", DisplayMapperOperationType::AcesLut)
                    ->Value("Passthrough", DisplayMapperOperationType::Passthrough)
                    ->Value("GammaSRGB", DisplayMapperOperationType::GammaSRGB)
                    ->Value("Reinhard", DisplayMapperOperationType::Reinhard)
                    ->Value("Invalid", DisplayMapperOperationType::Invalid)
                    ;
                
                serializeContext->Class<DisplayMapperConfigurationDescriptor>()
                    ->Version(1)
                    ->Field("Name", &DisplayMapperConfigurationDescriptor::m_name)
                    ->Field("OperationType", &DisplayMapperConfigurationDescriptor::m_operationType)
                    ->Field("LdrGradingLutEnabled", &DisplayMapperConfigurationDescriptor::m_ldrGradingLutEnabled)
                    ->Field("LdrColorGradingLut", &DisplayMapperConfigurationDescriptor::m_ldrColorGradingLut)
                ;
            }
        }

        void DisplayMapperPassData::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<DisplayMapperPassData, Base>()
                    ->Version(0)
                    ->Field("DisplayMapperConfig", &DisplayMapperPassData::m_config)
                    ;
            }
        }

    } // namespace RPI
} // namespace AZ
