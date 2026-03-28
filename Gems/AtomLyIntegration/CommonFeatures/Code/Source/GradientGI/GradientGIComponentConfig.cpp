/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomLyIntegration/CommonFeatures/GradientGI/GradientGIComponentConfig.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Render
    {
        void GradientGIComponentConfig::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<GradientGIComponentConfig, ComponentConfig>()
                    ->Version(2)
                    ->Field("HighColor",     &GradientGIComponentConfig::m_highColor)
                    ->Field("MidColor",      &GradientGIComponentConfig::m_midColor)
                    ->Field("LowColor",      &GradientGIComponentConfig::m_lowColor)
                    ->Field("Exposure",      &GradientGIComponentConfig::m_exposure)
                    ->Field("FaceResolution",&GradientGIComponentConfig::m_faceResolution)
                    ->Field("UpdateMode",    &GradientGIComponentConfig::m_updateMode)
                    ;
            }
        }

    } // namespace Render
} // namespace AZ
