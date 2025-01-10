/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/AssetCreator.h>
#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RPI.Reflect/ResourcePoolAsset.h>

namespace AZ
{
    namespace RPI
    {
        //! Use a ResourcePoolAssetCreator to create and configure a new ResourcePoolAsset which can either be BufferPoolAsset or ImagePoolAsset.
        //! (Note this class generally follows the builder design pattern, but is called a "creator" rather 
        //! than a "builder" to avoid confusion with the AssetBuilderSDK).
        class ATOM_RPI_REFLECT_API ResourcePoolAssetCreator
            : public AssetCreator<ResourcePoolAsset>
        {
        public:
            ResourcePoolAssetCreator() = default;

            //! Begins construction of a new ResourcePoolAsset. Resets the builder to a fresh state.
            //! @param assetId the unique id to use when creating the asset.
            void Begin(const Data::AssetId& assetId);

            //! Set a pool descriptor which can be BufferPoolDescriptor or ImagePoolDescriptor 
            //! @param poolDescriptor the unique ptr provides pool descriptor.
            //! Example: when assigning a derived pool descriptor, the code will look like
            //!     ResourcePoolAssetCreator assetCreator;
            //!      ...
            //!     AZStd::unique_ptr<RHI::BufferPoolDescriptor> bufferPoolDescriptor = AZStd::make_unique<RHI::BufferPoolDescriptor>();
            //!     ...
            //!     assetCreator.SetPoolDescriptor(AZStd::move(bufferPoolDescriptor));
            void SetPoolDescriptor(AZStd::unique_ptr<RHI::ResourcePoolDescriptor> poolDescriptor);
                       
            void SetPoolName(AZStd::string_view poolName);
            
            //! Finalizes and assigns ownership of the asset to result, if successful. 
            //! Otherwise false is returned and result is left untouched.
            bool End(Data::Asset<ResourcePoolAsset>& result);
        };
    } //namespace RPI
} //namespace AZ
