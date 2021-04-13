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

#include "MenuPage.h"
#include <Gallery/ui_MenuPage.h>

#include <QMenu>

MenuPage::MenuPage(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::MenuPage)
{
    ui->setupUi(this);

    auto menu = ui->menu;

    menu->setWindowFlag(Qt::Popup, false);

    const auto actionText = QStringLiteral("Option");
    menu->addAction(actionText);
    menu->addAction(QIcon(QStringLiteral(":/stylesheet/img/search.svg")), QStringLiteral("Search"));
    menu->addSeparator();

    auto shortcutAction = menu->addAction(actionText);
    shortcutAction->setShortcut(Qt::CTRL + Qt::Key_O);
    shortcutAction->setShortcutVisibleInContextMenu(true);

    auto disabledShortcutAction = menu->addAction(QStringLiteral("Disabled"));
    disabledShortcutAction->setEnabled(false);
    disabledShortcutAction->setShortcut(Qt::CTRL + Qt::Key_D);
    disabledShortcutAction->setShortcutVisibleInContextMenu(true);

    auto submenu = menu->addMenu(QStringLiteral("Submenu"));
    submenu->addAction(actionText);
    submenu->addAction(actionText);
    submenu->addSeparator();
    submenu->addAction(actionText);

    auto checkableAction = menu->addAction(QStringLiteral("Checkable"));
    checkableAction->setCheckable(true);
    checkableAction->setChecked(true);

    auto checkableIconAction = menu->addAction(QIcon(QStringLiteral(":/stylesheet/img/search.svg")), QStringLiteral("Checkable + Icon"));
    checkableIconAction->setCheckable(true);
    checkableIconAction->setChecked(true);

    menu->addSeparator();
    menu->addAction(actionText);

    menu->show();

    QString exampleText = R"(

QMenu docs: <a href="http://doc.qt.io/qt-5/qmenu.html">http://doc.qt.io/qt-5/qmenu.html</a><br/>

<pre>
#include &lt;QMenu&gt;

QMenu* radioButton;

// Assuming you've created a QMenu already:

// To add an action with a shortcut:
auto shortcutAction = menu->addAction("Menu text");
shortcutAction->setShortcut(Qt::CTRL + Qt::Key_O);
shortcutAction->setShortcutVisibleInContextMenu(true);

// To add a checkable action:
auto action = menu->addAction(QStringLiteral("Checkable"));
action->setCheckable(true);
action->setChecked(true);

// To add a sub-menu:
auto submenu = menu->addMenu(QStringLiteral("Submenu"));
submenu->addAction(actionText);

// Note: some Lumberyard menus (like the one in the MainWindow) forcefully hide icons by design.

</pre>

)";

    ui->exampleText->setHtml(exampleText);
}

MenuPage::~MenuPage()
{
}

#include <Gallery/moc_MenuPage.cpp>
