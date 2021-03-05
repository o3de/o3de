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
#include <AzFramework/Asset/FileTagAsset.h>
#include <AzFramework/Asset/GenericAssetHandler.h>
#include <AzFramework/Asset/XmlSchemaAsset.h>
#include <AzFramework/Asset/Benchmark/BenchmarkSettingsAsset.h>
#include <AzFramework/Asset/Benchmark/BenchmarkAsset.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzFramework
{
    class CustomAssetTypeComponent
        : public AZ::Component
    {
    public:

        AZ_COMPONENT(CustomAssetTypeComponent, "{E19B2FA4-60F1-4B7C-AF14-7C7A7D8DCFC2}");

        void Activate() override;
        void Deactivate() override;

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::unique_ptr<AzFramework::GenericAssetHandler<XmlSchemaAsset>> m_schemaAssetHandler;
        AZStd::unique_ptr<AzFramework::GenericAssetHandler<FileTag::FileTagAsset>> m_fileTagAssetHandler;
        AZStd::unique_ptr<AzFramework::GenericAssetHandler<BenchmarkSettingsAsset>> m_benchmarkSettingsAssetAssetHandler;
        AZStd::unique_ptr<AzFramework::GenericAssetHandler<BenchmarkAsset>> m_benchmarkAssetAssetHandler;
    };
} // namespace AzFramework

