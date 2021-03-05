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
#include "AzToolsFramework_precompiled.h"
#include "EditorLockComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzToolsFramework
{
    namespace Components
    {
        void EditorLockComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorLockComponent, EditorComponentBase>()
                    ->Field("Locked", &EditorLockComponent::m_locked)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorLockComponent>("Lock", "Edit-time entity lock state")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                            ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::SliceFlags::NotPushable)
                            ->Attribute(AZ::Edit::Attributes::HideIcon, true);
                }
            }
        }

        void EditorLockComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("EditorLockService", 0x6b15eacf));
        }

        void EditorLockComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("EditorLockService", 0x6b15eacf));
        }

        EditorLockComponent::~EditorLockComponent()
        {
            EditorLockComponentRequestBus::Handler::BusDisconnect();
        }

        void EditorLockComponent::Init()
        {
            EditorLockComponentRequestBus::Handler::BusConnect(GetEntityId());
        }

        void EditorLockComponent::SetLocked(bool locked)
        {
            if (m_locked != locked)
            {
                m_locked = locked;

                AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                    &AzToolsFramework::ToolsApplicationRequestBus::Events::AddDirtyEntity, m_entity->GetId());

                // notify individual entities connected to this bus
                EditorEntityLockComponentNotificationBus::Event(
                    m_entity->GetId(), &EditorEntityLockComponentNotifications::OnEntityLockFlagChanged, locked);
            }
        }

        bool EditorLockComponent::GetLocked()
        {
            return m_locked;
        }

    } // namespace Components
} // namespace AzToolsFramework
