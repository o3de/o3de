/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Window/StatusBar/StatusBarWidget.h>
#include <Source/Window/StatusBar/ui_StatusBarWidget.h>

namespace MaterialEditor
{
    StatusBarWidget::StatusBarWidget(QWidget* parent)
        : QWidget(parent)
        , m_ui(new Ui::StatusBarWidget)
    {
        m_ui->setupUi(this);
    }

    StatusBarWidget::~StatusBarWidget() = default;

    void StatusBarWidget::UpdateStatusInfo(const QString& status)
    {
        m_ui->m_statusLabel->setText(QString("<font color=\"White\">%1</font>").arg(status));
    }
    void StatusBarWidget::UpdateStatusWarning(const QString& status)
    {
        m_ui->m_statusLabel->setText(QString("<font color=\"Yellow\">%1</font>").arg(status));
    }
    void StatusBarWidget::UpdateStatusError(const QString& status)
    {
        m_ui->m_statusLabel->setText(QString("<font color=\"Red\">%1</font>").arg(status));
    }
} // namespace MaterialEditor

#include <Source/Window/StatusBar/moc_StatusBarWidget.cpp>
