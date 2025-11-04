/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyAudioCtrlTypes.h>

#include <Integration/Components/AnimAudioComponent.h>

#include <IAudioSystem.h>


namespace EMotionFX
{
    namespace Integration
    {
        struct EditorAudioTriggerEvent
        {
            AZStd::string m_event;
            AzToolsFramework::CReflectedVarAudioControl m_trigger;
            AZStd::string m_joint;

            AZ_RTTI(EditorAudioTriggerEvent, "{AA4D9F3A-F6C1-4E92-961F-E1D9DE11AD06}");
            AZ_CLASS_ALLOCATOR(EditorAudioTriggerEvent, EMotionFXAllocator);

            EditorAudioTriggerEvent()
            {
                m_trigger.m_propertyType = AzToolsFramework::AudioPropertyType::Trigger;
            }

            virtual ~EditorAudioTriggerEvent() = default;

            static void Reflect(AZ::ReflectContext* context)
            {
                if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<EditorAudioTriggerEvent>()
                        ->Version(0)
                        ->Field("event", &EditorAudioTriggerEvent::m_event)
                        ->Field("trigger", &EditorAudioTriggerEvent::m_trigger)
                        ->Field("joint", &EditorAudioTriggerEvent::m_joint);

                    if (auto editContext = serializeContext->GetEditContext())
                    {
                        editContext->Class<EditorAudioTriggerEvent>("Audio Trigger Event", "Audio trigger executed when animation event occurs")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::HideIcon, true)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &EditorAudioTriggerEvent::m_event, "Event", "EMotionFX event.")
                            ->DataElement("AudioControl", &EditorAudioTriggerEvent::m_trigger, "Trigger", "Audio trigger to execute.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &EditorAudioTriggerEvent::m_joint, "Joint", "Mesh joint (optional).");
                    }
                }
            }
        };

        class EditorAnimAudioComponent
            : public AzToolsFramework::Components::EditorComponentBase
        {
        public:
            AZ_EDITOR_COMPONENT(EditorAnimAudioComponent, "{DF2320B2-97E8-40C4-86C5-C3327D0DA3E6}");

            virtual ~EditorAnimAudioComponent() = default;

        protected:
            void BuildGameEntity(AZ::Entity* gameEntity) override;

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                AnimAudioComponent::GetProvidedServices(provided);
            }

            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
            {
                AnimAudioComponent::GetRequiredServices(required);
            }

            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
            {
                AnimAudioComponent::GetIncompatibleServices(incompatible);
            }

            AZStd::vector<EditorAudioTriggerEvent> m_editorTriggerEvents;
        };
    } // namespace Integration
} // namespace EMotionFX
