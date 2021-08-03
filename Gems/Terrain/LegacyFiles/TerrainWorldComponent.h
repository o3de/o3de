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

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Math/Vector3.h>
#include <Terrain/TerrainProvider.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Terrain
{
    struct TerrainRenderNode;

    class TerrainWorldConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainWorldConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(TerrainWorldConfig, "{295844DB-20DD-45B2-94DB-4245D5AE9AFF}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        AZ::Vector3 m_worldMin{ 0.0f, 0.0f, 0.0f };
        AZ::Vector3 m_worldMax{ 4096.0f, 4096.0f, 2048.0f };
        AZ::Vector3 m_regionBounds{ 2048.0f, 2048.0f, 2048.0f };
        float m_heightmapCellSize = { 1.0f };
        AZStd::string m_worldMaterialAssetName = { "Terrain/default_world.worldmat" };
    };


    class TerrainWorldComponent
        : public AZ::Component
        , private AZ::Data::AssetBus::MultiHandler
    {
    public:
        template<typename, typename>
        friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(TerrainWorldComponent, "{4734EFDC-135D-4BF5-BE57-4F9AD03ADF78}");
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        TerrainWorldComponent(const TerrainWorldConfig& configuration);
        TerrainWorldComponent() = default;
        ~TerrainWorldComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Data::AssetBus::Handler
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

    private:
        void LoadAssets();
        bool IsFullyLoaded() const;

        TerrainWorldConfig m_configuration;
        TerrainProvider* m_terrainProvider{ nullptr };
        TerrainRenderNode* m_terrainRenderNode{ nullptr };
    };
}
