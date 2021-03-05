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
