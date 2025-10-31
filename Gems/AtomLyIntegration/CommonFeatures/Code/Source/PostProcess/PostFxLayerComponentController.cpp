/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/PostFxLayerComponentController.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/PostFxWeightRequestBus.h>
#include <LmbrCentral/Scripting/TagComponentBus.h>
#include <LmbrCentral/Scripting/EditorTagComponentBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/ViewProviderBus.h>
#include <Atom/RPI.Public/ViewportContextBus.h>

namespace AZ
{
    namespace Render
    {
        void PostFxLayerComponentController::Reflect(ReflectContext* context)
        {
            PostFxLayerComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<PostFxLayerComponentController>()
                    ->Version(0)
                    ->Field("Configuration", &PostFxLayerComponentController::m_configuration);
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<PostFxLayerRequestBus>("PostFxLayerRequestBus")

                    // Auto-gen behavior context...
#define PARAM_EVENT_BUS PostFxLayerRequestBus::Events
#include <Atom/Feature/ParamMacros/StartParamBehaviorContext.inl>
#include <Atom/Feature/PostProcess/PostProcessParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
#undef PARAM_EVENT_BUS

                    ;
            }
        }

        void PostFxLayerComponentController::OnTick(float deltaTime, ScriptTimePoint time)
        {
            AZ_UNUSED(deltaTime);
            AZ_UNUSED(time);

            // get all camera entities if no tags were set in the PostFx layer
            const AZStd::unordered_set<AZ::EntityId>& cameraEntityList = GetCameraEntityList();

            // Get views of each camera
            AZStd::unordered_set<AZ::RPI::View*> allSceneViews;
            for (const AZ::EntityId& cameraEntityId : cameraEntityList)
            {

                for (uint32_t i = 0; i < AZ::RPI::MaxViewTypes; i++)
                {
                    if (i == AZ::RPI::DefaultViewType)
                    {
                        // Get the view pointer associated to each camera entity
                        AZ::RPI::ViewPtr view = nullptr;
                        AZ::RPI::ViewProviderBus::EventResult(view, cameraEntityId, &AZ::RPI::ViewProvider::GetView);

                        if (view != nullptr)
                        {
                            allSceneViews.insert(view.get());
                        }
                    }
                    else
                    {
                        AZ::RPI::ViewPtr stereoscopicView = nullptr;
                        AZ::RPI::ViewProviderBus::EventResult(
                            stereoscopicView, cameraEntityId, &AZ::RPI::ViewProvider::GetStereoscopicView, static_cast<AZ::RPI::ViewType>(i));

                        if (stereoscopicView != nullptr)
                        {
                            allSceneViews.insert(stereoscopicView.get());
                        }
                    }
                }
            }

            // Add the current view which can potentially be the editor view
            auto atomViewportRequests = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
            const AZ::Name contextName = atomViewportRequests->GetDefaultViewportContextName();
            auto currentView = atomViewportRequests->GetCurrentViewGroup(contextName)->GetView();
            if (IsEditorView(currentView))
            {
                allSceneViews.insert(currentView.get());
            }

            // calculate blend weights for all cameras
            PostProcessSettingsInterface::ViewBlendWeightMap perViewBlendWeights;
            for (auto view : allSceneViews)
            {
                if (view)
                {
                    // get the view position
                    AZ::Vector3 viewPosition = view->GetViewToWorldMatrix().GetTranslation();

                    // calculate blend weight based on proximity to weight modifier
                    float blendWeightResult = 1.0f;
                    AZ::Render::PostFxWeightRequestBus::EventResult(
                        blendWeightResult,
                        m_entityId,
                        &AZ::Render::PostFxWeightRequests::GetWeightAtPosition,
                        viewPosition
                    );

                    // store result
                    perViewBlendWeights[view] = blendWeightResult;
                }
            }

            // copy cameraToBlendWeight data to settings
            if (m_postProcessInterface)
            {
                m_postProcessInterface = m_featureProcessorInterface->GetOrCreateSettingsInterface(m_entityId);
                m_postProcessInterface->CopyViewToBlendWeightSettings(perViewBlendWeights);
                m_postProcessInterface->OnConfigChanged();
            }
        }

        void PostFxLayerComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("PostFXLayerService"));
        }

        void PostFxLayerComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("PostFXLayerService"));
        }

        PostFxLayerComponentController::PostFxLayerComponentController(const PostFxLayerComponentConfig& config)
            : m_configuration(config)
        {
        }

        void PostFxLayerComponentController::OnEntityTagAdded(const AZ::EntityId& entityId)
        {
            // If the entity contains an exclusion tag, do not add it to the tagged camera entities
            if (!HasTags(entityId, m_configuration.m_exclusionTags))
            {
                m_taggedCameraEntities.insert(entityId);
            }
        }

        void PostFxLayerComponentController::OnEntityTagRemoved(const AZ::EntityId& entityId)
        {
            m_taggedCameraEntities.erase(entityId);  
        }

        void PostFxLayerComponentController::OnCameraAdded(const AZ::EntityId& cameraId)
        {
            // If the entity contains an exclusion tag, do not add it to the camera entities
            if (!HasTags(cameraId, m_configuration.m_exclusionTags))
            {
                m_cameraEntities.insert(cameraId);
            }

            AZ::RPI::ViewPtr view = nullptr;
            AZ::RPI::ViewProviderBus::EventResult(view, cameraId, &AZ::RPI::ViewProvider::GetView);
            if (view != nullptr)
            {
                m_allCameraViews.insert(view.get());
            }
        }

        void PostFxLayerComponentController::OnCameraRemoved(const AZ::EntityId& cameraId)
        {
            m_cameraEntities.erase(cameraId);
        }

        void PostFxLayerComponentController::RebuildCameraEntitiesList()
        {
            // Reconnect buses to regenerate the entities list
            m_taggedCameraEntities.clear();
            BusConnectToTags();

            m_cameraEntities.clear();
            Camera::CameraNotificationBus::Handler::BusDisconnect();
            Camera::CameraNotificationBus::Handler::BusConnect();
        }

        const AZStd::unordered_set<AZ::EntityId>& PostFxLayerComponentController::GetCameraEntityList() const
        {
            if (m_configuration.m_cameraTags.empty())
            {
                return m_cameraEntities;
            }
            else
            {
                return m_taggedCameraEntities;
            }
        }

        bool PostFxLayerComponentController::IsEditorView(const AZ::RPI::ViewPtr view)
        {
            return m_allCameraViews.find(view.get()) == m_allCameraViews.end() ? true : false;
        }

        bool PostFxLayerComponentController::HasTags(const AZ::EntityId& entityId, const AZStd::vector<AZStd::string>& tags) const
        {
            bool hasTag = false;
            for (auto tag : tags)
            {
                LmbrCentral::TagComponentRequestBus::EventResult(
                    hasTag,
                    entityId,
                    &LmbrCentral::TagComponentRequests::HasTag,
                    LmbrCentral::Tag(tag)
                );
                LmbrCentral::EditorTagComponentRequestBus::EventResult(
                    hasTag,
                    entityId,
                    &LmbrCentral::EditorTagComponentRequests::HasTag,
                    tag.c_str()
                );

                if (hasTag)
                {
                    return true;
                }
            }

            return false;
        }

        void PostFxLayerComponentController::BusConnectToTags()
        {
            LmbrCentral::TagGlobalNotificationBus::MultiHandler::BusDisconnect();
            for (auto tag : m_configuration.m_cameraTags)
            {
                LmbrCentral::TagGlobalNotificationBus::MultiHandler::BusConnect(LmbrCentral::Tag(tag));
            }
        }
        
        void PostFxLayerComponentController::Activate(EntityId entityId)
        {
            m_entityId = entityId;

            m_featureProcessorInterface = RPI::Scene::GetFeatureProcessorForEntity<PostProcessFeatureProcessorInterface>(m_entityId);
            if (m_featureProcessorInterface)
            {
                m_postProcessInterface = m_featureProcessorInterface->GetOrCreateSettingsInterface(m_entityId);
                m_configuration.CopySettingsTo(m_postProcessInterface);
            }

            BusConnectToTags();
            Camera::CameraNotificationBus::Handler::BusConnect();
            PostFxLayerRequestBus::Handler::BusConnect(m_entityId);
            AZ::TickBus::Handler::BusConnect();
        }

        void PostFxLayerComponentController::Deactivate()
        {
            AZ::TickBus::Handler::BusDisconnect();
            PostFxLayerRequestBus::Handler::BusDisconnect(m_entityId);
            Camera::CameraNotificationBus::Handler::BusDisconnect();
            LmbrCentral::TagGlobalNotificationBus::MultiHandler::BusDisconnect();

            if (m_featureProcessorInterface)
            {
                m_featureProcessorInterface->RemoveSettingsInterface(m_entityId);
            }
            m_postProcessInterface = nullptr;
            m_featureProcessorInterface = nullptr;

            m_entityId.SetInvalid();
        }

        void PostFxLayerComponentController::SetConfiguration(const PostFxLayerComponentConfig& config)
        {
            m_configuration = config;
        }

        const PostFxLayerComponentConfig& PostFxLayerComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        void PostFxLayerComponentController::OnConfigChanged()
        {
            if (m_postProcessInterface)
            {
                m_configuration.CopySettingsTo(m_postProcessInterface);
                m_postProcessInterface->OnConfigChanged();
            }
        }

        // Auto-gen getter/setter function definitions...
        // The setter functions will set the values on the Atom settings class, then get the value back
        // from the settings class to set the local configuration. This is in case the settings class
        // applies some custom logic that results in the set value being different from the input
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                          \
                                                                                                                \
        ValueType PostFxLayerComponentController::Get##Name() const                                             \
        {                                                                                                       \
            return m_configuration.MemberName;                                                                  \
        }                                                                                                       \
        void PostFxLayerComponentController::Set##Name(ValueType val)                                           \
        {                                                                                                       \
            if(m_postProcessInterface)                                                                          \
            {                                                                                                   \
                m_postProcessInterface->Set##Name(val);                                                         \
                m_postProcessInterface->OnConfigChanged();                                                      \
                m_configuration.MemberName = m_postProcessInterface->Get##Name();                               \
            }                                                                                                   \
            else                                                                                                \
            {                                                                                                   \
                m_configuration.MemberName = val;                                                               \
            }                                                                                                   \
        }                                                                                                       \

#define AZ_GFX_COMMON_OVERRIDE(ValueType, Name, MemberName, OverrideValueType)                                  \
                                                                                                                \
        OverrideValueType PostFxLayerComponentController::Get##Name##Override() const                           \
        {                                                                                                       \
            return m_configuration.MemberName##Override;                                                        \
        }                                                                                                       \
        void PostFxLayerComponentController::Set##Name##Override(OverrideValueType val)                         \
        {                                                                                                       \
            m_configuration.MemberName = val;                                                                   \
            if(m_postProcessInterface)                                                                          \
            {                                                                                                   \
                m_postProcessInterface->Set##Name##Override(val);                                               \
                m_postProcessInterface->OnConfigChanged();                                                      \
            }                                                                                                   \
        }                                                                                                       \

#include <Atom/Feature/ParamMacros/MapAllCommon.inl>
#include <Atom/Feature/PostProcess/PostProcessParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
    } // namespace Render
} // namespace AZ
