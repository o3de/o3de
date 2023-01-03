/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/Prefab/PrefabViewportFocusPathHandler.h>

#include <AzToolsFramework/Prefab/PrefabFocusPublicInterface.h>

#include <QTimer>

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

        // Add icons to the widget
        m_breadcrumbsWidget->setDefaultIcon(QString(":/Entity/prefab_edit.svg"));

        // If a part of the path is clicked, focus on that instance
        connect(m_breadcrumbsWidget, &AzQtComponents::BreadCrumbs::linkClicked, this,
            [&](const QString&, int linkIndex)
            {
                m_prefabFocusPublicInterface->FocusOnPathIndex(m_editorEntityContextId, linkIndex);

                // Manually refresh path
                QTimer::singleShot(0, [&]() { Refresh(); });
            }
        );

        // The back button will allow user to go one level up
        connect(m_backButton, &QToolButton::clicked, this,
            [&]()
            {
                m_prefabFocusPublicInterface->FocusOnParentOfFocusedPrefab(m_editorEntityContextId);
            }
        );

        m_backButton->setToolTip("Up one level (-)");

        // Currently hide this button until we can correctly disable/enable it based on context.
        m_backButton->hide();

        Refresh();
    }

    void PrefabViewportFocusPathHandler::OnPrefabFocusChanged(
        [[maybe_unused]] AZ::EntityId previousContainerEntityId, [[maybe_unused]] AZ::EntityId newContainerEntityId)
    {
        Refresh();
    }

    void PrefabViewportFocusPathHandler::OnPrefabFocusRefreshed()
    {
        Refresh();
    }

    void PrefabViewportFocusPathHandler::Refresh()
    {
        if (int prefabFocusPathLength = m_prefabFocusPublicInterface->GetPrefabFocusPathLength(m_editorEntityContextId);
            prefabFocusPathLength > 0)
        {
            // Push new Path
            m_breadcrumbsWidget->pushPath(m_prefabFocusPublicInterface->GetPrefabFocusPath(m_editorEntityContextId).c_str());

            // Set root icon
            m_breadcrumbsWidget->setIconAt(0, QString(":/Level/level.svg"));

            // If root instance is focused, disable the back button; else enable it.
            m_backButton->setEnabled(prefabFocusPathLength > 1);
        }
    }

    PrefabFocusPathWidget::PrefabFocusPathWidget()
    {
        // Initialize internal variables
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            m_editorEntityContextId, &AzToolsFramework::EditorEntityContextRequestBus::Events::GetEditorEntityContextId);

        m_prefabFocusPublicInterface = AZ::Interface<PrefabFocusPublicInterface>::Get();
        if (m_prefabFocusPublicInterface == nullptr)
        {
            AZ_Assert(false, "Prefab - could not get PrefabFocusInterface on PrefabViewportFocusPathHandler construction.");
            return;
        }

        // If a part of the path is clicked, focus on that instance
        connect(
            this,
            &AzQtComponents::BreadCrumbs::linkClicked,
            this,
            [&](const QString&, int linkIndex)
            {
                m_prefabFocusPublicInterface->FocusOnPathIndex(m_editorEntityContextId, linkIndex);

                // Manually refresh path
                QTimer::singleShot(
                    0,
                    [&]()
                    {
                        Refresh();
                    }
                );
            }
        );

        PrefabFocusNotificationBus::Handler::BusConnect(m_editorEntityContextId);
    }

    PrefabFocusPathWidget::~PrefabFocusPathWidget()
    {
        PrefabFocusNotificationBus::Handler::BusDisconnect();
    }

    void PrefabFocusPathWidget::OnPrefabFocusChanged(
        [[maybe_unused]] AZ::EntityId previousContainerEntityId, [[maybe_unused]] AZ::EntityId newContainerEntityId)
    {
        Refresh();
    }

    void PrefabFocusPathWidget::OnPrefabFocusRefreshed()
    {
        Refresh();
    }

    void PrefabFocusPathWidget::Refresh()
    {
        // Push new Path
        pushPath(m_prefabFocusPublicInterface->GetPrefabFocusPath(m_editorEntityContextId).c_str());

        // Add icons to the widget
        setDefaultIcon(QString(":/Entity/prefab_edit.svg"));

        // Set root icon
        setIconAt(0, QString(":/Level/level.svg"));
    }

} // namespace AzToolsFramework::Prefab
