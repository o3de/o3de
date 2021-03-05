/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
