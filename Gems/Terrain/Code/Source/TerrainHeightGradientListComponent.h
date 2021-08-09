/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Jobs/JobManagerBus.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Aabb.h>

#include <LmbrCentral/Dependency/DependencyMonitor.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <TerrainProviderBus.h>


namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Terrain
{
    class TerrainHeightGradientListConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainHeightGradientListConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(TerrainHeightGradientListConfig, "{C5FD71A9-0722-4D4C-B605-EBEBF90C628F}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        AZStd::vector<AZ::EntityId> m_gradientEntities;
    };


    class TerrainHeightGradientListComponent
        : public AZ::Component
        , private Terrain::TerrainAreaHeightRequestBus::Handler
        , private LmbrCentral::DependencyNotificationBus::Handler
    {
    public:
        template<typename, typename>
        friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(TerrainHeightGradientListComponent, "{1BB3BA6C-6D4A-4636-B542-F23ECBA8F2AB}");
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        TerrainHeightGradientListComponent(const TerrainHeightGradientListConfig& configuration);
        TerrainHeightGradientListComponent() = default;
        ~TerrainHeightGradientListComponent() = default;

        void GetHeight(const AZ::Vector3& inPosition, AZ::Vector3& outPosition, Sampler sampleFilter) override;
        void GetNormal(const AZ::Vector3& inPosition, AZ::Vector3& outNormal, Sampler sampleFilter) override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // LmbrCentral::DependencyNotificationBus
        void OnCompositionChanged() override;

    private:
        TerrainHeightGradientListConfig m_configuration;

        ///////////////////////////////////////////
        void GetHeightSynchronous(float x, float y, float& height);
        void GetNormalSynchronous(float x, float y, AZ::Vector3& normal);

        void RefreshMinMaxHeights();
        float GetHeight(float x, float y);

        float m_cachedMinHeight{ 0.0f };
        float m_cachedMaxHeight{ 0.0f };
        AZ::Vector2 m_cachedHeightQueryResolution{ 1.0f, 1.0f };
        AZ::Aabb m_cachedShapeBounds;

        LmbrCentral::DependencyMonitor m_dependencyMonitor;
    };
}
