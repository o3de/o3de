/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
                    ->Field("AssetStreamingImagePoolSize", &ImageSystemDescriptor::m_assetStreamingImagePoolSize)
                    ->Field("SystemStreamingImagePoolSize", &ImageSystemDescriptor::m_systemStreamingImagePoolSize)
                    ->Field("SystemAttachmentImagePoolSize", &ImageSystemDescriptor::m_systemAttachmentImagePoolSize)
                    ;

                if (AZ::EditContext* ec = serializeContext->GetEditContext())
                {
                    ec->Class<ImageSystemDescriptor>("Image System Config", "Settings for RPI Image System")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ImageSystemDescriptor::m_assetStreamingImagePoolSize,
                            "Streaming image pool size for assets", "Streaming image pool size in bytes for streaming images created from assets")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ImageSystemDescriptor::m_systemStreamingImagePoolSize,
                            "System streaming image pool size", "Streaming image pool size in bytes for streaming images created in memory")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ImageSystemDescriptor::m_systemAttachmentImagePoolSize,
                            "System attachment image pool size", "Default attachment image pool size in bytes")
                        ;
                }
            }
        }
    } // namespace RPI
} // namespace AZ
