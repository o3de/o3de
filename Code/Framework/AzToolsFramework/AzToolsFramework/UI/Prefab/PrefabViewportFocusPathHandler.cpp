/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/Prefab/PrefabViewportFocusPathHandler.h>

#include <AzToolsFramework/Prefab/PrefabFocusPublicInterface.h>

namespace AzToolsFramework::Prefab
{
    PrefabViewportFocusPathHandler::PrefabViewportFocusPathHandler()
    {
        // Get default EntityContextId
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            m_editorEntityContextId, &AzToolsFramework::EditorEntityContextRequestBus::Events::GetEditorEntityContextId);

        // Connect to Prefab Focus Notifications
        PrefabFocusNotificationBus::Handler::BusConnect(m_editorEntityContextId);
    }

    PrefabViewportFocusPathHandler::~PrefabViewportFocusPathHandler()
    {
        // Disconnect from Prefab Focus Notifications
        PrefabFocusNotificationBus::Handler::BusDisconnect();
    }

    void PrefabViewportFocusPathHandler::Initialize(AzQtComponents::BreadCrumbs* breadcrumbsWidget, QToolButton* backButton)
    {
        // Get reference to the PrefabFocusInterface handler
        m_prefabFocusPublicInterface = AZ::Interface<PrefabFocusPublicInterface>::Get();
        if (m_prefabFocusPublicInterface == nullptr)
        {
            AZ_Assert(false, "Prefab - could not get PrefabFocusInterface on PrefabViewportFocusPathHandler construction.");
            return;
        }

        // Initialize Widgets
        m_breadcrumbsWidget = breadcrumbsWidget;
        m_backButton = backButton;

        // If a part of the path is clicked, focus on that instance
        connect(m_breadcrumbsWidget, &AzQtComponents::BreadCrumbs::linkClicked, this,
            [&](const QString&, int linkIndex)
            {
                m_prefabFocusPublicInterface->FocusOnPathIndex(m_editorEntityContextId, linkIndex);
            }
        );

        // The back button will allow user to go one level up
        connect(m_backButton, &QToolButton::clicked, this,
            [&]()
            {
                if (int length = m_prefabFocusPublicInterface->GetPrefabFocusPathLength(m_editorEntityContextId); length > 1)
                {
                    m_prefabFocusPublicInterface->FocusOnPathIndex(m_editorEntityContextId, length - 2);
                }
            }
        );
    }

    void PrefabViewportFocusPathHandler::OnPrefabFocusChanged()
    {
        // Push new Path
        m_breadcrumbsWidget->pushPath(m_prefabFocusPublicInterface->GetPrefabFocusPath(m_editorEntityContextId).c_str());
    }

} // namespace AzToolsFramework::Prefab
