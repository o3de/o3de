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
