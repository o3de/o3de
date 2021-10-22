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
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <SurfaceData/SurfaceDataTypes.h>

#include <Terrain/Ebuses/TerrainAreaSurfaceRequestBus.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Terrain
{
    class TerrainSurfaceGradientMapping final
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainSurfaceGradientMapping, AZ::SystemAllocator, 0);
        AZ_RTTI(TerrainSurfaceGradientMapping, "{473AD2CE-F22A-45A9-803F-2192F3D9F2BF}");
        static void Reflect(AZ::ReflectContext* context);

        AZ::EntityId m_gradientEntityId;
        SurfaceData::SurfaceTag m_surfaceTag;
    };

    class TerrainSurfaceGradientListConfig : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainSurfaceGradientListConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(TerrainSurfaceGradientListConfig, "{E9B083AD-8D30-47DA-8F8E-AA089BFA35E9}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        AZStd::vector<Terrain::TerrainSurfaceGradientMapping> m_gradientSurfaceMappings;
    };

    class TerrainSurfaceGradientListComponent
        : public AZ::Component
        , public Terrain::TerrainAreaSurfaceRequestBus::Handler
        , private LmbrCentral::DependencyNotificationBus::Handler
    {
    public:
        template<typename, typename>
        friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(TerrainSurfaceGradientListComponent, "{51F97C95-6B8A-4B06-B394-BD25BFCC8B7E}");
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        TerrainSurfaceGradientListComponent(const TerrainSurfaceGradientListConfig& configuration);
        TerrainSurfaceGradientListComponent() = default;
        ~TerrainSurfaceGradientListComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        // TerrainAreaSurfaceRequestBus
        void GetSurfaceWeights(const AZ::Vector3& inPosition, AzFramework::SurfaceData::SurfaceTagWeightList& outSurfaceWeights) const override;

    private:
        //////////////////////////////////////////////////////////////////////////
        // LmbrCentral::DependencyNotificationBus
        void OnCompositionChanged() override;

        TerrainSurfaceGradientListConfig m_configuration;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;
    };
} // namespace Terrain
