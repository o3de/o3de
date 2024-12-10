/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>

#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RPI.Reflect/Configuration.h>

#include <Atom/RHI/DeviceResourcePool.h>

namespace AZ
{
    class ReflectContext;

    namespace RHI
    {
        class ResourcePoolDescriptor;
        class Device;
    };

    namespace RPI
    {
        //! ResourcePoolAsset is the AssetData class for a resource pool asset.
        //! This is an immutable, serialized asset. It can be either serialized-in or created dynamically using ResourcePoolAssetCreator.
        //! Multiple runtime pool classes are based on this asset, for example RPI::ImagePool, RPI::StreamingImagePool, RPI::ShaderResourceGroupPool.
        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_REFLECT_API ResourcePoolAsset
            : public AZ::Data::AssetData
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            friend class ResourcePoolAssetCreator;

        public:
            AZ_RTTI(ResourcePoolAsset, "{62A59999-66DA-467E-804A-0EA64A64299F}", AZ::Data::AssetData);
            AZ_CLASS_ALLOCATOR(ResourcePoolAsset, AZ::SystemAllocator);

            static constexpr const char* DisplayName{ "ResourcePool" };
            static constexpr const char* Group{ "RenderingPipeline" };
            static constexpr const char* Extension{ "pool" };

            static void Reflect(ReflectContext* context);

            ResourcePoolAsset(const AZ::Data::AssetId& assetId = AZ::Data::AssetId());

            AZStd::string_view GetPoolName() const;

            const AZStd::shared_ptr<RHI::ResourcePoolDescriptor>& GetPoolDescriptor() const;

        protected:
            // Used by ResourcePoolAssetBuilder when the asset was built successfully
            void SetReady();

            // A RHI pool descriptor which could be a RHI::BufferPoolDescriptor or RHI::ImagePoolDescriptor
            AZStd::shared_ptr<RHI::ResourcePoolDescriptor> m_poolDescriptor;

            // A display name for this pool. 
            AZStd::string m_poolName;
        };
        
        using ResourcePoolAssetHandler = AssetHandler<ResourcePoolAsset>;

    } //namespace RPI
} //namespace AZ
