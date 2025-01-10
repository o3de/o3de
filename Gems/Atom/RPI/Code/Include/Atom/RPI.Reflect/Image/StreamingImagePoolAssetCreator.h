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
#include <Atom/RPI.Reflect/Image/StreamingImagePoolAsset.h>

namespace AZ
{
    namespace RPI
    {
        //! Constructs an instance of an StreamingImagePoolAsset.
        //! (Note this class generally follows the builder design pattern, but is called a "creator" rather 
        //! than a "builder" to avoid confusion with the AssetBuilderSDK).
        class ATOM_RPI_REFLECT_API StreamingImagePoolAssetCreator
            : public AssetCreator<StreamingImagePoolAsset>
        {
        public:
            StreamingImagePoolAssetCreator() = default;

            //! Begins construction of a new streaming image pool asset instance. Resets the builder to a fresh state.
            //! assetId the unique id to use when creating the asset.
            void Begin(const Data::AssetId& assetId);

            //! Assigns the descriptor used to initialize the RHI streaming image pool.
            void SetPoolDescriptor(AZStd::unique_ptr<RHI::StreamingImagePoolDescriptor>&& descriptor);

            //! Assigns the name of the pool
            void SetPoolName(AZStd::string_view poolName);
            
            //! Finalizes and assigns ownership of the asset to result, if successful. 
            //! Otherwise false is returned and result is left untouched.
            bool End(Data::Asset<StreamingImagePoolAsset>& result);
        };
    }
}
