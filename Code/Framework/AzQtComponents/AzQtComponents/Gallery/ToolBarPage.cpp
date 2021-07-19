/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ToolBarPage.h"
#include <AzQtComponents/Gallery/ui_ToolBarPage.h>

#include <AzQtComponents/Components/Widgets/Text.h>
#include <AzQtComponents/Components/Widgets/ToolBar.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Gallery/ComponentDemoWidget.h>

#include <QActionGroup>
#include <QHideEvent>
#include <QMainWindow>
#include <QMenu>
#include <QShowEvent>
#include <QToolButton>
#include <QPushButton>
#include <QDebug>

AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option") // 4251: 'QCss::Declaration::d': class 'QExplicitlySharedDataPointer<QCss::Declaration::DeclarationData>' needs to have dll-interface to be used by clients of struct 'QCss::Declaration'
#include <QtWidgets/private/qstylesheetstyle_p.h>
AZ_POP_DISABLE_WARNING

ToolBarPage::ToolBarPage(QMainWindow* parent)
    : QWidget(parent)
    , m_mainWindow(parent)
    , m_toolBar(new QToolBar(this))
    , ui(new Ui::ToolBarPage)
{
    ui->setupUi(this);

    AzQtComponents::Text::addTitleStyle(ui->mainToolBarLabel);
    AzQtComponents::Text::addTitleStyle(ui->secondaryToolBarLabel);
    AzQtComponents::Text::addTitleStyle(ui->resizableIconMainToolBarLabel);
    AzQtComponents::Text::addTitleStyle(ui->resizableIconSecondaryToolBarLabel);

    AzQtComponents::ToolBar::addMainToolBarStyle(ui->mainToolBar);
    AzQtComponents::ToolBar::addMainToolBarStyle(ui->resizableIconMainToolBar);

    for (auto toolBar : {ui->mainToolBar, ui->secondaryToolBar, ui->resizableIconMainToolBar, ui->resizableIconSecondaryToolBar, m_toolBar})
    {
        QAction* move = toolBar->addAction(QIcon(QStringLiteral(":/Gallery/Move.svg")), {});
        QAction* rotate = toolBar->addAction(QIcon(QStringLiteral(":/Gallery/Rotate.svg")), {});
        QAction* scale = toolBar->addAction(QIcon(QStringLiteral(":/Gallery/Scale.svg")), {});

        QActionGroup* group = new QActionGroup(this);
        group->setExclusive(true);
        for (auto action : {move, rotate, scale})
        {
            action->setCheckable(true);
            group->addAction(action);
        }

        toolBar->addSeparator();

        toolBar->addAction(QIcon(QStringLiteral(":/Gallery/Audio.svg")), {});
        toolBar->addAction(QIcon(QStringLiteral(":/Gallery/Camera.svg")), {});

        toolBar->addSeparator();

        toolBar->addAction(QIcon(QStringLiteral(":/Gallery/Delete.svg")), {});
        toolBar->addSeparator();
        toolBar->addAction(QIcon(QStringLiteral(":/Gallery/Grid-small.svg")), {});
        toolBar->addAction(QIcon(QStringLiteral(":/Gallery/Folder.svg")), {});

        QToolButton* settingsButton = new QToolButton(this);
        settingsButton->setPopupMode(QToolButton::InstantPopup);
        settingsButton->setIcon(QIcon(QStringLiteral(":/Gallery/Settings.svg")));

        QMenu* settingsMenu = new QMenu(this);
        settingsMenu->addAction(QStringLiteral("Configure"));
        settingsButton->setMenu(settingsMenu);

        toolBar->addWidget(settingsButton);
    }

    m_toolBar->setMovable(true);
    m_mainWindow->addToolBar(m_toolBar);
    m_toolBar->hide();

    ui->iconSizeComboBox->addItem(QString("Normal icons"), static_cast<int>(AzQtComponents::ToolBar::ToolBarIconSize::IconNormal));
    ui->iconSizeComboBox->addItem(QString("Large icons"), static_cast<int>(AzQtComponents::ToolBar::ToolBarIconSize::IconLarge));
    ui->iconSizeComboBox->setCurrentIndex(1);

    connect(ui->iconSizeComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &ToolBarPage::refreshIconSizes);
    connect(qobject_cast<ComponentDemoWidget*>(parent), &ComponentDemoWidget::styleChanged, this, &ToolBarPage::refreshIconSizes);

    QString exampleText = R"(

QToolBar docs: <a href="http://doc.qt.io/qt-5/qtoolbar.html">http://doc.qt.io/qt-5/qtoolbar.html</a><br/>

<pre>
#include &lt;QToolBar&gt;

QToolBar* toolbar;
// Assuming you've created a tool bar already (either in code or via .ui file):

using namespace AzQtComponents;

// Make the tool bar a main tool bar:
ToolBar::addMainToolBarStyle(toolbar);

// To add a QToolButton that displays a menu when clicked (rather than long-clicked):
QToolButton* toolButton = new QToolButton(this);
toolButton->setPopupMode(QToolButton::InstantPopup);
toolButton->setIcon(QIcon(QStringLiteral(":/Gallery/Settings.svg")));

QMenu* menu = new QMenu(this);
menu->addAction(QStringLiteral("Configure"));
toolButton->setMenu(settingsMenu);

toolBar->addWidget(toolButton);

</pre>

)";

    ui->exampleText->setHtml(exampleText);
}

ToolBarPage::~ToolBarPage()
{
}


void ToolBarPage::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    m_toolBar->hide();
}

void ToolBarPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    m_toolBar->show();
}

void ToolBarPage::refreshIconSizes()
{
    AzQtComponents::ToolBar::ToolBarIconSize iconSizeEnum = static_cast<AzQtComponents::ToolBar::ToolBarIconSize>(ui->iconSizeComboBox->currentData().toInt());
    AzQtComponents::ToolBar::setToolBarIconSize(ui->resizableIconMainToolBar, iconSizeEnum);
    AzQtComponents::ToolBar::setToolBarIconSize(ui->resizableIconSecondaryToolBar, iconSizeEnum);
}

#include <Gallery/moc_ToolBarPage.cpp>
