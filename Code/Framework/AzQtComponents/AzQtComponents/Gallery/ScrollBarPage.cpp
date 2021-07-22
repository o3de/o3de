/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ScrollBarPage.h"
#include <AzQtComponents/Gallery/ui_ScrollBarPage.h>

#include <AzQtComponents/Components/Widgets/ScrollBar.h>

ScrollBarPage::ScrollBarPage(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::ScrollBarPage)
{
    ui->setupUi(this);

    using namespace AzQtComponents;

    ScrollBar::setDisplayMode(ui->alwaysShowScrollArea, ScrollBar::ScrollBarMode::AlwaysShow);
    ScrollBar::setDisplayMode(ui->showOnHoverScrollArea, ScrollBar::ScrollBarMode::ShowOnHover);

    ui->defaultScrollArea_dark->setStyleSheet("QScrollArea { background: #FFFFFF; } QLabel { color: #000000; } ");
    ui->alwaysShowScrollArea_dark->setStyleSheet("QScrollArea { background: #FFFFFF; } QLabel { color: #000000; } ");
    ui->showOnHoverScrollArea_dark->setStyleSheet("QScrollArea { background: #FFFFFF; } QLabel { color: #000000; } ");

    ScrollBar::applyDarkStyle(ui->defaultScrollArea_dark);
    ScrollBar::applyDarkStyle(ui->alwaysShowScrollArea_dark);
    ScrollBar::applyDarkStyle(ui->showOnHoverScrollArea_dark);

    ScrollBar::setDisplayMode(ui->alwaysShowScrollArea_dark, ScrollBar::ScrollBarMode::AlwaysShow);
    ScrollBar::setDisplayMode(ui->showOnHoverScrollArea_dark, ScrollBar::ScrollBarMode::ShowOnHover);

    QString exampleText = R"(

QScrollArea docs: <a href="http://doc.qt.io/qt-5/qscrollarea.html">http://doc.qt.io/qt-5/qscrollarea.html</a><br/>

<pre>
#include &lt;QScrollArea&gt;

QScrollArea* scrollArea;
// Assuming you've created a scroll area already (either in code or via .ui file):

using namespace AzQtComponents;

// Modify the display mode by calling ScrollBar::setDisplayMode
ScrollBar::setDisplayMode(ui->showOnHoverScrollArea, ScrollBar::ScrollBarMode::ShowOnHover);

// Set the ScrollBar style to dark, to make the scrollbar visible on light backgrounds
ScrollBar::applyDarkStyle(ui->defaultScrollArea_dark);

// Reset the ScrollBar style to light (default - use the function to revert to it if the above function was used)
ScrollBar::applyLightStyle(ui->defaultScrollArea_dark);

</pre>

)";

    ui->exampleText->setHtml(exampleText);
}

ScrollBarPage::~ScrollBarPage()
{
}

#include <Gallery/moc_ScrollBarPage.cpp>
