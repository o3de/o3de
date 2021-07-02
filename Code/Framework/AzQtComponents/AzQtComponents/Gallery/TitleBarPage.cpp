/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TitleBarPage.h"
#include <AzQtComponents/Gallery/ui_TitleBarPage.h>

TitleBarPage::TitleBarPage(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::TitleBarPage)
{
    using namespace AzQtComponents;

    ui->setupUi(this);

    ui->activeSimpleTitleBar->setDrawSimple(true);
    ui->activeSimpleButtonsTitleBar->setDrawSimple(true);
    ui->inactiveSimpleTitleBar->setDrawSimple(true);
    ui->inactiveSimpleButtonsTitleBar->setDrawSimple(true);

    ui->activeTearTitleBar->setTearEnabled(true);
    ui->activeTearButtonsTitleBar->setTearEnabled(true);
    ui->inactiveTearTitleBar->setTearEnabled(true);
    ui->inactiveTearButtonsTitleBar->setTearEnabled(true);

    ui->inactiveTitleBar->setForceInactive(true);
    ui->inactiveButtonsTitleBar->setForceInactive(true);
    ui->inactiveSimpleTitleBar->setForceInactive(true);
    ui->inactiveSimpleButtonsTitleBar->setForceInactive(true);
    ui->inactiveTearTitleBar->setForceInactive(true);
    ui->inactiveTearButtonsTitleBar->setForceInactive(true);

    ui->activeButtonsTitleBar->setButtons(
        { DockBarButton::DividerButton, DockBarButton::MinimizeButton,
        DockBarButton::DividerButton, DockBarButton::MaximizeButton,
        DockBarButton::DividerButton, DockBarButton::CloseButton});
    ui->activeSimpleButtonsTitleBar->setButtons(
        { DockBarButton::DividerButton, DockBarButton::MinimizeButton,
        DockBarButton::DividerButton, DockBarButton::MaximizeButton,
        DockBarButton::DividerButton, DockBarButton::CloseButton});
    ui->activeTearButtonsTitleBar->setButtons(
        { DockBarButton::DividerButton, DockBarButton::MinimizeButton,
        DockBarButton::DividerButton, DockBarButton::MaximizeButton,
        DockBarButton::DividerButton, DockBarButton::CloseButton});
    ui->inactiveButtonsTitleBar->setButtons(
        { DockBarButton::DividerButton, DockBarButton::MinimizeButton,
        DockBarButton::DividerButton, DockBarButton::MaximizeButton,
        DockBarButton::DividerButton, DockBarButton::CloseButton});
    ui->inactiveSimpleButtonsTitleBar->setButtons(
        { DockBarButton::DividerButton, DockBarButton::MinimizeButton,
        DockBarButton::DividerButton, DockBarButton::MaximizeButton,
        DockBarButton::DividerButton, DockBarButton::CloseButton});
    ui->inactiveTearButtonsTitleBar->setButtons(
        { DockBarButton::DividerButton, DockBarButton::MinimizeButton,
        DockBarButton::DividerButton, DockBarButton::MaximizeButton,
        DockBarButton::DividerButton, DockBarButton::CloseButton});

    QString exampleText = R"(
    <pre>
    </pre>
    )";

    ui->exampleText->setHtml(exampleText);
}

TitleBarPage::~TitleBarPage()
{
}

#include "Gallery/moc_TitleBarPage.cpp"
