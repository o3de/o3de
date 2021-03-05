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

#include "DeploymentsWidget.h"
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <QPainter>
#include <QInputDialog>

using namespace AzQtComponents;

DeploymentsWidget::DeploymentsWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DeploymentsWidget())
{
    setFixedSize(263, 238);
    ui->setupUi(this);
    ui->titlebar->setWindowTitleOverride(tr("Deployments"));
    //ui->titlebar->setFrameHackEnabled(true);
    ui->titlebar->setForceInactive(true);
    ui->deploymentOk->setProperty("class", "Primary");
    ui->contents->setAttribute(Qt::WA_TranslucentBackground);
    ui->blueLabel->setProperty("class", "addDeployment"); // Should this label style be more generic ?
    ui->blueLabel->installEventFilter(this);
    ui->blueLabel->setCursor(Qt::PointingHandCursor);
}

void DeploymentsWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    StyledDockWidget::drawFrame(p, rect(), false);

    p.setPen(QColor(35, 36, 37));
    p.drawLine(1, height()-51, width() - 2, height() - 51);
    p.setPen(QColor(66, 67, 68));
    p.drawLine(1, height()-50, width() - 2, height() - 50);
}

bool DeploymentsWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        addDeployment();
        return true;
    }

    return QWidget::eventFilter(watched, event);
}

void DeploymentsWidget::addDeployment()
{
    QInputDialog::getText(this, tr("Enter deployment name"), QString());
}

#include "StyleGallery/moc_DeploymentsWidget.cpp"
