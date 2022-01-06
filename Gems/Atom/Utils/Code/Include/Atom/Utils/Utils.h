/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/Asset/AssetCommon.h>
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/BufferPool.h>

//! This file holds useful utility functions for working with the RPI.

namespace AZ
{
    namespace Render
    {

        //! Writes all of the data from the provided vector into the provided buffer.
        //! If the buffer is not large enough to hold the data, the map will not occur and AZ_Error will trigger.
        template <typename T>
        AZ::Outcome<void> WriteToBuffer(RHI::Ptr<RHI::Buffer> buffer, const AZStd::vector<T>& data);

        //! Writes all of the data from the provided pointer into the provided buffer.
        //! If the buffer is not large enough to hold the data, the map will not occur and AZ_Error will trigger.
        AZ::Outcome<void> WriteToBuffer(RHI::Ptr<RHI::Buffer> buffer, const void* data, size_t dataSize);

        //! Creates an asset of AssetType given a path. If the path invalid, then a default constructed asset is
        //! returned. If the path is not empty and the asset isn't found, then a warning is also issued.
        template <typename AssetType>
        Data::Asset<AssetType> GetAssetFromPath(const AZStd::string_view path, AZ::Data::AssetLoadBehavior loadBehavior, bool loadBlocking = false);

        //! Creates an asset of AssetType given an ID. If the ID is invalid, then a default constructed asset is
        //! returned.
        template <typename AssetType>
        Data::Asset<AssetType> GetAssetFromId(Data::AssetId assetId, AZ::Data::AssetLoadBehavior loadBehavior, bool loadBlocking = false);
    }
}

#include <Atom/Utils/Utils.inl>
