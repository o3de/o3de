/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/RPISystemDescriptor.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AZ
{
    namespace RPI
    {
        void RPISystemDescriptor::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<DynamicDrawSystemDescriptor>()
                    ->Version(0)
                    ->Field("DynamicBufferPoolSize", &DynamicDrawSystemDescriptor::m_dynamicBufferPoolSize)
                    ;

                serializeContext->Class<RPISystemDescriptor>()
                    ->Version(6) // ATOM-15472
                    ->Field("RHISystemDescriptor", &RPISystemDescriptor::m_rhiSystemDescriptor)
                    ->Field("CommonSrgsShaderAssetPath", &RPISystemDescriptor::m_commonSrgsShaderAssetPath)
                    ->Field("ImageSystemDescriptor", &RPISystemDescriptor::m_imageSystemDescriptor)
                    ->Field("GpuQuerySystemDescriptor", &RPISystemDescriptor::m_gpuQuerySystemDescriptor)
                    ->Field("DynamicDrawSystemDescriptor", &RPISystemDescriptor::m_dynamicDrawSystemDescriptor)
                    ;

                if (AZ::EditContext* ec = serializeContext->GetEditContext())
                {
                    ec->Class<DynamicDrawSystemDescriptor>("Dynamic Draw System Settings", "Settings for the Dynamic Draw System")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &DynamicDrawSystemDescriptor::m_dynamicBufferPoolSize, "Dynamic Buffer Pool Size", "The maxinum size of pool which is used to allocate dynamic buffers")
                        ;

                    ec->Class<RPISystemDescriptor>("RPI Settings", "Settings for runtime RPI system")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &RPISystemDescriptor::m_commonSrgsShaderAssetPath, "Common Shader Asset Path For Scene & View SRGs",
                            "Shader asset path used to get the Scene and View SRGs for all RPI scenes and views respectively")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &RPISystemDescriptor::m_rhiSystemDescriptor, "RHI System Config", "Configuration of Render Hardware Interface")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &RPISystemDescriptor::m_imageSystemDescriptor, "Image System Config", "Configuration of Image System")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &RPISystemDescriptor::m_gpuQuerySystemDescriptor, "Gpu Query System Config", "Configuration of Gpu Query System")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &RPISystemDescriptor::m_dynamicDrawSystemDescriptor, "Dynamic Draw System Config", "Configuration of Dynamic Draw System")
                        ;
                }
            }
        }
    } // namespace RPI
} // namespace AZ
