/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
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
                    ->Field("MaterialPipelineTag", &RenderPipelineDescriptor::m_materialPipelineTag)
                    ->Field("ExecuteOnce", &RenderPipelineDescriptor::m_executeOnce)
                    ->Field("RenderSettings", &RenderPipelineDescriptor::m_renderSettings)
                    ->Field("DefaultAAMethod", &RenderPipelineDescriptor::m_defaultAAMethod)
                    ->Field("AllowModification", &RenderPipelineDescriptor::m_allowModification)
                    ->Field("AllowSubpassMerging", &RenderPipelineDescriptor::m_allowSubpassMerging)
                ;
            }
        }
    } // namespace RPI
} // namespace AZ
