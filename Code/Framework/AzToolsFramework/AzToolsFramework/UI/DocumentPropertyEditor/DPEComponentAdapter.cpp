/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/DocumentPropertyEditor/DPEComponentAdapter.h>
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
        AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler::BusDisconnect(m_componentInstance->GetEntityId());
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
        AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler::BusConnect(m_componentInstance->GetEntityId());
        AzToolsFramework::ToolsApplicationEvents::Bus::Handler::BusConnect();
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::BusConnect();
        AZ::Uuid instanceTypeId = azrtti_typeid(m_componentInstance);
        SetValue(m_componentInstance, instanceTypeId);
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

} // namespace AZ::DocumentPropertyEditor
