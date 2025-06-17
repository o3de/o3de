/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            return Data::InstanceDatabase<AttachmentImage>::Instance().FindOrCreate(
                Data::InstanceId::CreateFromAssetId(imageAsset.GetId()), imageAsset);
        }

        Data::Instance<AttachmentImage> AttachmentImage::Create(
            const AttachmentImagePool& imagePool,
            const RHI::ImageDescriptor& imageDescriptor,
            const Name& imageName,
            const RHI::ClearValue* optimizedClearValue,
            const RHI::ImageViewDescriptor* imageViewDescriptor)
        {
            CreateAttachmentImageRequest createImageRequest;
            createImageRequest.m_imagePool = &imagePool;
            createImageRequest.m_imageDescriptor = imageDescriptor;
            createImageRequest.m_imageName = imageName;
            createImageRequest.m_isUniqueName = false;
            createImageRequest.m_optimizedClearValue = optimizedClearValue;
            createImageRequest.m_imageViewDescriptor = imageViewDescriptor;
            return Create(createImageRequest);
        }
                    
        Data::Instance<AttachmentImage> AttachmentImage::Create(const CreateAttachmentImageRequest& createImageRequest)
        {
            Data::Asset<AttachmentImageAsset> imageAsset;

            Data::AssetId assetId;
            if (createImageRequest.m_isUniqueName)
            {
                assetId = AZ::Uuid::CreateName(createImageRequest.m_imageName.GetCStr());
            }
            else
            {
                assetId = AZ::Uuid::CreateRandom();
            }

            Data::InstanceId instanceId = Data::InstanceId::CreateFromAssetId(assetId);

            AttachmentImageAssetCreator imageAssetCreator;
            imageAssetCreator.Begin(assetId);
            imageAssetCreator.SetImageDescriptor(createImageRequest.m_imageDescriptor);
            imageAssetCreator.SetPoolAsset({createImageRequest.m_imagePool->GetAssetId(), azrtti_typeid<ResourcePoolAsset>()});
            imageAssetCreator.SetName(createImageRequest.m_imageName, createImageRequest.m_isUniqueName);

            if (createImageRequest.m_imageViewDescriptor)
            {
                imageAssetCreator.SetImageViewDescriptor(*createImageRequest.m_imageViewDescriptor);
            }

            if (createImageRequest.m_optimizedClearValue)
            {
                imageAssetCreator.SetOptimizedClearValue(*createImageRequest.m_optimizedClearValue);
            }

            if (imageAssetCreator.End(imageAsset))
            {
                return Data::InstanceDatabase<AttachmentImage>::Instance().FindOrCreate(instanceId, imageAsset);
            }

            return nullptr;
        }

        Data::Instance<AttachmentImage> AttachmentImage::FindByUniqueName(const Name& uniqueAttachmentName)
        {            
            return ImageSystemInterface::Get()->FindRegisteredAttachmentImage(uniqueAttachmentName);
        }

        Data::Instance<AttachmentImage> AttachmentImage::CreateInternal(AttachmentImageAsset& imageAsset)
        {
            Data::Instance<AttachmentImage> image = aznew AttachmentImage();
            auto result = image->Init(imageAsset);

            if (result == RHI::ResultCode::Success)
            {
                image->m_imageAsset = { &imageAsset, AZ::Data::AssetLoadBehavior::PreLoad };
                return image;
            }

            return nullptr;
        }

        AttachmentImage::AttachmentImage() = default;

        AttachmentImage::~AttachmentImage()
        {
            Shutdown(); 
        }

        RHI::ResultCode AttachmentImage::Init(const AttachmentImageAsset& imageAsset)
        {
            Data::Instance<AttachmentImagePool> pool;
            if (imageAsset.GetPoolAsset().GetId().IsValid())
            {
                pool = AttachmentImagePool::FindOrCreate(imageAsset.GetPoolAsset());
            }
            else
            {
                pool = ImageSystemInterface::Get()->GetSystemAttachmentPool();
            }

            if (!pool)
            {
                AZ_Error("AttachmentImage", false, "Failed to acquire the attachment image pool instance.");
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
                m_imageView = m_image->BuildImageView(imageAsset.GetImageViewDescriptor());
                if(!m_imageView.get())
                {
                    AZ_Error("AttachmentImage", false, "AttachmentImage::Init() failed to initialize RHI image view.");
                    return RHI::ResultCode::Fail;
                }
                
                m_image->SetName(imageAsset.GetName());
                m_attachmentId = imageAsset.GetAttachmentId();

                if (imageAsset.HasUniqueName())
                {
                    ImageSystemInterface::Get()->RegisterAttachmentImage(this);
                }

                return RHI::ResultCode::Success;
            }

            AZ_Error("AttachmentImage", false, "AttachmentImage::Init() failed to initialize RHI image [%d].", resultCode);
            return resultCode;
        }
        
        const RHI::AttachmentId& AttachmentImage::GetAttachmentId() const
        {
            return m_attachmentId;
        }
        
        void AttachmentImage::Shutdown()
        {
            if (m_imageAsset && m_imageAsset->HasUniqueName())
            {
                ImageSystemInterface::Get()->UnregisterAttachmentImage(this);
            }
        }
    }
}
