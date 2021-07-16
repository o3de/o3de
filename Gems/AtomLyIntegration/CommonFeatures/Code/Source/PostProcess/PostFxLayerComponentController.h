/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/PostFxLayerComponentConfig.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/PostFxLayerBus.h>
#include <AzCore/Component/TickBus.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>
#include <LmbrCentral/Scripting/TagComponentBus.h>
#include <AzFramework/Components/CameraBus.h>

namespace AZ
{
    namespace Render
    {
        class PostFxLayerComponentController final
            : public PostFxLayerRequestBus::Handler
            , public LmbrCentral::TagGlobalNotificationBus::MultiHandler
            , public Camera::CameraNotificationBus::Handler
            , public TickBus::Handler
        {
        public:
            friend class EditorPostFxLayerComponent;

            AZ_TYPE_INFO(AZ::Render::PostFxLayerComponentController, "{A3285A02-944B-4339-95B1-15E0F410BD1D}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

            PostFxLayerComponentController() = default;
            PostFxLayerComponentController(const PostFxLayerComponentConfig& config);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const PostFxLayerComponentConfig& config);
            const PostFxLayerComponentConfig& GetConfiguration() const;

            // Called whenever a tag is added to this component via Editor or Script
            void RebuildCameraEntitiesList();

            // TagGlobalNotificationBus::MultiHandler overrides
            void OnEntityTagAdded(const AZ::EntityId& entityId) override;
            void OnEntityTagRemoved(const AZ::EntityId& entityId) override;

            // CameraNotificationBus::Handler overrides
            void OnCameraAdded(const AZ::EntityId& cameraId) override;
            void OnCameraRemoved(const AZ::EntityId& cameraId) override;

            // Auto-gen function override declarations (functions definitions in .cpp)...
#include <Atom/Feature/ParamMacros/StartParamFunctionsOverride.inl>
#include <Atom/Feature/PostProcess/PostProcessParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:
            AZ_DISABLE_COPY(PostFxLayerComponentController);

            void OnConfigChanged();

            void OnTick(float deltaTime, ScriptTimePoint time) override;

            // Connects to all camera tags listed in this component
            void BusConnectToTags();

            const AZStd::unordered_set<AZ::EntityId>& GetCameraEntityList() const;
            bool IsEditorView(const AZ::RPI::ViewPtr view);
            bool HasTags(const AZ::EntityId& entityId, const AZStd::vector<AZStd::string>& tags) const;

            // list of entities containing tags set in this component's property.
            AZStd::unordered_set<AZ::EntityId> m_taggedCameraEntities;
            // a list of cameras tracked by this component. This is used if no camera tags are specified.
            AZStd::unordered_set<AZ::EntityId> m_cameraEntities;
            // a list of camera views in the scene. This is used to test if a view is an editor view.
            AZStd::unordered_set<AZ::RPI::View*> m_allCameraViews;

            PostProcessFeatureProcessorInterface* m_featureProcessorInterface = nullptr;
            PostProcessSettingsInterface* m_postProcessInterface = nullptr;
            PostFxLayerComponentConfig m_configuration;
            EntityId m_entityId;
        };
    }
}
