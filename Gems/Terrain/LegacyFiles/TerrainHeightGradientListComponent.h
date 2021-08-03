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

#include <AzCore/Jobs/JobManagerBus.h>
#include <AzCore/Jobs/JobFunction.h>

#include "platform.h"
#include "ISystem.h"
#include "IRenderer.h"
#include "I3DEngine.h"
#include "ITerrain.h"
#include "MathConversion.h"


#include <Terrain/Bus/TerrainBus.h>
#include <Terrain/Bus/WorldMaterialRequestsBus.h>
#include <Terrain/Bus/TerrainProviderBus.h>
#include <Terrain/Bus/TerrainRendererBus.h>

#include <AzCore/Component/TransformBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Math/Aabb.h>

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
        , private AZ::TransformNotificationBus::Handler
        , private LmbrCentral::ShapeComponentNotificationsBus::Handler
        , private AZ::EntityBus::Handler
        , private Terrain::TerrainAreaHeightRequestBus::Handler
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

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;


        //////////////////////////////////////////////////////////////////////////
        // AZ::TransformNotificationBus::Handler
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // ShapeComponentNotificationsBus
        void OnShapeChanged(ShapeChangeReasons changeReason) override;

        ////////////////////////////////////////////////////////////////////////
        // EntityEvents
        void OnEntityActivated(const AZ::EntityId& entityId) override;
        void OnEntityDeactivated(const AZ::EntityId& entityId) override;

        void GetHeight(const AZ::Vector3& inPosition, AZ::Vector3& outPosition, Sampler sampleFilter) override;
        void GetNormal(const AZ::Vector3& inPosition, AZ::Vector3& outNormal, Sampler sampleFilter) override;

    private:
        TerrainHeightGradientListConfig m_configuration;

        ///////////////////////////////////////////
        void GetHeightSynchronous(float x, float y, float& height);
        void GetNormalSynchronous(float x, float y, AZ::Vector3& normal);
        void RefreshHeightData(bool& refresh);

        void RefreshMinMaxHeights();
        float GetHeight(float x, float y);

        AZ::Vector2 m_cachedHeightRange;
        float m_cachedHeightQueryResolution{ 1.0f };
        AZ::Aabb m_cachedShapeBounds;
        bool m_refreshHeightData{ true };
    };
}
