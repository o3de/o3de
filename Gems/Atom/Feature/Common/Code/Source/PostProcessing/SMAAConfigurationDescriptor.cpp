/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcessing/SMAAConfigurationDescriptor.h>

namespace AZ
{
    namespace Render
    {
        void SMAAConfigurationDescriptor::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<SMAAConfigurationDescriptor>()
                    ->Version(1)
                    ->Field("Name", &SMAAConfigurationDescriptor::m_name)
                    ->Field("Enable", &SMAAConfigurationDescriptor::m_enable)
                    ->Field("EdgeDetectionMode", &SMAAConfigurationDescriptor::m_edgeDetectionMode)
                    ->Field("OutputMode", &SMAAConfigurationDescriptor::m_outputMode)
                    ->Field("Quality", &SMAAConfigurationDescriptor::m_quality)
                    ;
            }
        }
    } // namespace Render
} // namespace AZ
