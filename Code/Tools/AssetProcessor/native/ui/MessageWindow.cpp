/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(Q_MOC_RUN)
#include <ui/MessageWindow.h>
#endif

MessageWindow::MessageWindow()
    : m_ui(new Ui::MessageWindow())
{
    m_ui->setupUi(this);

    const auto& standardIcon = style()->standardIcon(QStyle::SP_MessageBoxCritical);
    m_ui->icon->setPixmap(standardIcon.pixmap(QSize(64, 64)));
}

MessageWindow::~MessageWindow()
{
    delete m_ui;
}

void MessageWindow::SetHeaderText(QString headerText)
{
    m_ui->headerText->setText(headerText);
}
void MessageWindow::SetMessageText(QStringList messageText)
{
    m_ui->messageList->addItems(messageText);
}
void MessageWindow::SetTitleText(QString titleText)
{
    setWindowTitle(titleText);
}
