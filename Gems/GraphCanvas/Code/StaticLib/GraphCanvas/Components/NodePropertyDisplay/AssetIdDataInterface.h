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

    };
}