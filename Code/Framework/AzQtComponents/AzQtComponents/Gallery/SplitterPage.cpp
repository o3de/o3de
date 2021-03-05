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

#include "SplitterPage.h"
#include <Gallery/ui_SplitterPage.h>

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
