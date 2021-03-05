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

namespace AZ
{
    namespace Render
    {
        void DisplayMapperConfigurationDescriptor::Reflect(AZ::ReflectContext* context)
        {
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
