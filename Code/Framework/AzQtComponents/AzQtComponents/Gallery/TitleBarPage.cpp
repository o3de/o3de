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
