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

#include <AzCore/Component/Component.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzFramework
{
    class AssetCatalog;

    class AssetCatalogComponent
        : public AZ::Component
    {
    public:

        AZ_COMPONENT(AssetCatalogComponent, "{35D9C27B-CD07-4333-89BB-3D077444E10A}");

        AssetCatalogComponent();
        ~AssetCatalogComponent() override;

        void Init() override;
        void Activate() override;
        void Deactivate() override;

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ScannerCatalogService", 0x74e0c82c));
            provided.push_back(AZ_CRC("AssetCatalogService", 0xc68ffc57));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("ScannerCatalogService", 0x74e0c82c));
            incompatible.push_back(AZ_CRC("AssetCatalogService", 0xc68ffc57));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("AssetDatabaseService", 0x3abf5601));
        }

    protected:
        AssetCatalogComponent(const AssetCatalogComponent&) = delete;
        AZStd::unique_ptr<AssetCatalog> m_catalog;

        AZStd::string m_registryFile;
    };
} // namespace AzFramework

