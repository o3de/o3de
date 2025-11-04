/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Image/ImageSystemDescriptor.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AZ
{
    namespace RPI
    {
        void ImageSystemDescriptor::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ImageSystemDescriptor>()
                    ->Version(0)
                    ->Field("SystemStreamingImagePoolSize", &ImageSystemDescriptor::m_systemStreamingImagePoolSize)
                    ->Field("SystemStreamingImagePoolMipBias", &ImageSystemDescriptor::m_systemStreamingImagePoolMipBias)
                    ->Field("SystemAttachmentImagePoolSize", &ImageSystemDescriptor::m_systemAttachmentImagePoolSize)
                    ;
            }
        }
    } // namespace RPI
} // namespace AZ
