/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/PrefabFocusInterface.h>
#include <AzToolsFramework/UI/Prefab/PrefabViewportFocusPathHandler.h>

namespace AzToolsFramework::Prefab
{
    PrefabViewportFocusPathHandler::~PrefabViewportFocusPathHandler()
    {
        // Disconnect from Prefab Focus Notifications
        PrefabFocusNotificationBus::Handler::BusDisconnect();
    }

    void PrefabViewportFocusPathHandler::Initialize(AzQtComponents::BreadCrumbs* breadcrumbsWidget, QToolButton* backButton)
    {
        // Get reference to the PrefabFocusInterface handler
        m_prefabFocusInterface = AZ::Interface<PrefabFocusInterface>::Get();
        if (m_prefabFocusInterface == nullptr)
        {
            AZ_Assert(false, "Prefab - could not get PrefabFocusInterface on PrefabViewportFocusPathHandler construction.");
            return;
        }

        // Connect to Prefab Focus Notifications
        PrefabFocusNotificationBus::Handler::BusConnect();

        // Initialize Widgets
        m_breadcrumbsWidget = breadcrumbsWidget;
        m_backButton = backButton;

        m_backButton->setEnabled(false);

        connect(
            m_breadcrumbsWidget, &AzQtComponents::BreadCrumbs::linkClicked, this,
            [&](const QString&, int linkIndex)
            {
                m_prefabFocusInterface->FocusOnPathIndex(linkIndex);
            }
        );
    }

    void PrefabViewportFocusPathHandler::OnPrefabFocusChanged()
    {
        // Push new Path
        m_breadcrumbsWidget->pushPath(m_prefabFocusInterface->GetPrefabFocusPath().c_str());
    }

} // namespace AzToolsFramework::Prefab
