/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/ToolBarArea.h>
#include <QToolBar>
#include <QVBoxLayout>

namespace AzQtComponents
{
    ToolBarArea::ToolBarArea(QWidget *parent)
        : QMainWindow(parent)
    {
        // this will prevent this toolbar area from being temporarily styled with a custom title bar
        setProperty("HasNoWindowDecorations", true);

        // this will prevent this toolbar area from being created as a full window
        setWindowFlags(windowFlags() & ~Qt::Window);
    }

    QToolBar* ToolBarArea::CreateToolBarFromWidget(QWidget* sourceWidget, Qt::ToolBarArea area, QString title)
    {
        QToolBar* toolbar = new QToolBar(title, this);
        addToolBar(area, toolbar);

        QLayout* layout = sourceWidget->layout();
        while (QLayoutItem* item = layout->takeAt(0))
        {
            if (QWidget* widget = item->widget())
            {
                toolbar->addWidget(widget);
            }
            else if (item->spacerItem())
            {
                // Old default behavior - insert expanding spacer widget where separators are.
                if (item->sizeHint().width() > 1)
                {
                    QWidget* spacerWidget = new QWidget;
                    spacerWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                    toolbar->addWidget(spacerWidget);
                }
                // If the spacer is sized 1 px, honor that and insert a regular separator.
                else
                {
                    toolbar->addSeparator();
                }
            }
            delete item;
        }

        sourceWidget->hide();
        return toolbar;
    }

    void ToolBarArea::SetMainWidget(QWidget *widget)
    {
        if (!m_mainLayout)
        {
            QWidget* mainWidget = new QWidget;
            m_mainLayout = new QVBoxLayout(mainWidget);
            m_mainLayout->setContentsMargins(QMargins());
            setCentralWidget(mainWidget);
        }

        // We only have one main widget, clear the others.
        // We don't assume responsibility for deleting any widgets left behind, however.
        while (QLayoutItem* item = m_mainLayout->takeAt(0))
            delete item;

        m_mainLayout->addWidget(widget);
        m_mainWidget = widget;
    }
}
