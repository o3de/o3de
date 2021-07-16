/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Buffer/BufferAsset.h>

namespace AZ
{
    namespace RPI
    {
        class BufferAssetView final
        {
        public:
            AZ_TYPE_INFO(BufferAssetView, "{D2A51B9F-4210-477E-B253-0095EAF68230}");
            AZ_CLASS_ALLOCATOR(BufferAssetView, AZ::SystemAllocator, 0);

            static void Reflect(AZ::ReflectContext* context);

            BufferAssetView() = default;
            BufferAssetView(
                const Data::Asset<BufferAsset>& bufferAsset, 
                const RHI::BufferViewDescriptor& bufferViewDescriptor);
            ~BufferAssetView() = default;

            const RHI::BufferViewDescriptor& GetBufferViewDescriptor() const;

            const Data::Asset<BufferAsset>& GetBufferAsset() const;

        private:
            RHI::BufferViewDescriptor m_bufferViewDescriptor;
            Data::Asset<BufferAsset> m_bufferAsset;
        };
    } // namespace RPI
} // namespace AZ
