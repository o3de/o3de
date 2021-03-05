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

#pragma once

#include <AzCore/std/containers/vector.h>
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
