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
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzFramework/Spawnable/SpawnableAssetHandler.h>

namespace AzFramework
{
    class SpawnableSystemComponent
        : public AZ::Component
        , public AssetCatalogEventBus::Handler
    {
    public:
        AZ_COMPONENT(SpawnableSystemComponent, "{12D0DA52-BB86-4AC3-8862-9493E0D0E207}");

        inline static constexpr const char* RootSpawnableRegistryKey = "/Amazon/AzCore/Bootstrap/RootSpawnable";
            
        SpawnableSystemComponent() = default;
        SpawnableSystemComponent(const SpawnableSystemComponent&) = delete;
        SpawnableSystemComponent(SpawnableSystemComponent&&) = delete;

        SpawnableSystemComponent& operator=(const SpawnableSystemComponent&) = delete;
        SpawnableSystemComponent& operator=(SpawnableSystemComponent&&) = delete;

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services);

        //
        // AssetCatalogEventBus
        //

        void OnCatalogLoaded(const char* catalogFile) override;

    protected:
        void Activate() override;
        void Deactivate() override;

        SpawnableAssetHandler m_assetHandler;
        AZ::Data::Asset<Spawnable> m_rootSpawnable;
        bool m_rootSpawnableInitialized{ false };
    };
} // namespace AzFramework
