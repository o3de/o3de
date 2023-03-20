/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorModeFeedbackSystemComponent.h>
#include <EditorModeFeedbackFeatureProcessor.h>

#include <Atom/RPI.Public/FeatureProcessorFactory.h>
#include <Atom/RPI.Public/Scene.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/API/ApplicationAPI.h>

#include <AzFramework/Scene/SceneSystemInterface.h>


namespace AZ
{
    namespace Render
    {
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
            AZ::Interface<EditorModeFeedbackInterface>::Register(this);
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<EditorModeFeatureProcessor>();
            AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        }

        void EditorModeFeedbackSystemComponent::Deactivate()
        {
            if (!m_registeryEnabled)
            {
                return;
            }
            
            AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
            AzToolsFramework::Components::EditorComponentBase::Deactivate();
            AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<EditorModeFeatureProcessor>();
            AZ::Interface<EditorModeFeedbackInterface>::Unregister(this);
        }

        bool EditorModeFeedbackSystemComponent::IsEnabled() const
        {
            return m_registeryEnabled;
        }

        void EditorModeFeedbackSystemComponent::OnStartPlayInEditorBegin()
        {
            SetEnableRender(false);
        }

        void EditorModeFeedbackSystemComponent::OnStopPlayInEditor()
        {
            SetEnableRender(true);
        }

        void EditorModeFeedbackSystemComponent::SetEnableRender(bool enableRender)
        {
            auto sceneSystem = AzFramework::SceneSystemInterface::Get();
            if (!sceneSystem)
            {
                return;
            }

            AZStd::shared_ptr<AzFramework::Scene> mainScene = sceneSystem->GetScene(AzFramework::Scene::MainSceneName);
            if (!mainScene)
            {
                return;
            }

            AZ::RPI::ScenePtr* rpiScene = mainScene->FindSubsystem<AZ::RPI::ScenePtr>();
            if (!rpiScene)
            {
                return;
            }

            if (AZ::Render::EditorModeFeatureProcessor* fp = (*rpiScene)->GetFeatureProcessor<AZ::Render::EditorModeFeatureProcessor>())
            {
                fp->SetEnableRender(enableRender);
            }
        }
    } // namespace Render
} // namespace AZ
