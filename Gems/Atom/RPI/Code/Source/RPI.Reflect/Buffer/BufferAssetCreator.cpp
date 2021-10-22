/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Buffer/BufferAssetCreator.h>

#include <AzCore/Asset/AssetManager.h>

namespace AZ
{
    namespace RPI
    {
        void BufferAssetCreator::Begin(const Data::AssetId& assetId)
        {
            BeginCommon(assetId);
        }

        void BufferAssetCreator::SetBuffer(const void* initialData, const size_t initialDataSize, const RHI::BufferDescriptor& descriptor)
        {
            if (!ValidateIsReady())
            {
                return;
            }

            // buffer is allowed to be nullptr as a BufferAsset may describe a Read/Write buffer

            if (descriptor.m_byteCount == 0)
            {
                ReportError("Size of the buffer in the descriptor was 0");
                return;
            }

            if (initialDataSize > descriptor.m_byteCount)
            {
                ReportError("initialSize is larger than the total size in the descriptor.");
                return;
            }

            if (initialData == nullptr && initialDataSize > 0)
            {
                ReportError("Initial buffer data was not provided but the initial size was non-zero.");
                return;
            }

            if (initialData != nullptr && initialDataSize == 0)
            {
                ReportError("Initial buffer data was not null but the initial size was zero.");
                return;
            }

            BufferAsset* bufferAsset = m_asset.Get();

            // Only allocate buffer if initial data is not empty
            if (initialData != nullptr && initialDataSize > 0)
            {
                bufferAsset->m_buffer.resize_no_construct(descriptor.m_byteCount);
                memcpy(bufferAsset->m_buffer.data(), initialData, initialDataSize);
            }

            bufferAsset->m_bufferDescriptor = descriptor;
        }

        void BufferAssetCreator::SetBufferViewDescriptor(const RHI::BufferViewDescriptor& viewDescriptor)
        {
            if (!ValidateIsReady())
            {
                return;
            }

            if (viewDescriptor.m_elementCount == 0)
            {
                ReportError("BufferAssetCreator::SetBufferViewDescriptor was given a view descriptor with an element count of 0.");
                return;
            }

            if (viewDescriptor.m_elementSize == 0)
            {
                ReportError("BufferAssetCreator::SetBufferViewDescriptor was given a view descriptor with an element size of 0.");
                return;
            }

            BufferAsset* bufferAsset = m_asset.Get();

            bufferAsset->m_bufferViewDescriptor = viewDescriptor;
        }

        void BufferAssetCreator::SetPoolAsset(const Data::Asset<ResourcePoolAsset>& poolAsset)
        {
            if (ValidateIsReady())
            {
                m_asset->m_poolAsset = poolAsset;
            }
        }

        void BufferAssetCreator::SetUseCommonPool(CommonBufferPoolType poolType)
        {
            if (ValidateIsReady())
            {
                m_asset->m_poolType = poolType;
            }
        }

        bool BufferAssetCreator::End(Data::Asset<BufferAsset>& result)
        {
            if (!ValidateIsReady() || !ValidateBuffer())
            {
                return false;
            }

            m_asset->SetReady();
            return EndCommon(result);
        }

        bool BufferAssetCreator::ValidateBuffer()
        {
            RPI::BufferAsset* bufferAsset = m_asset.Get();

            if (!bufferAsset->m_poolAsset.GetId().IsValid() && bufferAsset->m_poolType == CommonBufferPoolType::Invalid)
            {
                ReportError("BufferAssetCreator::ValidateBuffer Failed; need valid pool asset or select a valid common pool.");
                return false;
            }

            if (bufferAsset->m_bufferDescriptor.m_byteCount == 0)
            {
                ReportError("BufferAssetCreator::ValidateBuffer Failed; buffer descriptor has a byte count of 0.");
                return false;
            }

            if (bufferAsset->m_bufferViewDescriptor.m_elementCount == 0)
            {
                ReportError("BufferAssetCreator::ValidateBuffer Failed; buffer view descirptor has an element count of 0.");
                return false;
            }

            if (bufferAsset->m_bufferViewDescriptor.m_elementSize == 0)
            {
                ReportError("BufferAssetCreator::ValidateBuffer Failed; buffer view descriptor has an element size of 0.");
                return false;
            }

            return true;
        }

        void BufferAssetCreator::SetBufferName(AZStd::string_view name)
        {
            if (ValidateIsReady())
            {
                m_asset->m_name = name;
            }
        }

        bool BufferAssetCreator::Clone(const Data::Asset<BufferAsset>& sourceAsset, Data::Asset<BufferAsset>& clonedResult, Data::AssetId& inOutLastCreatedAssetId)
        {
            BufferAssetCreator creator;
            inOutLastCreatedAssetId.m_subId = inOutLastCreatedAssetId.m_subId + 1;
            creator.Begin(inOutLastCreatedAssetId);

            creator.SetBufferName(sourceAsset.GetHint());
            creator.SetUseCommonPool(sourceAsset->GetCommonPoolType());
            creator.SetPoolAsset(sourceAsset->GetPoolAsset());
            creator.SetBufferViewDescriptor(sourceAsset->GetBufferViewDescriptor());

            const AZStd::array_view<uint8_t> sourceBuffer = sourceAsset->GetBuffer();
            creator.SetBuffer(sourceBuffer.data(), sourceBuffer.size(), sourceAsset->GetBufferDescriptor());

            return creator.End(clonedResult);
        }
    } // namespace RPI
} // namespace AZ
