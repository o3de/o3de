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
#include <Atom/Feature/DiffuseGlobalIllumination/DiffuseProbeGridFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <DiffuseGlobalIllumination/DiffuseProbeGridComponentConstants.h>

namespace AZ
{
    namespace Render
    {
        class DiffuseProbeGridComponentConfig final
            : public AZ::ComponentConfig
        {
        public:
            AZ_RTTI(AZ::Render::DiffuseProbeGridComponentConfig, "{BF190F2A-D7F7-453B-9D42-5CE940180DCE}", ComponentConfig);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridComponentConfig, SystemAllocator, 0);
            static void Reflect(AZ::ReflectContext* context);

            DiffuseProbeGridComponentConfig() = default;

            Vector3 m_extents = AZ::Vector3(DefaultDiffuseProbeGridExtents);
            Vector3 m_probeSpacing = AZ::Vector3(DefaultDiffuseProbeGridSpacing);
            float m_ambientMultiplier = DefaultDiffuseProbeGridAmbientMultiplier;
            float m_viewBias = DefaultDiffuseProbeGridViewBias;
            float m_normalBias = DefaultDiffuseProbeGridNormalBias;

            DiffuseProbeGridMode m_editorMode = DiffuseProbeGridMode::RealTime;
            DiffuseProbeGridMode m_runtimeMode = DiffuseProbeGridMode::RealTime;

            AZStd::string m_bakedIrradianceTextureRelativePath;
            AZStd::string m_bakedDistanceTextureRelativePath;
            AZStd::string m_bakedRelocationTextureRelativePath;
            AZStd::string m_bakedClassificationTextureRelativePath;

            Data::Asset<RPI::StreamingImageAsset> m_bakedIrradianceTextureAsset;
            Data::Asset<RPI::StreamingImageAsset> m_bakedDistanceTextureAsset;
            Data::Asset<RPI::StreamingImageAsset> m_bakedRelocationTextureAsset;
            Data::Asset<RPI::StreamingImageAsset> m_bakedClassificationTextureAsset;

            AZ::u64 m_entityId{ EntityId::InvalidEntityId };
        };

        class DiffuseProbeGridComponentController final
            : public Data::AssetBus::MultiHandler
            , private TransformNotificationBus::Handler
            , private LmbrCentral::ShapeComponentNotificationsBus::Handler
        {
        public:
            friend class EditorDiffuseProbeGridComponent;

            AZ_CLASS_ALLOCATOR(DiffuseProbeGridComponentController, AZ::SystemAllocator, 0);
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
            void SetEditorMode(DiffuseProbeGridMode editorMode);
            void SetRuntimeMode(DiffuseProbeGridMode runtimeMode);

            // Bake the diffuse probe grid textures to assets
            void BakeTextures(DiffuseProbeGridBakeTexturesCallback callback);

            // Update the baked texture assets from the configuration
            void UpdateBakedTextures();

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
        };
    } // namespace Render
} // namespace AZ
