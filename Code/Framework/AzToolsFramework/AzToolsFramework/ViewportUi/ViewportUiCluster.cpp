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

#include "AzToolsFramework_precompiled.h"

#include <AzToolsFramework/ViewportUi/ButtonGroup.h>
#include <AzToolsFramework/ViewportUi/ViewportUiCluster.h>

namespace AzToolsFramework::ViewportUi::Internal
{
    ViewportUiCluster::ViewportUiCluster(AZStd::shared_ptr<ButtonGroup> buttonGroup)
        : QToolBar(nullptr)
        , m_buttonGroup(buttonGroup)
    {
        setOrientation(Qt::Orientation::Vertical);
        setStyleSheet("background: black;");

        const AZStd::vector<Button*> buttons = buttonGroup->GetButtons();
        for (auto button : buttons)
        {
            RegisterButton(button);
        }
    }

    void ViewportUiCluster::RegisterButton(Button* button)
    {
        QAction* action = new QAction();
        action->setCheckable(true);
        action->setIcon(QIcon(QString(button->m_icon.c_str())));

        AddClusterAction(
            action,
            [this, button]() {
                m_buttonGroup->PressButton(button->m_buttonId);
            },
            [button](QAction* action) {
                action->setChecked(button->m_state == Button::State::Selected);
            });

        m_buttonActionMap.insert({ button->m_buttonId, action });
    }

    void ViewportUiCluster::RemoveButton(ButtonId buttonId)
    {
        if (auto actionEntry = m_buttonActionMap.find(buttonId);
            actionEntry != m_buttonActionMap.end())
        {
            auto action = actionEntry->second;
            RemoveClusterAction(action);
            m_buttonActionMap.erase(buttonId);
        }
    }

    void ViewportUiCluster::AddClusterAction(
        QAction* action, const AZStd::function<void()>& callback,
        const AZStd::function<void(QAction*)>& updateCallback)
    {
        if (!action)
        {
            return;
        }

        // set hover to true by default
        action->setProperty("IconHasHoverEffect", true);

        // add the action
        addAction(action);

        // resize to fit new action with minimum extra space
        resize(minimumSizeHint());

        // connect the callback if provided
        if (callback)
        {
            QObject::connect(action, &QAction::triggered, action, callback);
        }

        // register the action
        m_widgetCallbacks.AddWidget(action, [updateCallback](QPointer<QObject> object)
        {
            updateCallback(static_cast<QAction*>(object.data()));
        });
    }

    void ViewportUiCluster::RemoveClusterAction(QAction* action)
    {
        // remove the action from the toolbar
        removeAction(action);

        // deregister from the widget manager
        m_widgetCallbacks.RemoveWidget(action);

        // resize to fit new area with minimum extra space
        resize(minimumSizeHint());
    }

    void ViewportUiCluster::Update()
    {
        m_widgetCallbacks.Update();
    }

    ViewportUiWidgetCallbacks ViewportUiCluster::GetWidgetCallbacks()
    {
        return m_widgetCallbacks;
    }
} // namespace AzToolsFramework::ViewportUi::Internal
