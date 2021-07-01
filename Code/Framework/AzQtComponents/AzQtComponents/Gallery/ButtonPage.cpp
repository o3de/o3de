/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ButtonPage.h"
#include "FixedStateButton.h"
#include <AzQtComponents/Gallery/ui_ButtonPage.h>

#include <AzQtComponents/Components/Widgets/PushButton.h>

#include <QMenu>
#include <QPushButton>

ButtonPage::ButtonPage(QWidget* parent)
: QWidget(parent)
, ui(new Ui::ButtonPage)
{
    ui->setupUi(this);

    const QStyle::State hoverState {QStyle::State_Enabled | QStyle::State_Raised | QStyle::State_Active | QStyle::State_MouseOver};
    const QStyle::State pressedState {QStyle::State_Enabled | QStyle::State_Sunken | QStyle::State_Active | QStyle::State_MouseOver};
    const QStyle::State focusState {QStyle::State_Enabled | QStyle::State_Raised | QStyle::State_Active | QStyle::State_HasFocus};

    ui->primaryHover->setState(hoverState);
    ui->primaryPressed->setState(pressedState);
    ui->primaryFocused->setState(focusState);

    ui->secondaryHover->setState(hoverState);
    ui->secondaryPressed->setState(pressedState);
    ui->secondaryFocused->setState(focusState);

    QMenu* menu = new QMenu(this);
    menu->addAction("Option");
    auto selected = menu->addAction("Selected");
    selected->setCheckable(true);
    selected->setChecked(true);
    menu->addAction("Hover");
    menu->addAction("Option");

    const QList<QPushButton*> menuButtons = {ui->dropdown, ui->dropdownHover, ui->dropdownPressed, ui->dropdownDisabled};

    for (auto button : menuButtons)
    {
        button->setMenu(menu);
    }

    ui->dropdown->setMenu(menu);
    ui->dropdownHover->setState(hoverState);
    ui->dropdownPressed->setState(pressedState);

    const QList<QToolButton*> menuToolButtons = {ui->toolButtonMenu, ui->toolButtonMenuHover, ui->toolButtonMenuDisabled, ui->toolButtonMenuSelected};

    for (auto button : menuToolButtons)
    {
        button->setMenu(menu);
    }

    QString exampleText = R"(

QPushButton docs: <a href="http://doc.qt.io/qt-5/qpushbutton.html">http://doc.qt.io/qt-5/qpushbutton.html</a><br/>
QToolButton docs: <a href="http://doc.qt.io/qt-5/qpushbutton.html">http://doc.qt.io/qt-5/qtoolbutton.html</a><br/>

Use Push Buttons for regular test buttons, and Tool Buttons for icons.

<pre>
#include &lt;QPushButton&gt;
#include &lt;QToolButton&gt;
#include &lt;AzQtComponents/Components/Widgets/PushButton.h&gt;

QPushButton* pushButton;

// Assuming you've created a QPushButton already (either in code or via .ui file):

// To make a button into a "Primary" button:
pushButton->setAutoDefault(true);

// To set a drop down menu, just do it the normal Qt way:
QMenu* menu;
pushButton->setMenu(menu);

// To disable the button
pushButton->setEnabled(false);

// Assuming you've created a QToolButton already (either in code or via .ui file):
QIcon icon(":/stylesheet/img/logging/add-filter.svg");
QToolButton* smallIconButton;
smallIconButton->setIcon(icon);

// To make the button into a small icon button:
PushButton::applySmallIconStyle(smallIconButton);

// disabling a button and adding a menu work exactly the same way between QToolButton and QPushButton

</pre>

)";

    ui->exampleText->setHtml(exampleText);
}

ButtonPage::~ButtonPage()
{
}

#include "Gallery/moc_ButtonPage.cpp"
