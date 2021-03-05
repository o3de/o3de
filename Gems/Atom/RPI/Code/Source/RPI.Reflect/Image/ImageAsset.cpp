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
