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

            case QEvent::ActionAdded:
                actionEvent = static_cast<QActionEvent*>(event);
                actionButton = new QToolButton(this);
                actionButton->setIcon(actionEvent->action()->icon());
                actionButton->setToolTip(actionEvent->action()->text());
                if (!actionEvent->action()->objectName().isEmpty())
                {
                    actionButton->setObjectName(actionEvent->action()->objectName());
                }
                // Forcing styled background to allow using background-color from QSS
                actionButton->setAttribute(Qt::WA_StyledBackground, true);
                layout()->addWidget(actionButton);
                connect(actionButton, &QToolButton::clicked, actionEvent->action(), &QAction::trigger);
                m_actionButtons[actionEvent->action()] = actionButton;
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
