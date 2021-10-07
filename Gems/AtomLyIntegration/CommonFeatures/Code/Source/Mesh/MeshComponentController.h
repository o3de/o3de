/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/NonUniformScaleBus.h>

#include <AtomCore/Instance/InstanceDatabase.h>

#include <AzFramework/Render/GeometryIntersectionBus.h>
#include <AzFramework/Visibility/BoundsBus.h>

#include <Atom/RPI.Public/Model/Model.h>

#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>
#include <Atom/Feature/Material/MaterialAssignment.h>

#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>

namespace AZ
{
    namespace Render
    {
        //! A configuration structure for the MeshComponentController
        class MeshComponentConfig final
            : public AZ::ComponentConfig
        {
        public:
            AZ_RTTI(AZ::Render::MeshComponentConfig, "{63737345-51B1-472B-9355-98F99993909B}", ComponentConfig);
            AZ_CLASS_ALLOCATOR(MeshComponentConfig, SystemAllocator, 0);
            static void Reflect(AZ::ReflectContext* context);

            // Editor helper functions
            bool IsAssetSet();
            bool LodTypeIsScreenCoverage();
            bool LodTypeIsSpecificLOD();
            bool ShowLodConfig();
            AZStd::vector<AZStd::pair<RPI::Cullable::LodOverride, AZStd::string>> GetLodOverrideValues();

            Data::Asset<RPI::ModelAsset> m_modelAsset = { AZ::Data::AssetLoadBehavior::QueueLoad };
            RHI::DrawItemSortKey m_sortKey = 0;
            bool m_excludeFromReflectionCubeMaps = false;
            bool m_useForwardPassIblSpecular = false;

            RPI::Cullable::LodType m_lodType = RPI::Cullable::LodType::Default;
            RPI::Cullable::LodOverride m_lodOverride = aznumeric_cast<RPI::Cullable::LodOverride>(0);
            float m_minimumScreenCoverage = 1.0f / 1080.0f;
            float m_qualityDecayRate = 0.5f;
        };

        class MeshComponentController final
            : private MeshComponentRequestBus::Handler
            , public AzFramework::BoundsRequestBus::Handler
            , public AzFramework::RenderGeometry::IntersectionRequestBus::Handler
            , private TransformNotificationBus::Handler
            , private MaterialReceiverRequestBus::Handler
            , private MaterialComponentNotificationBus::Handler
        {
        public:
            friend class EditorMeshComponent;

            AZ_CLASS_ALLOCATOR(MeshComponentController, AZ::SystemAllocator, 0);
            AZ_RTTI(AZ::Render::MeshComponentController, "{D0F35FAC-4194-4C89-9487-D000DDB8B272}");

            ~MeshComponentController();

            static void Reflect(AZ::ReflectContext* context);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

            MeshComponentController() = default;
            MeshComponentController(const MeshComponentConfig& config);

            void Activate(const AZ::EntityComponentIdPair& entityComponentIdPair);
            void Deactivate();
            void SetConfiguration(const MeshComponentConfig& config);
            const MeshComponentConfig& GetConfiguration() const;

        private:
            AZ_DISABLE_COPY(MeshComponentController);

            // MeshComponentRequestBus::Handler overrides ...
            void SetModelAsset(Data::Asset<RPI::ModelAsset> modelAsset) override;
            Data::Asset<const RPI::ModelAsset> GetModelAsset() const override;
            void SetModelAssetId(Data::AssetId modelAssetId) override;
            Data::AssetId GetModelAssetId() const override;
            void SetModelAssetPath(const AZStd::string& modelAssetPath) override;
            AZStd::string GetModelAssetPath() const override;
            AZ::Data::Instance<RPI::Model> GetModel() const override;

            void SetSortKey(RHI::DrawItemSortKey sortKey) override;
            RHI::DrawItemSortKey GetSortKey() const override;

            void SetLodType(RPI::Cullable::LodType lodType) override;
            RPI::Cullable::LodType GetLodType() const override;

            void SetLodOverride(RPI::Cullable::LodOverride lodOverride) override;
            RPI::Cullable::LodOverride GetLodOverride() const override;

            void SetMinimumScreenCoverage(float minimumScreenCoverage) override;
            float GetMinimumScreenCoverage() const override;

            void SetQualityDecayRate(float qualityDecayRate) override;
            float GetQualityDecayRate() const override;

            void SetVisibility(bool visible) override;
            bool GetVisibility() const override;

            // BoundsRequestBus and MeshComponentRequestBus overrides ...
            AZ::Aabb GetWorldBounds() override;
            AZ::Aabb GetLocalBounds() override;

            // IntersectionRequestBus overrides ...
            AzFramework::RenderGeometry::RayResult RenderGeometryIntersect(const AzFramework::RenderGeometry::RayRequest& ray) override;

            // TransformNotificationBus::Handler overrides ...
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

            // MaterialReceiverRequestBus::Handler overrides ...
            MaterialAssignmentId FindMaterialAssignmentId(
                const MaterialAssignmentLodIndex lod, const AZStd::string& label) const override;
            RPI::ModelMaterialSlotMap GetModelMaterialSlots() const override;
            MaterialAssignmentMap GetMaterialAssignments() const override;
            AZStd::unordered_set<AZ::Name> GetModelUvNames() const override;

            // MaterialComponentNotificationBus::Handler overrides ...
            void OnMaterialsUpdated(const MaterialAssignmentMap& materials) override;

            //! Check if the model asset requires to be cloned (e.g. cloth) for unique model instances.
            //! @param modelAsset The model asset to check.
            //! @result True in case the model asset needs to be cloned before creating the model. False if there is a 1:1 relationship between
            //! the model asset and the model and it is static and shared. In the second case the m_originalModelAsset of the mesh handle is
            //! equal to the model asset that the model is linked to.
            static bool RequiresCloning(const Data::Asset<RPI::ModelAsset>& modelAsset);

            void HandleModelChange(Data::Instance<RPI::Model> model);
            void RegisterModel();
            void UnregisterModel();
            void RefreshModelRegistration();

            RPI::Cullable::LodConfiguration GetMeshLodConfiguration() const;

            void HandleNonUniformScaleChange(const AZ::Vector3& nonUniformScale);

            Render::MeshFeatureProcessorInterface* m_meshFeatureProcessor = nullptr;
            Render::MeshFeatureProcessorInterface::MeshHandle m_meshHandle;
            TransformInterface* m_transformInterface = nullptr;
            AZ::EntityComponentIdPair m_entityComponentIdPair;
            bool m_isVisible = true;
            MeshComponentConfig m_configuration;
            AZ::Vector3 m_cachedNonUniformScale = AZ::Vector3::CreateOne();

            MeshFeatureProcessorInterface::ModelChangedEvent::Handler m_changeEventHandler
            {
                [&](Data::Instance<RPI::Model> model) { HandleModelChange(model); }
            };

            AZ::NonUniformScaleChangedEvent::Handler m_nonUniformScaleChangedHandler
            {
                [&](const AZ::Vector3& nonUniformScale) { HandleNonUniformScaleChange(nonUniformScale); }
            };
        };

    } // namespace Render
} // namespace AZ
