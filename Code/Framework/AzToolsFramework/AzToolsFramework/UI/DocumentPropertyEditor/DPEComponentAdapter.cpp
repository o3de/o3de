/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/Backends/JSON/JsonSerializationUtils.h>
#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
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
        if (m_componentInstance->GetId() == componentId)
        {
            m_queuedRefreshLevel = AzToolsFramework::PropertyModificationRefreshLevel::Refresh_Values;
            QTimer::singleShot(
                0,
                [this]()
                {
                    DoRefresh();
                });
        }
    }

    void ComponentAdapter::InvalidatePropertyDisplay(AzToolsFramework::PropertyModificationRefreshLevel level)
    {
        if (level > m_queuedRefreshLevel)
        {
            m_queuedRefreshLevel = level;
            QTimer::singleShot(
                0,
                [this]()
                {
                    DoRefresh();
                });
        }
    }

    void ComponentAdapter::RequestRefresh(AzToolsFramework::PropertyModificationRefreshLevel level)
    {
        if (level > m_queuedRefreshLevel)
        {
            m_queuedRefreshLevel = level;
            QTimer::singleShot(
                0,
                [this]()
                {
                    DoRefresh();
                });
        }
    }

    void ComponentAdapter::SetComponent(AZ::Component* componentInstance)
    {
        m_componentInstance = componentInstance;
        m_entityId = m_componentInstance->GetEntityId();

        AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler::BusConnect(m_entityId);
        AzToolsFramework::ToolsApplicationEvents::Bus::Handler::BusConnect();
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::BusConnect();

        SetValue(m_componentInstance, azrtti_typeid(m_componentInstance));
    }

    void ComponentAdapter::DoRefresh()
    {
        if (m_queuedRefreshLevel == AzToolsFramework::PropertyModificationRefreshLevel::Refresh_None)
        {
            return;
        }
        m_queuedRefreshLevel = AzToolsFramework::PropertyModificationRefreshLevel::Refresh_None;
        NotifyResetDocument();
    }

    Dom::Value ComponentAdapter::HandleMessage(const AdapterMessage& message)
    {
        auto handlePropertyEditorChanged = [&]([[maybe_unused]] const Dom::Value& valueFromEditor, Nodes::ValueChangeType changeType)
        {
            switch (changeType)
            {
            case Nodes::ValueChangeType::InProgressEdit:
                if (m_componentInstance)
                {
                    const AZ::EntityId& entityId = m_componentInstance->GetEntityId();
                    if (entityId.IsValid())
                    {
                        if (m_currentUndoNode)
                        {
                            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                                m_currentUndoNode,
                                &AzToolsFramework::ToolsApplicationRequests::ResumeUndoBatch,
                                m_currentUndoNode,
                                "Modify Entity Property");
                        }
                        else
                        {
                            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                                m_currentUndoNode, &AzToolsFramework::ToolsApplicationRequests::BeginUndoBatch, "Modify Entity Property");
                        }

                        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
                            &AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, entityId);
                    }
                }
                break;
            case Nodes::ValueChangeType::FinishedEdit:
                if (m_currentUndoNode)
                {
                    AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::EndUndoBatch);
                    m_currentUndoNode = nullptr;
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
} // namespace AZ::DocumentPropertyEditor
