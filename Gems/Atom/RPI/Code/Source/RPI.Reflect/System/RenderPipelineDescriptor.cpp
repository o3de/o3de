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

#include <Atom/RPI.Reflect/System/RenderPipelineDescriptor.h>

namespace AZ
{
    namespace RPI
    {
        void RenderPipelineDescriptor::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<RenderPipelineDescriptor>()
                    ->Version(2)
                    ->Field("Name", &RenderPipelineDescriptor::m_name)
                    ->Field("MainViewTag", &RenderPipelineDescriptor::m_mainViewTagName)
                    ->Field("RootPassTemplate", &RenderPipelineDescriptor::m_rootPassTemplate)
                    ->Field("ExecuteOnce", &RenderPipelineDescriptor::m_executeOnce)
                    ->Field("RenderSettings", &RenderPipelineDescriptor::m_renderSettings)
                ;
            }
        }
    } // namespace RPI
} // namespace AZ
