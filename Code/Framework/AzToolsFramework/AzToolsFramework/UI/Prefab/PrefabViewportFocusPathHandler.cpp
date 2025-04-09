/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/Prefab/PrefabViewportFocusPathHandler.h>

#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Prefab/PrefabFocusPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>

#include <QTimer>

namespace AzToolsFramework::Prefab
{

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
        ViewportEditorModeNotificationsBus::Handler::BusConnect(m_editorEntityContextId);

        Refresh();
    }

    PrefabFocusPathWidget::~PrefabFocusPathWidget()
    {
        ViewportEditorModeNotificationsBus::Handler::BusDisconnect();
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

    void PrefabFocusPathWidget::OnEditorModeActivated(
        [[maybe_unused]] const ViewportEditorModesInterface& editorModeState, ViewportEditorMode mode)
    {
        // This function triggers when the Editor changes from regular editing to a Component Mode.
        if (mode == ViewportEditorMode::Component)
        {
            // Disable the widgets to prevent it from being used to change selection.
            setEnabled(false);
        }
    }

    void PrefabFocusPathWidget::OnEditorModeDeactivated(
        [[maybe_unused]] const ViewportEditorModesInterface& editorModeState, ViewportEditorMode mode)
    {
        // This function triggers when the Editor changes from a Component Mode to regular editing.
        if (mode == ViewportEditorMode::Component)
        {
            // Enable the widgets when leaving component mode.
            setEnabled(true);
        }
    }

    void PrefabFocusPathWidget::Refresh()
    {
        auto prefabPublicInterface = AZ::Interface<Prefab::PrefabPublicInterface>::Get();

        AZ::EntityId levelEntityId = prefabPublicInterface->GetLevelInstanceContainerEntityId();
        AZStd::size_t childCount = 0;

        EditorEntityInfoRequestBus::EventResult(childCount, levelEntityId, &EditorEntityInfoRequestBus::Events::GetChildCount);
        // Ignore the refresh if there isn't a level loaded yet
        if (childCount == 0)
        {
            return;
        }
        
        // Push new Path
        pushPath(m_prefabFocusPublicInterface->GetPrefabFocusPath(m_editorEntityContextId).c_str());

        // Add icons to the widget
        setDefaultIcon(QString(":/Entity/prefab_edit.svg"));

        // Set root icon
        setIconAt(0, QString(":/Level/level.svg"));
    }

} // namespace AzToolsFramework::Prefab
