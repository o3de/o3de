/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/Backends/JSON/JsonSerializationUtils.h>
#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/DPEComponentAdapter.h>
#include <AzToolsFramework/Prefab/DocumentPropertyEditor/PrefabAdapterInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabFocusPublicInterface.h>
#include <QtCore/QTimer>

namespace AZ::DocumentPropertyEditor
{
    ComponentAdapter::ComponentAdapter() = default;

    ComponentAdapter::ComponentAdapter(AZ::Component* componentInstace)
    {
        SetComponent(componentInstace);
    }

    ComponentAdapter::~ComponentAdapter()
    {
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::BusDisconnect();
        AzToolsFramework::ToolsApplicationEvents::Bus::Handler::BusDisconnect();
        if (m_entityId.IsValid())
        {
            AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler::BusDisconnect(m_entityId);
        }
    }

    void ComponentAdapter::OnEntityComponentPropertyChanged(AZ::ComponentId componentId)
    {
        if (m_componentId == componentId)
        {
            RequestRefresh(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_Values);
        }
    }

    void ComponentAdapter::InvalidatePropertyDisplay(AzToolsFramework::PropertyModificationRefreshLevel level)
    {
        RequestRefresh(level);
    }

    void ComponentAdapter::InvalidatePropertyDisplayForComponent(AZ::EntityComponentIdPair entityComponentIdPair, AzToolsFramework::PropertyModificationRefreshLevel level)
    {
        if ((entityComponentIdPair.GetEntityId() == m_entityId) && (entityComponentIdPair.GetComponentId() == m_componentId))
        {
            RequestRefresh(level);
        }
    }

    void ComponentAdapter::RequestRefresh(AzToolsFramework::PropertyModificationRefreshLevel level)
    {
        if (level > m_queuedRefreshLevel)
        {
            if (m_queuedRefreshLevel == AzToolsFramework::PropertyModificationRefreshLevel::Refresh_None)
            {
                QPointer<QObject> stillAlive(&m_stillAlive);
                QTimer::singleShot(
                    0,
                    [this, stillAlive]()
                    {
                        // make sure the component adapter still exists by the time this refresh resolves
                        if (stillAlive)
                        {
                            DoRefresh();
                        }
                    });
            }
            m_queuedRefreshLevel = level;
        }
    }

    void ComponentAdapter::SetComponent(AZ::Component* componentInstance)
    {
        m_entityId = componentInstance->GetEntityId();
        m_componentId = componentInstance->GetId();
        AZ::EntitySystemBus::Handler::BusConnect();

        AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler::BusConnect(m_entityId);
        AzToolsFramework::ToolsApplicationEvents::Bus::Handler::BusConnect();
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::BusConnect();

        AZ::Uuid instanceTypeId = azrtti_typeid(componentInstance);
        SetValue(componentInstance, instanceTypeId);
    }

    bool ComponentAdapter::IsComponentValid() const
    {
        if (m_entityId.IsValid())
        {
            const Entity* entity = AzToolsFramework::GetEntity(m_entityId);

            // Since DoRefresh() gets called on the next tick, the entity and its components could have been destroyed by then.
            if (entity == nullptr)
            {
                return false;
            }

            bool isEntityActive = entity->GetState() == AZ::Entity::State::Active;
            return isEntityActive && entity->FindComponent(m_componentId) != nullptr;
        }

        return false;
    }

    void ComponentAdapter::DoRefresh()
    {
        if (IsComponentValid())
        {
            m_queuedRefreshLevel = AzToolsFramework::PropertyModificationRefreshLevel::Refresh_None;
            NotifyResetDocument();
        }
    }

    Dom::Value ComponentAdapter::HandleMessage(const AdapterMessage& message)
    {
        auto handlePropertyEditorChanged = [&]([[maybe_unused]] const Dom::Value& valueFromEditor, Nodes::ValueChangeType changeType)
        {
            switch (changeType)
            {
            case Nodes::ValueChangeType::InProgressEdit:
                if (m_entityId.IsValid())
                {
                    if (m_currentUndoBatch)
                    {
                        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                            m_currentUndoBatch,
                            &AzToolsFramework::ToolsApplicationRequests::ResumeUndoBatch,
                            m_currentUndoBatch,
                            "Modify Entity Property");
                    }
                    else
                    {
                        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                            m_currentUndoBatch, &AzToolsFramework::ToolsApplicationRequests::BeginUndoBatch, "Modify Entity Property");
                    }

                    AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
                        &AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, m_entityId);
                }
                break;
            case Nodes::ValueChangeType::FinishedEdit:
                if (m_currentUndoBatch)
                {
                    AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::EndUndoBatch);
                    m_currentUndoBatch = nullptr;
                }
                break;
            }
        };

        Dom::Value returnValue = message.Match(Nodes::PropertyEditor::OnChanged, handlePropertyEditorChanged);

        ReflectionAdapter::HandleMessage(message);

        return returnValue;
    }

    void ComponentAdapter::CreateLabel(AdapterBuilder* adapterBuilder, AZStd::string_view labelText, AZStd::string_view serializedPath)
    {
        ReflectionAdapter::CreateLabel(adapterBuilder, labelText, serializedPath);
    }

    void ComponentAdapter::OnEntityDestruction(const AZ::EntityId& entityId)
    {
        if (entityId == m_entityId)
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::BusDisconnect();
            AzToolsFramework::ToolsApplicationEvents::Bus::Handler::BusDisconnect();
            AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler::BusDisconnect(m_entityId);
            
            m_entityId.SetInvalid();
            AZ::EntitySystemBus::Handler::BusDisconnect();
        }
    }

} // namespace AZ::DocumentPropertyEditor
