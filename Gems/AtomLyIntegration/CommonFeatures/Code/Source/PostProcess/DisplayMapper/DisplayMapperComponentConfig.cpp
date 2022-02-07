/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomLyIntegration/CommonFeatures/PostProcess/DisplayMapper/DisplayMapperComponentConfig.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AZ
{
    namespace Render
    {
        void DisplayMapperComponentConfig::Reflect(ReflectContext* context)
        {
            AcesParameterOverrides::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<DisplayMapperComponentConfig, ComponentConfig>()
                    ->Version(1)
                    ->Field("DisplayMapperOperationType", &DisplayMapperComponentConfig::m_displayMapperOperation)
                    ->Field("LdrColorGradingLutEnabled", &DisplayMapperComponentConfig::m_ldrColorGradingLutEnabled)
                    ->Field("LdrColorGradingLut", &DisplayMapperComponentConfig::m_ldrColorGradingLut)
                    ->Field("AcesParameterOverrides", &DisplayMapperComponentConfig::m_acesParameterOverrides)
                    ;
            }
        }
    } // namespace Render
} // namespace AZ
