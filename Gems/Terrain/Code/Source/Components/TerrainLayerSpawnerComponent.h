/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Math/Vector3.h>

#include <AzCore/Jobs/JobManagerBus.h>
#include <AzCore/Jobs/JobFunction.h>

#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <TerrainSystem/TerrainSystemBus.h>

#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <AzCore/Math/Aabb.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Terrain
{
    namespace AreaConstants
    {
        static const uint32_t s_backgroundLayer = 0;
        static const uint32_t s_foregroundLayer = 1;
        static const int32_t s_priorityMin = -10000;
        static const int32_t s_priorityMax = 10000;
        static const int32_t s_prioritySoftMin = -100; //design specified slider range
        static const int32_t s_prioritySoftMax = 100; //design specified slider range
    }

    class TerrainLayerSpawnerConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainLayerSpawnerConfig, AZ::SystemAllocator);
        AZ_RTTI(TerrainLayerSpawnerConfig, "{8E0695DE-E843-4858-BAEA-70953E74C810}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        AZStd::vector<AZStd::pair<AZ::u32, AZStd::string>> GetSelectableLayers() const;
        uint32_t m_layer = AreaConstants::s_foregroundLayer;
        int32_t m_priority = 0;
        bool m_useGroundPlane = true;
    };

    inline constexpr AZ::TypeId TerrainLayerSpawnerComponentTypeId{ "{3848605F-A4EA-478C-B710-84AB8DCA9EC5}" };

    class TerrainLayerSpawnerComponent
        : public AZ::Component
        , private LmbrCentral::ShapeComponentNotificationsBus::Handler
        , private Terrain::TerrainSpawnerRequestBus::Handler
    {
    public:
        template<typename, typename>
        friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(TerrainLayerSpawnerComponent, TerrainLayerSpawnerComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        TerrainLayerSpawnerComponent(const TerrainLayerSpawnerConfig& configuration);
        TerrainLayerSpawnerComponent() = default;
        ~TerrainLayerSpawnerComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

    protected:
        // ShapeComponentNotificationsBus
        void OnShapeChanged(ShapeChangeReasons changeReason) override;

        // TerrainSpawnerRequestBus
        void GetPriority(uint32_t& outLayer, int32_t& outPriority) override;
        bool GetUseGroundPlane() override;
        
        void RefreshArea();

    private:
        TerrainLayerSpawnerConfig m_configuration;
    };
}
