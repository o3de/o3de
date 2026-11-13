/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/StyleManager.h>
#include <AzToolsFramework/ViewportUi/ButtonGroup.h>
#include <AzToolsFramework/ViewportUi/ViewportUiSwitcher.h>

#include <QBitmap>
#include <QLayout>
#include <QPainter>

namespace AzToolsFramework::ViewportUi::Internal
{
    // ----------------------------------------------------------------------------------
    // Flattened GUI: the switcher's spacing
    //
    // The legacy GUI sets the sheet below in the constructor, and ViewportUiDisplay replaces it with `QToolBar { border: none; }` as it
    // takes the switcher (PrepareWidgetForViewportUi), before the switcher is polished. The toolbar's layout read its spacing when
    // the first sheet was set, and a widget that is not polished is not told its style changed again (QWidget::setStyleSheet), so
    // the layout keeps the 3px of the first sheet: the switcher of the captured legacy Editor has a layout spacing of 3 where a
    // cluster has 12 (m3 Editor 01_main_window, qss). The flattened GUI sets no sheet and gives the layout that spacing itself. The
    // second rule of the sheet (the black buttons) reaches no button of a switcher the viewport UI builds: the switcher is made with
    // an empty button group, and its tool buttons are made and polished after the second sheet is set (the captured button is 86 x 19,
    // which a padding of 7px and a border of 2px would make larger).
    // ----------------------------------------------------------------------------------

    //! Everything private to this file. A named namespace, not an anonymous one: a unity build joins files into one translation
    //! unit, where anonymous namespaces merge.
    namespace ViewportUiSwitcherFlatInternal
    {
        //! The spacing between the switcher's items: `spacing: 3px` of the sheet the ViewportUiSwitcher constructor sets in the legacy
        //! GUI. TOKENIZE: ViewportUiSwitcherItemSpacing
        constexpr int ItemSpacing = 3;

        //! The colour the switcher's button icons are recoloured to (RemoveIconColor: the icon's shape filled with it, SourceIn).
        //! ViewportUiSwitcher.cpp:86. TOKENIZE: ViewportUiSwitcherIconColor
        const QColor IconColor(Qt::white);
    } // namespace ViewportUiSwitcherFlatInternal

    ViewportUiSwitcher::ViewportUiSwitcher(AZStd::shared_ptr<ButtonGroup> buttonGroup)
        : m_buttonGroup(buttonGroup)
    {
        setOrientation(Qt::Orientation::Horizontal);
        setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
        AzQtComponents::StyleManager::setLegacyStyleSheet(
            this,
            QString("QToolBar {background-color: none; border: none; spacing: 3px;}"
                    "QToolButton {background-color: black; border: outset; border-color: white; border-radius: 7px; "
                    "border-width: 2px; padding: 7px; color: white;}"));
        if (AzQtComponents::StyleManager::stylesheetsDisabled())
        {
            layout()->setSpacing(ViewportUiSwitcherFlatInternal::ItemSpacing);
        }

        // Add am empty active button (is set in the call to SetActiveMode)
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
        delete m_activeButton;
    }

    static QPixmap RemoveIconColor(const char* buttonIconPath)
    {
        QPainter painter;
        QPixmap buttonPixmap = QPixmap(QString(buttonIconPath));

        painter.begin(&buttonPixmap);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(buttonPixmap.rect(), ViewportUiSwitcherFlatInternal::IconColor);
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
} // namespace AzToolsFramework::ViewportUi::Internal
