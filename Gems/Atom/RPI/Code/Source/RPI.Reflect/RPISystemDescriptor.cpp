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

                serializeContext->Class<RayTracingSystemDescriptor>()
                    ->Version(0)
                    ->Field("EnableBlasCompaction", &RayTracingSystemDescriptor::m_enableBlasCompaction)
                    ->Field("MaxBlasCreatedPerFrame", &RayTracingSystemDescriptor::m_maxBlasCreatedPerFrame)
                    ->Field("RayTracingCompactionQueryPoolSize", &RayTracingSystemDescriptor::m_rayTracingCompactionQueryPoolSize);

                serializeContext->Class<RPISystemDescriptor>()
                    ->Version(7) // ATOM-16237
                    ->Field("CommonSrgsShaderAssetPath", &RPISystemDescriptor::m_commonSrgsShaderAssetPath)
                    ->Field("ImageSystemDescriptor", &RPISystemDescriptor::m_imageSystemDescriptor)
                    ->Field("GpuQuerySystemDescriptor", &RPISystemDescriptor::m_gpuQuerySystemDescriptor)
                    ->Field("DynamicDrawSystemDescriptor", &RPISystemDescriptor::m_dynamicDrawSystemDescriptor)
                    ->Field("RayTracingSystemDescriptor", &RPISystemDescriptor::m_rayTracingSystemDescriptor)
                    ->Field("NullRenderer", &RPISystemDescriptor::m_isNullRenderer);
            }
        }
    } // namespace RPI
} // namespace AZ
