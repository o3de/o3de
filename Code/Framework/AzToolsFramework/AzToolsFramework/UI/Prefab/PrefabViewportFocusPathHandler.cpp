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

#include <AzToolsFramework/Prefab/PrefabFocusInterface.h>
#include <QHBoxLayout>

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

        setPushPathOnLinkActivation(false);
        setDefaultIcon(QStringLiteral(":/Entity/prefab_edit.svg"));
        m_focusUpButton = new QToolButton(this);
        m_focusUpButton->setAutoRaise(true);
        m_focusUpButton->setIcon(QIcon(QStringLiteral(":/Breadcrumb/img/UI20/Breadcrumb/arrow_left-default.svg")));
        m_focusUpButton->setToolTip(tr("Focus up one level"));
        qobject_cast<QBoxLayout*>(layout())->insertWidget(0, m_focusUpButton);
        connect(m_focusUpButton, &QToolButton::clicked, this, [this]
        {
            m_prefabFocusPublicInterface->FocusOnParentOfFocusedPrefab(m_editorEntityContextId);
        });
        connect(this, &AzQtComponents::BreadCrumbs::linkClicked, this, [this](const QString&, int index)
        {
            m_prefabFocusPublicInterface->FocusOnPathIndex(m_editorEntityContextId, index);
        });

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

    void PrefabFocusPathWidget::showEvent(QShowEvent* event)
    {
        BreadCrumbs::showEvent(event);
        Refresh();
    }

    void PrefabFocusPathWidget::Refresh()
    {
        auto* focus = AZ::Interface<PrefabFocusInterface>::Get();
        if (!focus || !m_prefabFocusPublicInterface)
        {
            return;
        }
        AZ::IO::Path path;
        for (auto* owner = parentWidget(); owner; owner = owner->parentWidget())
        {
            if (const auto viewportId = owner->property("editorViewportId"); viewportId.isValid())
            {
                path = focus->GetViewportFocusPath(viewportId.toInt());
                break;
            }
        }
        if (path.empty())
        {
            path = m_prefabFocusPublicInterface->GetPrefabFocusPath(m_editorEntityContextId);
        }
        m_focusUpButton->setEnabled(path.HasParentPath());
        const QString displayPath = QString::fromUtf8(path.c_str());
        if (displayPath != currentPath())
        {
            setCurrentPath(displayPath);
            setIconAt(0, QStringLiteral(":/Level/level.svg"));
        }
    }
} // namespace AzToolsFramework::Prefab
