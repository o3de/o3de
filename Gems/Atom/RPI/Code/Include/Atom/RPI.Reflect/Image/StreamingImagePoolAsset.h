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
#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RPI.Reflect/Image/StreamingImageControllerAsset.h>
#include <Atom/RHI.Reflect/StreamingImagePoolDescriptor.h>

namespace AZ
{
    namespace RPI
    {
        class StreamingImageController;

        //! Flat data used to instantiate a streaming image pool instance at runtime.
        //! The streaming image pool asset contains configuration data used to instantiate both
        //! a pool instance and a controller instance. Each pool asset is able to spawn a unique
        //! streaming controller implementation with its own platform-specific configuration data.
        //! Similarly, the pool descriptor may also be platform-specific.
        //! To accomplish this, each descriptor has its own override.
        //! - The pool descriptor *may* be overridden with a platform-specific derived version. Do this
        //!   to communicate platform-specific details directly to the platform under the RHI.
        //! - The controller descriptor is completely abstract, so it must be overridden to communicate
        //!   configuration data to the underlying controller implementation.
        //! Both of these overrides should be assigned at asset build time for the specific platform.
        //! This is an immutable, serialized asset. It can be either serialized-in or created dynamically using StreamingImagePoolAssetCreator.
        //! See RPI::StreamingImagePool for runtime features based on this asset.
        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_REFLECT_API StreamingImagePoolAsset final
            : public Data::AssetData
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            friend class StreamingImagePoolAssetCreator;
            friend class StreamingImagePoolAssetTester;
        public:
            AZ_RTTI(StreamingImagePoolAsset, "{877B2DA2-BBE7-42E7-AED3-F571929820FE}", Data::AssetData);
            AZ_CLASS_ALLOCATOR(StreamingImagePoolAsset, SystemAllocator);

            static constexpr const char* DisplayName{ "StreamingImagePool" };
            static constexpr const char* Group{ "Image" };
            static constexpr const char* Extension{ "streamingimagepool" };

            static void Reflect(AZ::ReflectContext* context);

            StreamingImagePoolAsset() = default;

            //! Returns the RHI streaming image pool descriptor used to initialize a runtime instance. This
            //! is a heap-allocated shared base-class pointer. It may be a RHI backend-specific derived class
            //! type. This is determined by the asset builder.
            const RHI::StreamingImagePoolDescriptor& GetPoolDescriptor() const;

            //! Returns the name of the pool.
            AZStd::string_view GetPoolName() const;

        private:

            // Called by StreamingImagePoolAssetCreator to assign the asset to a ready state.
            void SetReady();

            // [Serialized] The platform-specific descriptor used to initialize the RHI pool.
            AZStd::unique_ptr<RHI::StreamingImagePoolDescriptor> m_poolDescriptor;

            // [Serialized] A display name for this pool. 
            AZStd::string m_poolName;
        };

        using StreamingImagePoolAssetHandler = AssetHandler<StreamingImagePoolAsset>;
    }
}
