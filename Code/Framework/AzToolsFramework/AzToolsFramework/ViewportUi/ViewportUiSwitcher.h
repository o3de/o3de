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

#include <AzToolsFramework/ViewportUi/Button.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzToolsFramework/ViewportUi/ViewportUiWidgetCallbacks.h>
#include <functional>
#include <QPointer>
#include <QToolBar>
#include <QToolButton>

namespace AzToolsFramework::ViewportUi::Internal
{
    class ButtonGroup;

    //! Helper class to make switchers (toolbars) for display in Viewport UI.
    class ViewportUiSwitcher : public QToolBar
    {
        Q_OBJECT

    public:
        ViewportUiSwitcher(AZStd::shared_ptr<ButtonGroup> buttonGroup);
        ~ViewportUiSwitcher();
        //! Adds a new button to the switcher.
        void AddButton(Button* button);
        //! Removes a button from the switcher.
        void RemoveButton(ButtonId buttonId);
        void Update();
        //! Changes the m_activeButton.
        void SetActiveButton(ButtonId buttonId);

    private:
        QToolButton* m_activeButton; //!< The first button in the toolbar. Only button with a label/text.
        ButtonId m_activeButtonId = ButtonId(0); //!< ButtonId corresponding to the active button in the buttonActionMap.
        AZStd::shared_ptr<ButtonGroup> m_buttonGroup; //!< Data structure which the cluster will be displaying to the Viewport UI.
        AZStd::unordered_map<ButtonId, QPointer<QAction>> m_buttonActionMap; //!< Map for buttons to their corresponding actions.
        ViewportUiWidgetCallbacks m_widgetCallbacks; //!< Registers actions and manages updates.
    };
} // namespace AzToolsFramework::ViewportUi::Internal
