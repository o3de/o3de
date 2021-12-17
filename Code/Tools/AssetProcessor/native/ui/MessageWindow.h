/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QStringList>
#include <ui/ui_MessageWindow.h>
#endif

class MessageWindow
    : public QDialog
{
public:
    explicit MessageWindow(QWidget* parent = nullptr);
    ~MessageWindow() override;

    void SetHeaderText(QString headerText);
    void SetMessageText(QStringList messageText);
    void SetTitleText(QString titleText);
    void ShowLineContextMenu(const QPoint& pos);

private:
    Ui::MessageWindow* m_ui{};
    QStringList m_messageText;
};
