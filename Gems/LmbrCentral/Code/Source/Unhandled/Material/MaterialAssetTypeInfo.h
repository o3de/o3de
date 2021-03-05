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

#include <AzCore/Asset/AssetTypeInfoBus.h>

namespace LmbrCentral
{
    class MaterialAssetTypeInfo
        : public AZ::AssetTypeInfoBus::Handler
    {
    public:

        AZ_CLASS_ALLOCATOR(MaterialAssetTypeInfo, AZ::SystemAllocator, 0);

        ~MaterialAssetTypeInfo() override;

        //////////////////////////////////////////////////////////////////////////////////////////////
        // AZ::AssetTypeInfoBus::Handler
        AZ::Data::AssetType GetAssetType() const override;
        const char* GetAssetTypeDisplayName() const override;
        const char* GetGroup() const override;
        const char* GetBrowserIcon() const override;
        AZ::Uuid GetComponentTypeId() const override;
        //////////////////////////////////////////////////////////////////////////////////////////////

        void Register();
        void Unregister();
    };

    class DccMaterialAssetTypeInfo
        : public AZ::AssetTypeInfoBus::Handler
    {
    public:

        AZ_CLASS_ALLOCATOR(DccMaterialAssetTypeInfo, AZ::SystemAllocator, 0);

        ~DccMaterialAssetTypeInfo() override;

        //////////////////////////////////////////////////////////////////////////////////////////////
        // AZ::AssetTypeInfoBus::Handler
        AZ::Data::AssetType GetAssetType() const override;
        const char* GetAssetTypeDisplayName() const override;
        const char* GetGroup() const override;
        const char* GetBrowserIcon() const override;
        //////////////////////////////////////////////////////////////////////////////////////////////

        void Register();
        void Unregister();
    };
} // namespace LmbrCentral