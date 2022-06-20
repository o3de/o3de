/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ViewportUi/ButtonGroup.h>
#include <AzToolsFramework/ViewportUi/ViewportUiCluster.h>

namespace AzToolsFramework::ViewportUi::Internal
{
    ViewportUiCluster::ViewportUiCluster(AZStd::shared_ptr<ButtonGroup> buttonGroup)
        : QToolBar(nullptr)
        , m_buttonGroup(AZStd::move(buttonGroup))
    {
        setOrientation(Qt::Orientation::Vertical);
        setStyleSheet("background: black;");

        for (auto button : m_buttonGroup->GetButtons())
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
            [this, button]()
            {
                m_buttonGroup->PressButton(button->m_buttonId);
            },
            [button](QAction* action)
            {
                action->setChecked(button->m_state == Button::State::Selected);
            });

        m_buttonActionMap.insert({ button->m_buttonId, action });
    }

    void ViewportUiCluster::RemoveButton(ButtonId buttonId)
    {
        if (auto actionEntry = m_buttonActionMap.find(buttonId); actionEntry != m_buttonActionMap.end())
        {
            auto action = actionEntry->second;
            RemoveClusterAction(action);
            m_buttonActionMap.erase(buttonId);
        }
    }

    void ViewportUiCluster::AddClusterAction(
        QAction* action, const AZStd::function<void()>& callback, const AZStd::function<void(QAction*)>& updateCallback)
    {
        if (!action)
        {
            return;
        }

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
        m_widgetCallbacks.AddWidget(
            action,
            [updateCallback](QPointer<QObject> object)
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

    void ViewportUiCluster::SetButtonLocked(const ButtonId buttonId, const bool isLocked)
    {
        const auto& buttons = m_buttonGroup->GetButtons();

        // unlocked previously locked button
        if (m_lockedButtonId.has_value() && isLocked)
        {
            // find the button to extract the old icon (without overlay)
            auto findLocked = [this](const Button* button)
            {
                return (button->m_buttonId == m_lockedButtonId);
            };
            if (auto lockedButtonIt = AZStd::find_if(buttons.begin(), buttons.end(), findLocked); lockedButtonIt != buttons.end())
            {
                // get the action corresponding to the lockedButtonId
                if (auto actionEntry = m_buttonActionMap.find(m_lockedButtonId.value()); actionEntry != m_buttonActionMap.end())
                {
                    // remove the overlay
                    auto action = actionEntry->second;
                    action->setIcon(QIcon(QString((*lockedButtonIt)->m_icon.c_str())));
                }
            }
        }

        auto found = [buttonId](Button* button)
        {
            return (button->m_buttonId == buttonId);
        };

        if (auto buttonIt = AZStd::find_if(buttons.begin(), buttons.end(), found); buttonIt != buttons.end())
        {
            QIcon newIcon;

            if (isLocked)
            {
                // overlay the locked icon on top of the button's icon
                QPixmap comboPixmap(24, 24);
                comboPixmap.fill(Qt::transparent);
                QPixmap firstImage(QString((*buttonIt)->m_icon.c_str()));
                QPixmap secondImage(QString(":/stylesheet/img/UI20/toolbar/Locked_Status.svg"));

                QPainter painter(&comboPixmap);
                painter.drawPixmap(0, 0, firstImage);
                painter.drawPixmap(0, 0, secondImage);
                newIcon.addPixmap(comboPixmap);
                m_lockedButtonId = buttonId;
            }
            else
            {
                // remove the overlay
                newIcon = QIcon(QString((*buttonIt)->m_icon.c_str()));
                m_lockedButtonId = AZStd::nullopt;
            }

            if (auto actionEntry = m_buttonActionMap.find(buttonId); actionEntry != m_buttonActionMap.end())
            {
                auto action = actionEntry->second;
                action->setIcon(newIcon);
            }
        }
    }

    void ViewportUiCluster::SetButtonTooltip(const ButtonId buttonId, const AZStd::string& tooltip)
    {
        // get the action corresponding to the buttonId
        if (auto actionEntry = m_buttonActionMap.find(buttonId); actionEntry != m_buttonActionMap.end())
        {
            // update the tooltip
            auto action = actionEntry->second;
            action->setToolTip(QString((tooltip).c_str()));
        }
    }

    ViewportUiWidgetCallbacks ViewportUiCluster::GetWidgetCallbacks()
    {
        return m_widgetCallbacks;
    }
} // namespace AzToolsFramework::ViewportUi::Internal
