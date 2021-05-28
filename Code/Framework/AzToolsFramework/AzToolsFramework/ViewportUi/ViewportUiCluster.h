/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzToolsFramework/ViewportUi/Button.h>
#include <AzToolsFramework/ViewportUi/ViewportUiWidgetCallbacks.h>
#include <QToolBar>

namespace AzToolsFramework::ViewportUi::Internal
{
    class ButtonGroup;

    //! Helper class to make clusters (toolbars) for display in Viewport UI.
    class ViewportUiCluster
        : public QToolBar
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
        //! Returns the widget manager.
        ViewportUiWidgetCallbacks GetWidgetCallbacks();

    private:
        //! Adds an action to the Viewport UI Cluster.
        void AddClusterAction(
            QAction* action, const AZStd::function<void()>& callback = {},
            const AZStd::function<void(QAction*)>& updateCallback = {});
        //! Removes an action from the Viewport UI Cluster.
        void RemoveClusterAction(QAction* action);

        AZStd::shared_ptr<ButtonGroup> m_buttonGroup; //!< Data structure which the cluster will be displaying to the Viewport UI.
        AZStd::unordered_map<ButtonId, QPointer<QAction>> m_buttonActionMap; //!< Map for buttons to their corresponding actions.
        ViewportUiWidgetCallbacks m_widgetCallbacks; //!< Registers actions and manages updates.
    };
} // namespace AzToolsFramework::ViewportUi::Internal
