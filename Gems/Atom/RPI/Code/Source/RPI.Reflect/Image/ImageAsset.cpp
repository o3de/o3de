/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Image/ImageAsset.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {
        void ImageAsset::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ImageAsset, Data::AssetData>()
                    ->Field("m_imageDescriptor", &ImageAsset::m_imageDescriptor)
                    ->Field("m_imageViewDescriptor", &ImageAsset::m_imageViewDescriptor)
                    ;
            }
        }

        ImageAsset::~ImageAsset()
        {
        }

        const RHI::ImageDescriptor& ImageAsset::GetImageDescriptor() const
        {
            return m_imageDescriptor;
        }

        const RHI::ImageViewDescriptor& ImageAsset::GetImageViewDescriptor() const
        {
            return m_imageViewDescriptor;
        }
        
        void ImageAsset::SetReady()
        {
            m_status = AssetStatus::Ready;
        }
    }
}
