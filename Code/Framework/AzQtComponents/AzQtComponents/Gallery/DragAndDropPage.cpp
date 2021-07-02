/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DragAndDropPage.h"
#include <AzQtComponents/Gallery/ui_DragAndDropPage.h>

#include <QLabel>
#include <AzQtComponents/Components/Style.h>

DragAndDropPage::DragAndDropPage(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::DragAndDropPage)
{
    ui->setupUi(this);

    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    ui->treeWidget->setAutoExpandDelay(2500);

    QString exampleText = R"(

Drag and drop docs: <a href="https://doc.qt.io/qt-5/model-view-programming.html#using-drag-and-drop-with-item-views">https://doc.qt.io/qt-5/model-view-programming.html#using-drag-and-drop-with-item-views</a><br/>

<pre>
#include &lt;QTreeView&gt;

QTreeView* view;

// Assuming you've created a QTreeView already (with model set up):

// To enable dragging from that view
view->setDragEnabled(true);
// To enable dragging to that view
view->viewport()->setAcceptDrops(true);

</pre>

)";

    ui->exampleText->setHtml(exampleText);
}

#include <Gallery/moc_DragAndDropPage.cpp>
