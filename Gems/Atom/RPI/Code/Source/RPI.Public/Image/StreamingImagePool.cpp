/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Base.h>

#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Image/StreamingImagePool.h>
#include <Atom/RPI.Public/Image/StreamingImageController.h>

#include <Atom/RHI/Factory.h>

#include <AzCore/Debug/EventTrace.h>
#include <AtomCore/Instance/InstanceDatabase.h>

namespace AZ
{
    namespace RPI
    {
        Data::Instance<StreamingImagePool> StreamingImagePool::FindOrCreate(const Data::Asset<StreamingImagePoolAsset>& streamingImagePoolAsset)
        {
            return Data::InstanceDatabase<StreamingImagePool>::Instance().FindOrCreate(
                Data::InstanceId::CreateFromAssetId(streamingImagePoolAsset.GetId()),
                streamingImagePoolAsset);
        }

        Data::Instance<StreamingImagePool> StreamingImagePool::CreateInternal(RHI::Device& device, StreamingImagePoolAsset& streamingImagePoolAsset)
        {
            Data::Instance<StreamingImagePool> streamingImagePool = aznew StreamingImagePool();
            const RHI::ResultCode resultCode = streamingImagePool->Init(device, streamingImagePoolAsset);

            if (resultCode == RHI::ResultCode::Success)
            {
                return streamingImagePool;
            }

            return nullptr;
        }

        RHI::ResultCode StreamingImagePool::Init(RHI::Device& device, StreamingImagePoolAsset& poolAsset)
        {
            AZ_TRACE_METHOD();

            if (Validation::IsEnabled())
            {
                if (m_pool)
                {
                    AZ_Error("StreamingImagePool", false, "Invalid Operation: Attempted to initialize an already initialized pool.");
                    return RHI::ResultCode::InvalidOperation;
                }
            }

            RHI::Ptr<RHI::StreamingImagePool> pool = RHI::Factory::Get().CreateStreamingImagePool();

            const RHI::ResultCode resultCode = pool->Init(device, poolAsset.GetPoolDescriptor());

            if (resultCode == RHI::ResultCode::Success)
            {
                m_controller = StreamingImageController::Create(poolAsset.GetControllerAsset(), *pool);
                m_pool = AZStd::move(pool);
                m_pool->SetName(AZ::Name{ poolAsset.GetPoolName() });
                return RHI::ResultCode::Success;
            }

            AZ_Warning("StreamingImagePoolAsset", false, "Failed to initialize RHI::StreamingImagePool.");
            return resultCode;
        }

        void StreamingImagePool::AttachImage(StreamingImage* image)
        {
            // Add image to controller only if the image is streamable 
            if (image->IsStreamable())
            {
                m_controller->AttachImage(image);
            }
        }

        void StreamingImagePool::DetachImage(StreamingImage* image)
        {
            if (image->IsStreamable())
            {
                m_controller->DetachImage(image);
            }
        }

        void StreamingImagePool::Update()
        {
            m_controller->Update();
        }

        RHI::StreamingImagePool* StreamingImagePool::GetRHIPool()
        {
            return m_pool.get();
        }

        const RHI::StreamingImagePool* StreamingImagePool::GetRHIPool() const
        {
            return m_pool.get();
        }
    }
}
