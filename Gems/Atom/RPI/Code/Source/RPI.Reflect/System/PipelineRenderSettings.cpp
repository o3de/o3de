/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RPI.Reflect/System/PipelineRenderSettings.h>

namespace AZ
{
    namespace RPI
    {
        void PipelineRenderSettings::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PipelineRenderSettings>()
                    ->Version(0)
                    ->Field("Size", &PipelineRenderSettings::m_size)
                    ->Field("Format", &PipelineRenderSettings::m_format)
                    ->Field("MultisampleState", &PipelineRenderSettings::m_multisampleState)
                    ;
            }
        }
    } // namespace RPI
} // namespace AZ
