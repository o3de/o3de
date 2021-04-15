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
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/NonUniformScaleBus.h>

#include <AtomCore/Instance/InstanceDatabase.h>

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
        /**
         * A configuration structure for the MeshComponentController
         */
        class MeshComponentConfig final
            : public AZ::ComponentConfig
        {
        public:
            AZ_RTTI(AZ::Render::MeshComponentConfig, "{63737345-51B1-472B-9355-98F99993909B}", ComponentConfig);
            AZ_CLASS_ALLOCATOR(MeshComponentConfig, SystemAllocator, 0);
            static void Reflect(AZ::ReflectContext* context);

            // Editor helper functions
            bool IsAssetSet();
            AZStd::vector<AZStd::pair<RPI::Cullable::LodOverride, AZStd::string>> GetLodOverrideValues();

            Data::Asset<RPI::ModelAsset> m_modelAsset = { AZ::Data::AssetLoadBehavior::QueueLoad };
            RHI::DrawItemSortKey m_sortKey = 0;
            RPI::Cullable::LodOverride m_lodOverride = RPI::Cullable::NoLodOverride;
            bool m_excludeFromReflectionCubeMaps = false;
            bool m_useForwardPassIblSpecular = false;
        };

        class MeshComponentController final
            : private MeshComponentRequestBus::Handler
            , public AzFramework::BoundsRequestBus::Handler
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

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const MeshComponentConfig& config);
            const MeshComponentConfig& GetConfiguration() const;

        private:

            AZ_DISABLE_COPY(MeshComponentController);

            // MeshComponentRequestBus::Handler overrides ...
            void SetModelAsset(Data::Asset<RPI::ModelAsset> modelAsset) override;
            const Data::Asset<RPI::ModelAsset>& GetModelAsset() const override;
            void SetModelAssetId(Data::AssetId modelAssetId) override;
            Data::AssetId GetModelAssetId() const override;
            void SetModelAssetPath(const AZStd::string& modelAssetPath) override;
            AZStd::string GetModelAssetPath() const override;
            const AZ::Data::Instance<RPI::Model> GetModel() const override;

            void SetSortKey(RHI::DrawItemSortKey sortKey) override;
            RHI::DrawItemSortKey GetSortKey() const override;

            void SetLodOverride(RPI::Cullable::LodOverride lodOverride) override;
            RPI::Cullable::LodOverride GetLodOverride() const override;

            void SetVisibility(bool visible) override;
            bool GetVisibility() const override;

            // BoundsRequestBus and MeshComponentRequestBus ...
            AZ::Aabb GetWorldBounds() override;
            AZ::Aabb GetLocalBounds() override;

            // TransformNotificationBus::Handler overrides ...
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

            // MaterialReceiverRequestBus::Handler overrides ...
            MaterialAssignmentMap GetMaterialAssignments() const override;
            AZStd::unordered_set<AZ::Name> GetModelUvNames() const override;

            // MaterialComponentNotificationBus::Handler overrides ...
            void OnMaterialsUpdated(const MaterialAssignmentMap& materials) override;

            void HandleModelChange(Data::Instance<RPI::Model> model);
            void RegisterModel();
            void UnregisterModel();
            void RefreshModelRegistration();

            void HandleNonUniformScaleChange(const AZ::Vector3& nonUniformScale);
            void UpdateOverallMatrix();

            Render::MeshFeatureProcessorInterface* m_meshFeatureProcessor = nullptr;
            Render::MeshFeatureProcessorInterface::MeshHandle m_meshHandle;
            TransformInterface* m_transformInterface = nullptr;
            AZ::EntityId m_entityId;
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
