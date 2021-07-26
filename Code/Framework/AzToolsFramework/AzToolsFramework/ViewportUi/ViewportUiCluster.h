/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzToolsFramework/ViewportUi/Button.h>
#include <AzToolsFramework/ViewportUi/ViewportUiWidgetCallbacks.h>

#include <QPainter>
#include <QToolBar>

namespace AzToolsFramework::ViewportUi::Internal
{
    class ButtonGroup;

    //! Helper class to make clusters (toolbars) for display in Viewport UI.
    class ViewportUiCluster : public QToolBar
    {
        Q_OBJECT

    public:
        ViewportUiCluster(AZStd::shared_ptr<ButtonGroup> buttonGroup);
        ~ViewportUiCluster() = default;

        //! Adds a new button to the cluster.
        void RegisterButton(Button* button);
        //! Removes a button from the cluster.
        void RemoveButton(ButtonId buttonId);
        //! Updates all registered actions.
        void Update();
        //! Adds a locked overlay to the button's icon.
        void SetButtonLocked(ButtonId buttonId, bool isLocked);
        //! Updates the button's tooltip to the passed string.
        void SetButtonTooltip(ButtonId buttonId, const AZStd::string& tooltip);
        //! Returns the widget manager.
        ViewportUiWidgetCallbacks GetWidgetCallbacks();

    private:
        //! Adds an action to the Viewport UI Cluster.
        void AddClusterAction(
            QAction* action, const AZStd::function<void()>& callback = {}, const AZStd::function<void(QAction*)>& updateCallback = {});
        //! Removes an action from the Viewport UI Cluster.
        void RemoveClusterAction(QAction* action);

        AZStd::shared_ptr<ButtonGroup> m_buttonGroup; //!< Data structure which the cluster will be displaying to the Viewport UI.
        AZStd::unordered_map<ButtonId, QPointer<QAction>> m_buttonActionMap; //!< Map for buttons to their corresponding actions.
        ViewportUiWidgetCallbacks m_widgetCallbacks; //!< Registers actions and manages updates.
        AZStd::optional<ButtonId> m_lockedButtonId = AZStd::nullopt; //!< Used to track the last button locked.
    };
} // namespace AzToolsFramework::ViewportUi::Internal
