/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/EditorEntityUi/EditorEntityUiSystemComponent.h>

#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzToolsFramework
{
    namespace Components
    {

        EditorEntityUiSystemComponent::EditorEntityUiSystemComponent()
        {
            AZ::Interface<EditorEntityUiInterface>::Register(this);
        }

        EditorEntityUiSystemComponent::~EditorEntityUiSystemComponent()
        {
            AZ::Interface<EditorEntityUiInterface>::Unregister(this);
        }

        void EditorEntityUiSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<EditorEntityUiSystemComponent, AZ::Component>();
            }
        }

        void EditorEntityUiSystemComponent::Activate()
        {
        }

        void EditorEntityUiSystemComponent::Deactivate()
        {
        }

        void EditorEntityUiSystemComponent::Init()
        {
        }

        EditorEntityUiHandlerId EditorEntityUiSystemComponent::RegisterHandler(EditorEntityUiHandlerBase* handler)
        {
            if (handler == nullptr)
            {
                AZ_Warning("EditorEntityUiSystemComponent", false, "Trying to register a null handler.");
                return 0;
            }

            for (const auto& handlerIdPair : m_handlerIdHandlerMap)
            {
                if (handlerIdPair.second == handler)
                {
                    AZ_Warning("EditorEntityUiSystemComponent", false, "Trying to register a handler more than once.");
                    return 0;
                }
            }

            EditorEntityUiHandlerId handlerId = m_idCounter++;
            m_handlerIdHandlerMap.emplace(handlerId, handler);
            return handlerId;
        }

        void EditorEntityUiSystemComponent::UnregisterHandler(EditorEntityUiHandlerBase* handler)
        {
            if (handler == nullptr)
            {
                AZ_Warning("EditorEntityUiSystemComponent", false, "Trying to unregister a null handler.");
                return;
            }

            // Unregister from handlerId map
            EditorEntityUiHandlerId handlerId = 0;
            for (auto iter = m_handlerIdHandlerMap.begin(); iter != m_handlerIdHandlerMap.end(); ++iter)
            {
                if (iter->second == handler)
                {
                    handlerId = iter->first;
                }
            }

            if (handlerId > 0)
            {
                m_handlerIdHandlerMap.erase(handlerId);
            }
            else
            {
                AZ_Warning("EditorEntityUiSystemComponent", false, "Trying to unregister a handler that was not registered.");
                return;
            }

            // Unregister all entities referencing the handler
            AZStd::vector<AZ::EntityId> entitiesToUnregister;
            for(auto iter = m_entityIdHandlerMap.begin(); iter != m_entityIdHandlerMap.end(); ++iter)
            {
                if (iter->second == handler)
                {
                    entitiesToUnregister.push_back(iter->first);
                }
            }

            for (AZ::EntityId entityId : entitiesToUnregister)
            {
                m_entityIdHandlerMap.erase(entityId);
            }
        }

        bool EditorEntityUiSystemComponent::RegisterEntity(AZ::EntityId entityId, EditorEntityUiHandlerId handlerId)
        {
            if (m_entityIdHandlerMap.find(entityId) != m_entityIdHandlerMap.end())
            {
                AZ_Warning("EditorEntityUiSystemComponent", false, "Trying to register an entity that is already registered");
                return false;
            }

            if (!m_handlerIdHandlerMap.contains(handlerId))
            {
                AZ_Warning("EditorEntityUiSystemComponent", false, "Trying to register an entity to an invalid handlerId");
                return false;
            }

            m_entityIdHandlerMap.emplace(entityId, m_handlerIdHandlerMap[handlerId]);
            return true;
        }

        bool EditorEntityUiSystemComponent::UnregisterEntity(AZ::EntityId entityId)
        {
            if (!m_entityIdHandlerMap.contains(entityId))
            {
                AZ_Warning("EditorEntityUiSystemComponent", false, "Trying to unregister an entity that wasn't previously registered");
                return false;
            }

            m_entityIdHandlerMap.erase(entityId);
            return true;
        }

        EditorEntityUiHandlerBase* EditorEntityUiSystemComponent::GetHandler(AZ::EntityId entityId)
        {
            if (!m_entityIdHandlerMap.contains(entityId))
            {
                return nullptr;
            }

            return m_entityIdHandlerMap[entityId];
        }
    }
}
