/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorComponentBase.h"
#include "TransformComponent.h"
#include <AzCore/Math/Vector2.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

namespace AzToolsFramework
{
    namespace Components
    {
        void EditorComponentBase::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<EditorComponentBase, AZ::Component>()
                    ->Version(1)
                ;
            }
        }

        EditorComponentBase::EditorComponentBase()
        {
            m_transform = nullptr;
        }

        void EditorComponentBase::Init()
        {
        }

        void EditorComponentBase::Activate()
        {
            m_transform = GetEntity()->FindComponent<TransformComponent>();
        }

        void EditorComponentBase::Deactivate()
        {
            m_transform = nullptr;
        }

        void EditorComponentBase::SetDirty()
        {
            if (GetEntity())
            {
                EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, AddDirtyEntity, GetEntity()->GetId());
            }
            else
            {
                AZ_Warning("Editor", false, "EditorComponentBase::SetDirty() failed. Couldn't add dirty entity because the pointer to the entity is NULL. Make sure the entity is Init()'d properly.");
            }
        }

        AZ::TransformInterface* EditorComponentBase::GetTransform() const
        {
            AZ_Assert(m_transform, "Attempt to GetTransformComponent when the entity is inactive or does not have one.");
            return m_transform;
        }

        AZ::Transform EditorComponentBase::GetWorldTM() const
        {
            if (m_transform)
            {
                return m_transform->GetWorldTM();
            }
            else
            {
                return AZ::Transform::Identity();
            }
        }

        AZ::Transform EditorComponentBase::GetLocalTM() const
        {
            if (m_transform)
            {
                return m_transform->GetLocalTM();
            }
            else
            {
                return AZ::Transform::Identity();
            }
        }

        bool EditorComponentBase::IsSelected() const
        {
            return AzToolsFramework::IsSelected(GetEntityId());
        }
    }
} // namespace AzToolsFramework
