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
#include "CryRenderOther_precompiled.h"
#include "AttachmentImagePool.h"
#include "Conversions.h"
#include "Image.h"
#include "Device.h"
#include "ObjectCollector.h"
#include <Adapters/RHI/MemoryStatisticsBuilder.h>

namespace AZ
{
    namespace DX12
    {
        class CommandList;

        RHI::Ptr<AttachmentImagePool> AttachmentImagePool::Create()
        {
            return aznew AttachmentImagePool();
        }

        RHI::ResultCode AttachmentImagePool::InitImageInternal(const RHI::AttachmentImageInitRequest& request)
        {
            Image* image = static_cast<Image*>(request.m_image);
            image->Init(request.m_descriptor);

            /**
             * Super simple implementation. Just creates a committed resource for the image. No
             * real pooling happening at the moment. Other approaches might involve creating dedicated
             * heaps and then placing resources onto those heaps. This will allow us to control residency
             * at the heap level.
             */

            RHI::Ptr<Resource> resource = Device::Instance().CreateImageCommitted(
                request.m_descriptor,
                request.m_optimizedClearValue,
                D3D12_RESOURCE_STATE_COMMON,
                D3D12_HEAP_TYPE_DEFAULT);

            if (resource)
            {
                m_memoryUsageLocal += resource->GetAllocationInfo().SizeInBytes;
                image->SetResource(AZStd::move(resource));
                return RHI::ResultCode::Success;
            }

            return RHI::ResultCode::Fail;
        }

        void AttachmentImagePool::ShutdownImageInternal(RHI::Image& imageBase)
        {
            Image& image = static_cast<Image&>(imageBase);
            m_memoryUsageLocal -= image.GetResource().GetAllocationInfo().SizeInBytes;
        }

        void AttachmentImagePool::ReportMemoryUsageInternal(RHI::MemoryStatisticsBuilder& builder) const
        {
            const size_t currentMemoryUsage = m_memoryUsageLocal;

            RHI::MemoryUsageInfo info;
            info.m_byteCountBudget = currentMemoryUsage;
            info.m_byteCountCapacity = currentMemoryUsage;
            info.m_byteCountResident = currentMemoryUsage;

            builder.SetMemoryUsageForHeap(RHI::PlatformHeapId{ RHI::PlatformHeapType::Local }, info);
        }
    }
}