/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorModeFeedbackSystemComponent.h>
#include <EditorModeFeedbackFeatureProcessor.h>
#include <Pass/EditorModeFeedbackParentPass.h>
#include <Pass/EditorModeDesaturationPass.h>
#include <Pass/EditorModeTintPass.h>
#include <Pass/EditorModeBlurPass.h>
#include <Pass/EditorModeOutlinePass.h>

#include <Atom/RPI.Public/FeatureProcessorFactory.h>
#include <Atom/Utils/Utils.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/FocusMode/FocusModeInterface.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

namespace AZ
{
    namespace Render
    {
        // Creates the material for the mask pass shader
        static Data::Instance<RPI::Material> CreateMaskMaterial()
        {
            const AZStd::string path = "shaders/editormodemask.azmaterial";
            const auto materialAsset = GetAssetFromPath<RPI::MaterialAsset>(path, Data::AssetLoadBehavior::PreLoad, true);
            const auto maskMaterial = RPI::Material::FindOrCreate(materialAsset);
            return maskMaterial;
        }

        void EditorModeFeedbackSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<EditorModeFeedbackSystemComponent, AzToolsFramework::Components::EditorComponentBase>()
                    ->Version(0)
                    ;

                if (auto* editContext = serialize->GetEditContext())
                {
                    editContext->Class<EditorModeFeedbackSystemComponent>(
                        "Editor Mode Feedback System", "Manages discovery of Editor Mode Feedback effects")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
                }
            }

            EditorModeFeatureProcessor::Reflect(context);
        }

        void EditorModeFeedbackSystemComponent::Activate()
        {
            AzFramework::ApplicationRequests::Bus::Broadcast(
                [this](AzFramework::ApplicationRequests::Bus::Events* ebus)
                {
                    m_registeryEnabled = ebus->IsEditorModeFeedbackEnabled();
                });

            if (!m_registeryEnabled)
            {
                return;
            }

            AzToolsFramework::Components::EditorComponentBase::Activate();
            AzToolsFramework::ViewportEditorModeNotificationsBus::Handler::BusConnect(AzToolsFramework::GetEntityContextId());
            AZ::TickBus::Handler::BusConnect();

            AZ::Interface<EditorModeFeedbackInterface>::Register(this);
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<EditorModeFeatureProcessor>();
            auto* passSystem = RPI::PassSystemInterface::Get();
            AZ_Assert(passSystem, "Cannot get the pass system.");

            // Setup handler for load pass templates mappings
            m_loadTemplatesHandler = RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler(
                [this]()
                {
                    LoadPassTemplateMappings();
                });
           
            passSystem->AddPassCreator(Name("EditorModeFeedbackParentPass"), &EditorModeFeedbackParentPass::Create);
            passSystem->AddPassCreator(Name("EditorModeDesaturationPass"), &EditorModeDesaturationPass::Create);
            passSystem->AddPassCreator(Name("EditorModeTintPass"), &EditorModeTintPass::Create);
            passSystem->AddPassCreator(Name("EditorModeBlurPass"), &EditorModeBlurPass::Create);
            passSystem->AddPassCreator(Name("EditorModeOutlinePass"), &EditorModeOutlinePass::Create);
            passSystem->ConnectEvent(m_loadTemplatesHandler);
        }

        void EditorModeFeedbackSystemComponent::Deactivate()
        {
            if (!m_registeryEnabled)
            {
                return;
            }

            AZ::TickBus::Handler::BusDisconnect();
            AzToolsFramework::ViewportEditorModeNotificationsBus::Handler::BusDisconnect();
            AzToolsFramework::Components::EditorComponentBase::Deactivate();

            AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<EditorModeFeatureProcessor>();
            m_loadTemplatesHandler.Disconnect();
            AZ::Interface<EditorModeFeedbackInterface>::Unregister(this);
        }

        void EditorModeFeedbackSystemComponent::LoadPassTemplateMappings()
        {
            auto* passSystem = RPI::PassSystemInterface::Get();
            AZ_Assert(passSystem, "Cannot get the pass system.");

            const char* passTemplatesFile = "Passes/EditorModeFeedback_PassTemplates.azasset";
            passSystem->LoadPassTemplateMappings(passTemplatesFile);
        }

        bool EditorModeFeedbackSystemComponent::IsEnabled() const
        {
            return m_enabled && m_registeryEnabled;
        }

        void EditorModeFeedbackSystemComponent::OnEditorModeActivated(
            [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState,
            AzToolsFramework::ViewportEditorMode mode)
        {
            // Purge the draw packets for all registered 
            if (mode == AzToolsFramework::ViewportEditorMode::Focus)
            {
                if (auto* focusModeInterface = AZ::Interface<AzToolsFramework::FocusModeInterface>::Get())
                {
                    m_enabled = true;
                }
                else
                {
                    m_enabled = false;
                }
            }
        }

        void EditorModeFeedbackSystemComponent::OnEditorModeDeactivated(
            [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState,
            AzToolsFramework::ViewportEditorMode mode)
        {
            if (mode == AzToolsFramework::ViewportEditorMode::Focus)
            {
                m_focusedEntities.clear();
                m_enabled = false;
            }
        }

        void EditorModeFeedbackSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
        {
            if (!IsEnabled())
            {
                return;
            }

            const auto focusModeInterface = AZ::Interface<AzToolsFramework::FocusModeInterface>::Get();
            if (!focusModeInterface)
            {
                return;
            }

            if (!m_maskMaterial)
            {
                m_maskMaterial = CreateMaskMaterial();
            }

            const auto focusedEntityIds = focusModeInterface->GetFocusedEntities(AzToolsFramework::GetEntityContextId());
            for (const auto& focusedEntityId : focusedEntityIds)
            {
                if (auto focusedEntity = m_focusedEntities.find(focusedEntityId);
                    focusedEntity == m_focusedEntities.end())
                {
                    m_focusedEntities.emplace(AZStd::piecewise_construct, AZStd::forward_as_tuple(focusedEntityId), AZStd::forward_as_tuple(focusedEntityId, m_maskMaterial));
                }
            }

            for (auto& [entityId, focusedEntity]: m_focusedEntities)
            {
                if (focusedEntity.CanDraw())
                {
                    focusedEntity.Draw();
                }
            }
        }

        int EditorModeFeedbackSystemComponent::GetTickOrder()
        {
            return AZ::TICK_PRE_RENDER;
        }
    } // namespace Render
} // namespace AZ
