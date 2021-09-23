/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/BufferDescriptor.h>
#include <Atom/RHI.Reflect/BufferViewDescriptor.h>

#include <Atom/RPI.Public/Buffer/BufferSystemInterface.h>

#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/ResourcePoolAsset.h>

#include <AtomCore/std/containers/array_view.h>

#include <AzCore/Asset/AssetCommon.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        //! An asset representation of a buffer meant to be uploaded to the GPU.
        //! For example: vertex buffer, index buffer, etc
        class BufferAsset final
            : public Data::AssetData
        {
            friend class BufferAssetCreator;

        public:
            static const char* DisplayName;
            static const char* Extension;
            static const char* Group;

            AZ_RTTI(BufferAsset, "{F6C5EA8A-1DB3-456E-B970-B6E2AB262AED}", Data::AssetData);
            AZ_CLASS_ALLOCATOR(BufferAsset, AZ::SystemAllocator, 0);

            static void Reflect(AZ::ReflectContext* context);

            BufferAsset() = default;
            ~BufferAsset() = default;

            AZStd::array_view<uint8_t> GetBuffer() const;

            const RHI::BufferDescriptor& GetBufferDescriptor() const;

            //! Returns the descriptor for a view of the entire buffer
            const RHI::BufferViewDescriptor& GetBufferViewDescriptor() const;

            const Data::Asset<ResourcePoolAsset>& GetPoolAsset() const;

            CommonBufferPoolType GetCommonPoolType() const;
            
            const AZStd::string& GetName() const;

        private:
            // AssetData overrides...
            bool HandleAutoReload() override
            {
                return false;
            }

            // Called by asset creators to assign the asset to a ready state.
            void SetReady();

            AZStd::string m_name;

            AZStd::vector<uint8_t> m_buffer;

            RHI::BufferDescriptor m_bufferDescriptor;

            RHI::BufferViewDescriptor m_bufferViewDescriptor;

            Data::Asset<ResourcePoolAsset> m_poolAsset{ Data::AssetLoadBehavior::PreLoad };

            CommonBufferPoolType m_poolType = CommonBufferPoolType::Invalid;
        };

        using BufferAssetHandler = AssetHandler<BufferAsset>;
    } // namespace RPI
} // namespace AZ
