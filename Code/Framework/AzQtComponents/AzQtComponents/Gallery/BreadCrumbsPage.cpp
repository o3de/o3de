/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "BreadCrumbsPage.h"
#include <AzQtComponents/Gallery/ui_BreadCrumbsPage.h>

#include <QDebug>
#include <QStyle>
#include <QToolButton>
#include <QToolBar>
#include <QPushButton>
#include <QFileDialog>
#include <AzQtComponents/Components/Widgets/BreadCrumbs.h>


BreadCrumbsPage::BreadCrumbsPage(QWidget* parent)
: QWidget(parent)
, ui(new Ui::BreadCrumbsPage)
{
    ui->setupUi(this);

    AzQtComponents::BreadCrumbs* breadCrumbs = new AzQtComponents::BreadCrumbs(this);

    // Seed the bread crumbs with an arbitrary path, to make the example obvious
    breadCrumbs->pushPath("C:/Documents/SubDirectory1/SubDirectory2/SubDirectory3");
    breadCrumbs->setEditable(true);

    // Have the bread crumb widget create the right buttons for us and we just lay them out
    ui->horizontalLayout->addWidget(breadCrumbs->createBackForwardToolBar());

    ui->horizontalLayout->addWidget(breadCrumbs->createSeparator());

    ui->horizontalLayout->addWidget(breadCrumbs);

    ui->horizontalLayout->addWidget(breadCrumbs->createSeparator());

    auto browseButton = breadCrumbs->createButton(AzQtComponents::NavigationButton::Browse);

    ui->horizontalLayout->addWidget(browseButton);

    connect(browseButton, &QPushButton::pressed, breadCrumbs, [breadCrumbs] {
        QString newPath = QFileDialog::getExistingDirectory(breadCrumbs, "Please select a new path to push into the bread crumbs");
        if (!newPath.isEmpty())
        {
            breadCrumbs->pushPath(newPath);
        }
    });

    connect(breadCrumbs, &AzQtComponents::BreadCrumbs::pathChanged, this, [](const QString& newPath) {
        qDebug() << QString("New path: %1").arg(newPath);
    });

    QString exampleText = R"(

The BreadCrumbs widget can be used to show paths, and provide functionality to click on pieces of the path and jump to them.<br/><br/>

Some sample code:<br/><br/>

<pre>
#include &lt;AzQtComponents/Components/Widgets/BreadCrumbs.h&gt;

// Create a new BreadCrumbs widget:
AzQtComponents::BreadCrumbs* breadCrumbs = new AzQtComponents::BreadCrumbs(this);

// To set the initial path or jump to another
QString somePath = "C:\\Documents";
breadCrumbs->pushPath(somePath);

// To navigate:
breadCrumbs->back();
breadCrumbs->forward();

// Listen for path changes
connect(breadCrumbs, &AzQtComponents::BreadCrumbs::pathChanged, this, [](const QString& newPath){
    // Handle path change
});

// To get the current path:
QString currentPath = breadCrumbs->currentPath();

// To make breadcrumbs editable by the user.
breadCrumbs->setEditable(true);
connect(breadCrumbs, &AzQtComponents::BreadCrumbs::pathEdited, this, [breadCrumbs](const QString& requestedPath){
    // Handle user request
    // WARNING: breadcrumbs themselves won't change the path. If user request is valid, set the path:
    breadCrumbs->pushPath(requestedPath);
});

// Create auto-connected navigation buttons and layout everything in a group widget:
QWidget* group = new QWidget(this);
QHBoxLayout* groupLayout = new QHBoxLayout(group);

AzQtComponents::BreadCrumbs* breadCrumbs = new AzQtComponents::BreadCrumbs(group);

layout->addWidget(breadCrumbs->createBackForwardToolBar());

layout->addWidget(breadCrumbs);

</pre>

)";

    ui->exampleText->setHtml(exampleText);
}

BreadCrumbsPage::~BreadCrumbsPage()
{
}

#include "Gallery/moc_BreadCrumbsPage.cpp"
