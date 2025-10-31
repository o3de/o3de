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
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Visibility/BoundsBus.h>
#include <Atom/Feature/ReflectionProbe/ReflectionProbeFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <ReflectionProbe/ReflectionProbeComponentConstants.h>
#include <CubeMapCapture/EditorCubeMapRenderer.h>

namespace AZ
{
    namespace Render
    {
        class ReflectionProbeComponentConfig final
            : public AZ::ComponentConfig
        {
        public:
            AZ_RTTI(AZ::Render::ReflectionProbeComponentConfig, "{D61730A1-CAF5-448C-B2A3-50D5DC909F31}", ComponentConfig);
            AZ_CLASS_ALLOCATOR(ReflectionProbeComponentConfig, SystemAllocator);
            static void Reflect(AZ::ReflectContext* context);

            float m_outerHeight = DefaultReflectionProbeExtents;
            float m_outerLength = DefaultReflectionProbeExtents;
            float m_outerWidth = DefaultReflectionProbeExtents;
            float m_innerHeight = DefaultReflectionProbeExtents;
            float m_innerLength = DefaultReflectionProbeExtents;
            float m_innerWidth = DefaultReflectionProbeExtents;

            bool m_useParallaxCorrection = true;
            bool m_showVisualization = true;
            bool m_useBakedCubemap = true;

            CubeMapSpecularQualityLevel m_bakedCubeMapQualityLevel = CubeMapSpecularQualityLevel::Medium;
            AZStd::string m_bakedCubeMapRelativePath;
            Data::Asset<RPI::StreamingImageAsset> m_bakedCubeMapAsset;
            Data::Asset<RPI::StreamingImageAsset> m_authoredCubeMapAsset;
            AZ::u64 m_entityId{ EntityId::InvalidEntityId };

            float m_renderExposure = 0.0f;
            float m_bakeExposure = 0.0f;
        };

        class ReflectionProbeComponentController final
            : public Data::AssetBus::MultiHandler
            , private TransformNotificationBus::Handler
            , private LmbrCentral::ShapeComponentNotificationsBus::Handler
            , public AzFramework::BoundsRequestBus::Handler
        {
        public:
            friend class EditorReflectionProbeComponent;

            AZ_CLASS_ALLOCATOR(ReflectionProbeComponentController, AZ::SystemAllocator);
            AZ_RTTI(AZ::Render::ReflectionProbeComponentController, "{EFFA88F1-7ED2-4552-B6F6-5E6B2B6D9311}");

            static void Reflect(AZ::ReflectContext* context);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            ReflectionProbeComponentController() = default;
            ReflectionProbeComponentController(const ReflectionProbeComponentConfig& config);

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const ReflectionProbeComponentConfig& config);
            const ReflectionProbeComponentConfig& GetConfiguration() const;

            // returns the outer extent Aabb for this reflection
            AZ::Aabb GetAabb() const;

            // set the exposure to use when baking the cubemap
            void SetBakeExposure(float bakeExposure);

            // initiate the reflection probe bake, invokes callback when complete
            void BakeReflectionProbe(BuildCubeMapCallback callback, const AZStd::string& relativePath);

            // update the currently rendering cubemap asset for this probe
            void UpdateCubeMap();

            // BoundsRequestBus overrides ...
            AZ::Aabb GetWorldBounds() const override;
            AZ::Aabb GetLocalBounds() const override;

            void RegisterInnerExtentsChangedHandler(AZ::Event<bool>::Handler& handler);

        private:

            AZ_DISABLE_COPY(ReflectionProbeComponentController);

            // Data::AssetBus overrides ...
            void OnAssetReady(Data::Asset<Data::AssetData> asset) override;
            void OnAssetReloaded(Data::Asset<Data::AssetData> asset) override;

            // TransformNotificationBus overrides ...
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

            // ShapeComponentNotificationsBus overrides ...
            void OnShapeChanged(ShapeChangeReasons changeReason) override;

            // update the feature processor and configuration outer extents
            void UpdateOuterExtents();

            // computes the effective transform taking both the entity transform and the shape translation offset into account
            AZ::Transform ComputeOverallTransform(const AZ::Transform& entityTransform) const;

            // box shape component, used for defining the outer extents of the probe area
            LmbrCentral::BoxShapeComponentRequests* m_boxShapeInterface = nullptr;
            LmbrCentral::ShapeComponentRequests* m_shapeBus = nullptr;

            // handle for this probe in the feature processor
            ReflectionProbeHandle m_handle;

            ReflectionProbeFeatureProcessorInterface* m_featureProcessor = nullptr;
            TransformInterface* m_transformInterface = nullptr;
            AZ::EntityId m_entityId;
            ReflectionProbeComponentConfig m_configuration;

            // event fired when the inner extents change
            AZ::Event<bool> m_innerExtentsChangedEvent;
        };
    } // namespace Render
} // namespace AZ
