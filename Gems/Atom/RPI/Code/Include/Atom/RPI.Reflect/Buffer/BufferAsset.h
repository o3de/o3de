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

#include <Atom/RPI.Reflect/Allocators.h>
#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RPI.Reflect/ResourcePoolAsset.h>

#include <AzCore/std/containers/span.h>

#include <AzCore/Asset/AssetCommon.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        //! An asset representation of a buffer meant to be uploaded to the GPU.
        //! For example: vertex buffer, index buffer, etc
        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_REFLECT_API BufferAsset final
            : public Data::AssetData
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            friend class BufferAssetCreator;

        public:
            static constexpr const char* DisplayName{ "BufferAsset" };
            static constexpr const char* Group{ "Buffer" };
            static constexpr const char* Extension{ "azbuffer" };

            AZ_RTTI(BufferAsset, "{F6C5EA8A-1DB3-456E-B970-B6E2AB262AED}", Data::AssetData);
            AZ_CLASS_ALLOCATOR_DECL;

            static void Reflect(AZ::ReflectContext* context);

            BufferAsset() = default;
            ~BufferAsset() = default;

            AZStd::span<const uint8_t> GetBuffer() const;

            const RHI::BufferDescriptor& GetBufferDescriptor() const;

            //! Returns the descriptor for a view of the entire buffer
            const RHI::BufferViewDescriptor& GetBufferViewDescriptor() const;

            const Data::Asset<ResourcePoolAsset>& GetPoolAsset() const;

            CommonBufferPoolType GetCommonPoolType() const;
            
            const AZStd::string& GetName() const;

        private:
            using Allocator = BufferAssetAllocator_for_std_t;

            // AssetData overrides...
            bool HandleAutoReload() override
            {
                // Automatic asset reloads via the AssetManager are disabled for Atom models and their dependent assets because reloads
                // need to happen in a specific order to refresh correctly. They require more complex code than what the default
                // AssetManager reloading provides. See ModelReloader() for the actual handling of asset reloads.
                // Models need to be loaded via the MeshFeatureProcessor to reload correctly, and reloads can be listened
                // to by using MeshFeatureProcessor::ConnectModelChangeEventHandler().
                return false;
            }

            // Called by asset creators to assign the asset to a ready state.
            void SetReady();

            AZStd::string m_name;

            AZStd::vector<uint8_t, Allocator> m_buffer;

            RHI::BufferDescriptor m_bufferDescriptor;

            RHI::BufferViewDescriptor m_bufferViewDescriptor;

            Data::Asset<ResourcePoolAsset> m_poolAsset{ Data::AssetLoadBehavior::PreLoad };

            CommonBufferPoolType m_poolType = CommonBufferPoolType::Invalid;
        };

        using BufferAssetHandler = AssetHandler<BufferAsset>;
    } // namespace RPI
} // namespace AZ
