/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/DocumentPropertyEditor/ComponentAdapter.h>
#include <QtCore/QTimer>

namespace AZ::DocumentPropertyEditor
{
    ComponentAdapter::ComponentAdapter()
    {
    }

    ComponentAdapter::ComponentAdapter(AZ::Component* componentInstace)
    {
        SetComponent(componentInstace);
    }

    ComponentAdapter::~ComponentAdapter() = default;

    void ComponentAdapter::OnEntityComponentPropertyChanged(AZ::ComponentId componentId)
    {
        if (m_componentInstance->GetId() == componentId)
        {
            m_refreshQueue++;
            QTimer::singleShot(
                0,
                [this]()
                {
                    DoRefresh();
                });
        }
    }

    void ComponentAdapter::InvalidatePropertyDisplay([[maybe_unused]] AzToolsFramework::PropertyModificationRefreshLevel level)
    {
        m_refreshQueue++;
        QTimer::singleShot(
            0,
            [this]()
            {
                DoRefresh();
            });
    }

    void ComponentAdapter::SetComponent(AZ::Component* componentInstance)
    {
        m_componentInstance = componentInstance;
        AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler::BusConnect(m_componentInstance->GetEntityId());
        AzToolsFramework::ToolsApplicationEvents::Bus::Handler::BusConnect();
        AZ::Uuid instanceTypeId = azrtti_typeid(m_componentInstance);
        SetValue(m_componentInstance, instanceTypeId);
    }

    void ComponentAdapter::DoRefresh()
    {
        if (m_refreshQueue == 0)
        {
            return;
        }
        m_refreshQueue = 0;
        NotifyResetDocument();
    }

} // namespace AZ::DocumentPropertyEditor
