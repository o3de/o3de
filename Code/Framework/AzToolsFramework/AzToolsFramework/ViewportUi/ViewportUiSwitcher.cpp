/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ViewportUi/ButtonGroup.h>
#include <AzToolsFramework/ViewportUi/ViewportUiSwitcher.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

#include <QBitmap>
#include <QTimer>

#pragma optimize("", off)
#pragma inline_depth(0)


namespace AzToolsFramework::ViewportUi::Internal
{
    ViewportUiSwitcher::ViewportUiSwitcher(AZStd::shared_ptr<ButtonGroup> buttonGroup)
        : m_buttonGroup(buttonGroup)
    {
        AzFramework::ViewportBorderNotificationBus::Handler::BusConnect();
        ViewportEditorModeNotificationsBus::Handler::BusConnect(GetEntityContextId()); 

        setOrientation(Qt::Orientation::Horizontal);
        setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
        setStyleSheet("border: none;");
        m_styleSheet = styleSheet();

        m_activeButton = new QToolButton();
        m_activeButton->setCheckable(false);
        m_activeButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        addWidget(m_activeButton);

        const AZStd::vector<Button*> buttons = buttonGroup->GetButtons();

        for (auto button : buttons)
        {
            // Add all the buttons as actions
            if (button->m_buttonId)
            {
                AddButton(button);
            }
        }
    }

    ViewportUiSwitcher::~ViewportUiSwitcher()
    {
        ViewportEditorModeNotificationsBus::Handler::BusDisconnect();
        AzFramework::ViewportBorderNotificationBus::Handler::BusDisconnect();
        delete m_activeButton;
    }

    static QPixmap RemoveIconColor(const char* buttonIconPath)
    {
        QPainter painter;
        QPixmap buttonPixmap = QPixmap(QString(buttonIconPath));

        painter.begin(&buttonPixmap);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(buttonPixmap.rect(), Qt::white);
        painter.end();

        return buttonPixmap;
    }

    void ViewportUiSwitcher::AddButton(Button* button)
    {
        QAction* action = new QAction();
        action->setCheckable(false);
        action->setIcon(RemoveIconColor(button->m_icon.c_str()));

        if (!action)
        {
            return;
        }

        // add the action
        addAction(action);

        // resize to fit new action with minimum extra space
        resize(minimumSizeHint());

        const AZStd::function<void()>& callback = [this, button]()
        {
            m_buttonGroup->PressButton(button->m_buttonId);
        };
        const AZStd::function<void(QAction*)>& updateCallback = [button](QAction* action)
        {
            action->setChecked(button->m_state == Button::State::Selected);
        };

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

        m_buttonActionMap.insert({ button->m_buttonId, action });
    }

    void ViewportUiSwitcher::RemoveButton(ButtonId buttonId)
    {
        if (auto actionEntry = m_buttonActionMap.find(buttonId); actionEntry != m_buttonActionMap.end())
        {
            QAction* action = actionEntry->second;

            // remove the action from the toolbar
            removeAction(action);

            // deregister from the widget manager
            m_widgetCallbacks.RemoveWidget(action);

            // resize to fit new area with minimum extra space
            resize(minimumSizeHint());

            m_buttonActionMap.erase(buttonId);
            m_buttonGroup->RemoveButton(buttonId);

            // reset current active mode if its the button being removed
            if (buttonId == m_activeButtonId)
            {
                if (auto nextEntry = m_buttonActionMap.find(ButtonId(buttonId + 1)); nextEntry != m_buttonActionMap.end())
                {
                    SetActiveButton(nextEntry->first);
                }
            }
        }
    }

    void ViewportUiSwitcher::Update()
    {
        QToolBar::repaint();
        m_widgetCallbacks.Update();
    }

    void ViewportUiSwitcher::SetActiveButton(ButtonId buttonId)
    {
        // Check if it is the first active mode to be set
        bool initialActiveMode = (m_activeButtonId == ButtonId(0));

        // Change the tool button's name and icon to that button
        const AZStd::vector<Button*> buttons = m_buttonGroup->GetButtons();
        auto found = [buttonId](Button* button)
        {
            return (button->m_buttonId == buttonId);
        };

        if (auto buttonIt = AZStd::find_if(buttons.begin(), buttons.end(), found); buttonIt != buttons.end())
        {
            QString buttonName = ((*buttonIt)->m_name).c_str();
            QIcon buttonIcon = RemoveIconColor(((*buttonIt)->m_icon).c_str());

            m_activeButton->setIcon(buttonIcon);
            m_activeButton->setText(buttonName);
        }

        // Look up button ID in map then remove it from its current position
        auto itr = m_buttonActionMap.find(buttonId);
        QAction* action = itr->second;
        removeAction(action);

        if (!initialActiveMode)
        {
            // Add the last action removed
            if (m_activeButtonId != buttonId)
            {
                itr = m_buttonActionMap.find(m_activeButtonId);
                action = itr->second;
                addAction(action);
            }
        }

        m_activeButtonId = buttonId;
    }

    void ViewportUiSwitcher::SetButtonTooltip(const ButtonId buttonId, const AZStd::string& tooltip)
    {
        // get the action corresponding to the buttonId
        if (auto actionEntry = m_buttonActionMap.find(buttonId); actionEntry != m_buttonActionMap.end())
        {
            // update the tooltip
            auto action = actionEntry->second;
            action->setToolTip(QString((tooltip).c_str()));
        }
    }

    void ViewportUiSwitcher::ImGuiActive(bool active)
    {
        if (active)
        {
            m_imGuiActive = true;

            bool borderVisible;
            ViewportUi::ViewportUiRequestBus::EventResult(
                borderVisible, ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::ViewportBorderVisible);
            if (borderVisible)
            {
                setStyleSheet(QString("QToolBar#viewportUiSwitcher { margin-top: 50px; border: none; }"));
                
            }
        }
        else
        {
            m_imGuiActive = false;
            //setStyleSheet(m_styleSheet + tr("margin-top: 0px;"));
            AZ_Printf("debugging", "%s", m_styleSheet.toStdString().c_str());
        }
    };

    void ViewportUiSwitcher::OnEditorModeDeactivated(
        [[maybe_unused]] const ViewportEditorModesInterface& editorModeState, ViewportEditorMode mode)
    {
        if (mode == ViewportEditorMode::Component)
        {
            if (m_imGuiActive)
            {
                // wait one frame for the undo stack to actually be updated
                m_test = false;
                QTimer::singleShot(
                    0,
                    nullptr,
                    [this]()
                    {
                        if (m_test == false)
                        {
                        setStyleSheet(m_styleSheet + tr("margin-top: 0px;"));
                        }
                    });
            }
        }
    }

    void ViewportUiSwitcher::OnEditorModeActivated(
        [[maybe_unused]] const ViewportEditorModesInterface& editorModeState, ViewportEditorMode mode)
    {
        // if the user enters component mode not using the switcher, the switcher should still switch buttons
        if (mode == ViewportEditorMode::Component)
        {
            if (m_imGuiActive)
            {
                ViewportUiSwitcher::setStyleSheet(styleSheet() + tr("margin-top: 25px;"));
                m_test = true;
            }

        }
    }
} // namespace AzToolsFramework::ViewportUi::Internal
