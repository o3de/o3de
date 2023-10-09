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
#include <DiffuseProbeGrid/DiffuseProbeGridFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <Components/DiffuseProbeGridComponentConstants.h>

namespace AZ
{
    namespace Render
    {
        class DiffuseProbeGridComponentConfig final
            : public AZ::ComponentConfig
        {
        public:
            AZ_RTTI(AZ::Render::DiffuseProbeGridComponentConfig, "{BF190F2A-D7F7-453B-9D42-5CE940180DCE}", ComponentConfig);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridComponentConfig, SystemAllocator);
            static void Reflect(AZ::ReflectContext* context);

            DiffuseProbeGridComponentConfig() = default;

            Vector3 m_extents = AZ::Vector3(DefaultDiffuseProbeGridExtents);
            Vector3 m_probeSpacing = AZ::Vector3(DefaultDiffuseProbeGridSpacing);
            float m_ambientMultiplier = DefaultDiffuseProbeGridAmbientMultiplier;
            float m_viewBias = DefaultDiffuseProbeGridViewBias;
            float m_normalBias = DefaultDiffuseProbeGridNormalBias;
            DiffuseProbeGridNumRaysPerProbe m_numRaysPerProbe = DefaultDiffuseProbeGridNumRaysPerProbe;
            bool m_scrolling = false;
            bool m_edgeBlendIbl = true;
            uint32_t m_frameUpdateCount = 1;
            DiffuseProbeGridTransparencyMode m_transparencyMode = DefaultDiffuseProbeGridTransparencyMode;
            float m_emissiveMultiplier = DefaultDiffuseProbeGridEmissiveMultiplier;

            DiffuseProbeGridMode m_editorMode = DiffuseProbeGridMode::RealTime;
            DiffuseProbeGridMode m_runtimeMode = DiffuseProbeGridMode::RealTime;

            AZStd::string m_bakedIrradianceTextureRelativePath;
            AZStd::string m_bakedDistanceTextureRelativePath;
            AZStd::string m_bakedProbeDataTextureRelativePath;

            Data::Asset<RPI::StreamingImageAsset> m_bakedIrradianceTextureAsset;
            Data::Asset<RPI::StreamingImageAsset> m_bakedDistanceTextureAsset;
            Data::Asset<RPI::StreamingImageAsset> m_bakedProbeDataTextureAsset;

            bool m_visualizationEnabled = false;
            bool m_visualizationShowInactiveProbes = false;
            float m_visualizationSphereRadius = DefaultVisualizationSphereRadius;

            AZ::u64 m_entityId{ EntityId::InvalidEntityId };
        };

        class DiffuseProbeGridComponentController final
            : public Data::AssetBus::MultiHandler
            , private TransformNotificationBus::Handler
            , private LmbrCentral::ShapeComponentNotificationsBus::Handler
        {
        public:
            friend class EditorDiffuseProbeGridComponent;

            AZ_CLASS_ALLOCATOR(DiffuseProbeGridComponentController, AZ::SystemAllocator);
            AZ_RTTI(AZ::Render::DiffuseProbeGridComponentController, "{108588E8-355E-4A19-94AC-955E64A37CE2}");

            static void Reflect(AZ::ReflectContext* context);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            DiffuseProbeGridComponentController() = default;
            DiffuseProbeGridComponentController(const DiffuseProbeGridComponentConfig& config);

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const DiffuseProbeGridComponentConfig& config);
            const DiffuseProbeGridComponentConfig& GetConfiguration() const;

            // returns the Aabb for this grid
            AZ::Aabb GetAabb() const;

            void RegisterBoxChangedByGridHandler(AZ::Event<bool>::Handler& handler);

        private:

            AZ_DISABLE_COPY(DiffuseProbeGridComponentController);

            // TransformNotificationBus overrides
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

            // ShapeComponentNotificationsBus overrides
            void OnShapeChanged(ShapeChangeReasons changeReason) override;

            // AssetBus overrides
            void OnAssetReady(Data::Asset<Data::AssetData> asset) override;
            void OnAssetError(Data::Asset<Data::AssetData> asset) override;

            // Property handlers
            bool ValidateProbeSpacing(const AZ::Vector3& newSpacing);
            void SetProbeSpacing(const AZ::Vector3& probeSpacing);
            void SetAmbientMultiplier(float ambientMultiplier);
            void SetViewBias(float viewBias);
            void SetNormalBias(float normalBias);
            void SetNumRaysPerProbe(const DiffuseProbeGridNumRaysPerProbe& numRaysPerProbe);
            void SetScrolling(bool scrolling);
            void SetEdgeBlendIbl(bool edgeBlendIbl);
            void SetFrameUpdateCount(uint32_t frameUpdateCount);
            void SetTransparencyMode(DiffuseProbeGridTransparencyMode transparencyMode);
            void SetEmissiveMultiplier(float emissiveMultiplier);
            void SetEditorMode(DiffuseProbeGridMode editorMode);
            void SetRuntimeMode(DiffuseProbeGridMode runtimeMode);
            void SetVisualizationEnabled(bool visualizationEnabled);
            void SetVisualizationShowInactiveProbes(bool visualizationShowInactiveProbes);
            void SetVisualizationSphereRadius(float visualizationSphereRadius);

            // Bake the diffuse probe grid textures to assets
            void BakeTextures(DiffuseProbeGridBakeTexturesCallback callback);
            bool CanBakeTextures();

            // Update the baked texture assets from the configuration
            void UpdateBakedTextures();

            // Computes the effective transform taking both the entity transform and the shape translation offset into account
            AZ::Transform ComputeOverallTransform(const AZ::Transform& entityTransform) const;

            // box shape component, used for defining the outer extents of the probe area
            LmbrCentral::BoxShapeComponentRequests* m_boxShapeInterface = nullptr;
            LmbrCentral::ShapeComponentRequests* m_shapeBus = nullptr;

            // handle for this probe in the feature processor
            DiffuseProbeGridHandle m_handle;

            DiffuseProbeGridFeatureProcessorInterface* m_featureProcessor = nullptr;
            TransformInterface* m_transformInterface = nullptr;
            AZ::EntityId m_entityId;
            DiffuseProbeGridComponentConfig m_configuration;
            bool m_inShapeChangeHandler = false;

            // event for the diffuse probe grid modifying the underlying box dimensions
            AZ::Event<bool> m_boxChangedByGridEvent;
        };
    } // namespace Render
} // namespace AZ
