/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Components/NodePropertyDisplay/DataInterface.h>

namespace GraphCanvas
{
    class AssetIdDataInterface
        : public DataInterface
    {

    public:
        virtual AZ::Data::AssetId GetAssetId() const = 0;
        virtual void SetAssetId(const AZ::Data::AssetId& assetId) = 0;

        virtual AZ::Data::AssetType GetAssetType() const = 0;
        virtual AZStd::string GetStringFilter() const = 0; 

        virtual void SetAssetType(AZ::Data::AssetType) {}
        virtual void SetStringFilter(const AZStd::string&) {}

    };
}
