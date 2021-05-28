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

#include <Atom/RPI.Reflect/Buffer/BufferAssetView.h>

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
