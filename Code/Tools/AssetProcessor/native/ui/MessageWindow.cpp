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

MessageWindow::MessageWindow(QWidget* parent)
    : QDialog(parent), m_ui(new Ui::MessageWindow())
{
    m_ui->setupUi(this);

    const auto& standardIcon = style()->standardIcon(QStyle::SP_MessageBoxCritical);
    m_ui->icon->setPixmap(standardIcon.pixmap(QSize(64, 64)));
    m_ui->messageList->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
    connect(m_ui->messageList, &QListWidget::customContextMenuRequested, this, &MessageWindow::ShowLineContextMenu);
    connect(m_ui->okButton, &QPushButton::clicked, this, &MessageWindow::accept);
    connect(m_ui->logButton, &QPushButton::clicked, this, &MessageWindow::ViewLogsClicked);
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
    m_messageText = messageText;
    m_ui->messageList->addItems(messageText);
}

void MessageWindow::SetTitleText(QString titleText)
{
    setWindowTitle(titleText);
}

void MessageWindow::ShowLineContextMenu(const QPoint& pos)
{
    QListWidgetItem* listWidgetItem = m_ui->messageList->itemAt(pos);

    if (!listWidgetItem)
    {
        return;
    }

    QMenu menu;
    menu.addAction(tr("Copy Line"), [listWidgetItem]()
    {
        QGuiApplication::clipboard()->setText(listWidgetItem->text());
    });

    menu.addAction(tr("Copy All Lines"), [this]()
    {
        QGuiApplication::clipboard()->setText(m_messageText.join('\n'));
    });

    menu.exec(m_ui->messageList->viewport()->mapToGlobal(pos));
}

void MessageWindow::ViewLogsClicked()
{
    char resolvedDir[AZ_MAX_PATH_LEN * 2];
    QString currentDir;
    AZ::IO::FileIOBase::GetInstance()->ResolvePath("@log@", resolvedDir, sizeof(resolvedDir));

    currentDir = QString::fromUtf8(resolvedDir);

    if (QFile::exists(currentDir))
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(currentDir));
    }
    else
    {
        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "[Error] Logs folder (%s) does not exist.\n", currentDir.toUtf8().constData());
    }
}
