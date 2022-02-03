/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/EditorModeFeedback/EditorEditorModeFeedbackSystemComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/FocusMode/FocusModeInterface.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

namespace AZ
{
    namespace Render
    {
        void EditorEditorModeFeedbackSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<EditorEditorModeFeedbackSystemComponent, AzToolsFramework::Components::EditorComponentBase>()
                    ->Version(0)
                    ;

                if (AZ::EditContext* ec = serialize->GetEditContext())
                {
                    ec->Class<EditorEditorModeFeedbackSystemComponent>(
                          "Editor Mode Feedback System", "Manages discovery of Editr Mode Feedback effects")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
                }
            }
        }

        void EditorEditorModeFeedbackSystemComponent::Init()
        {
            AzToolsFramework::Components::EditorComponentBase::Init();
        }

        void EditorEditorModeFeedbackSystemComponent::Activate()
        {
            AzToolsFramework::Components::EditorComponentBase::Activate();
            AzToolsFramework::ViewportEditorModeNotificationsBus::Handler::BusConnect(AzToolsFramework::GetEntityContextId());
            AZ::Interface<EditorModeFeedbackInterface>::Register(this);
        }

        void EditorEditorModeFeedbackSystemComponent::Deactivate()
        {
            AZ::Interface<EditorModeFeedbackInterface>::Unregister(this);
            AzToolsFramework::ViewportEditorModeNotificationsBus::Handler::BusDisconnect();
            AzToolsFramework::Components::EditorComponentBase::Deactivate();
        }

        bool EditorEditorModeFeedbackSystemComponent::IsEnabled() const
        {
            return m_enabled;
        }

        void EditorEditorModeFeedbackSystemComponent::OnEditorModeActivated(
            [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState,
            AzToolsFramework::ViewportEditorMode mode)
        {
            if (mode == AzToolsFramework::ViewportEditorMode::Focus)
            {
                if (auto focusModeInterface = AZ::Interface<AzToolsFramework::FocusModeInterface>::Get())
                {
                    const auto focusedEntityIds = focusModeInterface->GetFocusedEntities(AzToolsFramework::GetEntityContextId());

                    AZStd::string debugOutput;

                    debugOutput = AZStd::string::format("I have entered Focus Mode and have %zu entities:\n", focusedEntityIds.size());
                    for (const auto& focusedEntityId : focusedEntityIds)
                    {
                        debugOutput += "\t" +  focusedEntityId.ToString() + "\n";
                        const auto* focusedEntity = AzToolsFramework::GetEntityById(focusedEntityId);

                        if (focusedEntity)
                        {
                            const auto components = focusedEntity->GetComponents();

                            debugOutput += "\tName: " + focusedEntity->GetName() + "\n";

                            for (const auto& component : components)
                            {
                                AZStd::string componentName;
                                AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(
                                    componentName, &AzToolsFramework::EntityCompositionRequests::GetComponentName, component);

                                debugOutput += "\t\t" + componentName + "\n";
                            }
                        }
                    }

                    AZ_Printf("\n", debugOutput.c_str());
                }

                m_enabled = true;
            }
        }

        void EditorEditorModeFeedbackSystemComponent::OnEditorModeDeactivated(
            [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState,
            AzToolsFramework::ViewportEditorMode mode)
        {
            if (mode == AzToolsFramework::ViewportEditorMode::Focus)
            {
                m_enabled = false;
            }
        }
    }
}
