/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SplitterPage.h"
#include <AzQtComponents/Gallery/ui_SplitterPage.h>

SplitterPage::SplitterPage(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::SplitterPage)
{
    ui->setupUi(this);

    QString exampleText = R"(

QSplitter docs: <a href="http://doc.qt.io/qt-5/qsplitter.html">http://doc.qt.io/qt-5/qsplitter.html</a><br/>

)";

    ui->exampleText->setHtml(exampleText);
}

SplitterPage::~SplitterPage()
{
}
