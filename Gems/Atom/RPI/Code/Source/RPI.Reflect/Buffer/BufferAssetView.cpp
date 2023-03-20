/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Buffer/BufferAssetView.h>
#include <AzCore/Asset/AssetSerializer.h>

namespace AZ
{
    namespace RPI
    {
        void BufferAssetView::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<BufferAssetView>()
                    ->Version(0)
                    ->Field("BufferAsset", &BufferAssetView::m_bufferAsset)
                    ->Field("BufferViewDescriptor", &BufferAssetView::m_bufferViewDescriptor)
                    ;
            }
        }

        BufferAssetView::BufferAssetView(
            const Data::Asset<BufferAsset>& bufferAsset,
            const RHI::BufferViewDescriptor& bufferViewDescriptor)
            : m_bufferAsset(bufferAsset)
            , m_bufferViewDescriptor(bufferViewDescriptor)
        {}

        const Data::Asset<BufferAsset>& BufferAssetView::GetBufferAsset() const
        {
            return m_bufferAsset;
        }

        const RHI::BufferViewDescriptor& BufferAssetView::GetBufferViewDescriptor() const
        {
            return m_bufferViewDescriptor;
        }
    } // namespace RPI
} // namespace AZ
