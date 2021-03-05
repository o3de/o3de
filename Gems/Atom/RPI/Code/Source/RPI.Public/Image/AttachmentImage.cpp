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

#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>

#include <Atom/RPI.Reflect/Base.h>
#include <Atom/RPI.Reflect/Image/AttachmentImageAsset.h>
#include <Atom/RPI.Reflect/Image/AttachmentImageAssetCreator.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/ImagePool.h>

#include <AtomCore/Instance/InstanceDatabase.h>

namespace AZ
{
    namespace RPI
    {
        Data::Instance<AttachmentImage> AttachmentImage::FindOrCreate(const Data::Asset<AttachmentImageAsset>& imageAsset)
        {
            auto image = Data::InstanceDatabase<AttachmentImage>::Instance().FindOrCreate(
                Data::InstanceId::CreateFromAssetId(imageAsset.GetId()),
                imageAsset);
            if (image && image->m_image)
            {
                image->m_image->SetName(Name(imageAsset.GetHint()));
                image->m_attachmentId = image->m_image->GetName();
            }
            return image;
        }

        Data::Instance<AttachmentImage> AttachmentImage::Create(
            const AttachmentImagePool& imagePool,
            const RHI::ImageDescriptor& imageDescriptor,
            const Name& imageName,
            const RHI::ClearValue* optimizedClearValue,
            const RHI::ImageViewDescriptor* imageViewDescriptor)
        {
            Data::Asset<AttachmentImageAsset> imageAsset;

            AttachmentImageAssetCreator imageAssetCreator;
            auto uuid = Uuid::CreateRandom();
            imageAssetCreator.Begin(uuid);
            imageAssetCreator.SetImageDescriptor(imageDescriptor);
            imageAssetCreator.SetPoolAsset({imagePool.GetAssetId(), azrtti_typeid<ResourcePoolAsset>()});
            imageAssetCreator.SetAssetHint(imageName.GetCStr());

            if (imageViewDescriptor)
            {
                imageAssetCreator.SetImageViewDescriptor(*imageViewDescriptor);
            }

            if (optimizedClearValue)
            {
                imageAssetCreator.SetOptimizedClearValue(*optimizedClearValue);
            }

            if (imageAssetCreator.End(imageAsset))
            {
                return AttachmentImage::FindOrCreate(imageAsset);
            }

            return nullptr;
        }

        Data::Instance<AttachmentImage> AttachmentImage::CreateInternal(AttachmentImageAsset& imageAsset)
        {
            Data::Instance<AttachmentImage> image = aznew AttachmentImage();
            auto result = image->Init(imageAsset);

            if (result == RHI::ResultCode::Success)
            {
                return image;
            }

            return nullptr;
        }

        RHI::ResultCode AttachmentImage::Init(const AttachmentImageAsset& imageAsset)
        {
            Data::Instance<AttachmentImagePool> pool = ImageSystemInterface::Get()->GetSystemAttachmentPool();
            if (!pool)
            {
                AZ_Error("AttachmentImage", false, "Failed to acquire the image pool instance.");
                return RHI::ResultCode::Fail;
            }

            RHI::ImagePool* rhiPool = pool->GetRHIPool();

            RHI::ImageInitRequest initRequest;
            initRequest.m_image = GetRHIImage();
            initRequest.m_descriptor = imageAsset.GetImageDescriptor();
            initRequest.m_optimizedClearValue = imageAsset.GetOptimizedClearValue();

            RHI::ResultCode resultCode = rhiPool->InitImage(initRequest);
            if (resultCode == RHI::ResultCode::Success)
            {
                m_imagePool = pool;
                m_imageView = m_image->GetImageView(imageAsset.GetImageViewDescriptor());
                if(!m_imageView.get())
                {
                    AZ_Error("AttachmentImage", false, "AttachmentImage::Init() failed to initialize RHI image view.");
                    return RHI::ResultCode::Fail;
                }

                return RHI::ResultCode::Success;
            }

            AZ_Error("AttachmentImage", false, "AttachmentImage::Init() failed to initialize RHI image [%d].", resultCode);
            return resultCode;
        }
        
        RHI::ResultCode AttachmentImage::UpdateImageContents(const RHI::ImageUpdateRequest& request)
        {
            return m_imagePool->GetRHIPool()->UpdateImageContents(request);
        }

        const RHI::AttachmentId& AttachmentImage::GetAttachmentId()
        {
            return m_attachmentId;
        }
    }
}
