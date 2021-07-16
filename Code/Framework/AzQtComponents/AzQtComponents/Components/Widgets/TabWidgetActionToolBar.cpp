/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/TabWidgetActionToolBar.h>

#include <QAction>
#include <QActionEvent>
#include <QHBoxLayout>
#include <QToolButton>

namespace AzQtComponents
{
    TabWidgetActionToolBar::TabWidgetActionToolBar(QWidget* parent)
        : QWidget(parent)
    {
        setLayout(new QHBoxLayout(this));
        setAttribute(Qt::WA_StyledBackground, true);
        layout()->setContentsMargins({0, 0, 0, 0});
        setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    }

    bool TabWidgetActionToolBar::eventFilter(QObject* watched, QEvent* event)
    {
        switch (event->type())
        {
            QToolButton* actionButton;
            QActionEvent* actionEvent;
            QAction* action;

            case QEvent::ActionAdded:
                actionEvent = static_cast<QActionEvent*>(event);
                actionButton = new QToolButton(this);
                action = actionEvent->action();
                actionButton->setIcon(action->icon());
                actionButton->setToolTip(action->text());
                if (!action->objectName().isEmpty())
                {
                    actionButton->setObjectName(action->objectName());
                }

                // If the action has a menu, we need to configure the QToolButton appropriately
                if (action->menu())
                {
                    actionButton->setAutoRaise(true);
                    actionButton->setPopupMode(QToolButton::InstantPopup);
                    actionButton->setMenu(action->menu());
                }

                // Forcing styled background to allow using background-color from QSS
                actionButton->setAttribute(Qt::WA_StyledBackground, true);
                layout()->addWidget(actionButton);
                connect(actionButton, &QToolButton::clicked, action, &QAction::trigger);
                m_actionButtons[action] = actionButton;
                emit actionsChanged();
                return true;
            case QEvent::ActionChanged:
                actionEvent = static_cast<QActionEvent*>(event);
                actionButton = m_actionButtons[actionEvent->action()];
                if (actionButton)
                {
                    actionButton->setIcon(actionEvent->action()->icon());
                    actionButton->setText(actionEvent->action()->text());
                }
                return true;
            case QEvent::ActionRemoved:
                actionEvent = static_cast<QActionEvent*>(event);
                removeWidgetFromLayout(m_actionButtons[actionEvent->action()]);
                m_actionButtons.remove(actionEvent->action());
                emit actionsChanged();
                return true;
            default:
                return QObject::eventFilter(watched, event);
        }
    }

    void TabWidgetActionToolBar::removeWidgetFromLayout(QWidget* widget)
    {
        auto* currentLayout = qobject_cast<QHBoxLayout*>(layout());

        if (!widget || !currentLayout)
        {
            return;
        }

        int widgetIndex = currentLayout->indexOf(widget);
        if (widgetIndex < 0)
        {
            return;
        }

        currentLayout->takeAt(widgetIndex);
        delete widget;

        return;
    }
} // namespace AzQtComponents

#include "Components/Widgets/moc_TabWidgetActionToolBar.cpp"
