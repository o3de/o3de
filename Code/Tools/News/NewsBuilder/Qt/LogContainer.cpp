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

#include "LogContainer.h"

#include "Qt/ui_LogContainer.h"

namespace News
{

    LogContainer::LogContainer(QWidget* parent)
        : QWidget(parent)
        , m_ui(new Ui::LogContainerWidget)
    {
        m_ui->setupUi(this);
    }

    LogContainer::~LogContainer() {}

    void LogContainer::AddLog(QString text, LogType logType) const {
        QString color;
        switch (logType)
        {
        case LogOk:
            color = "green";
            break;
        case LogInfo:
            color = "white";
            break;
        case LogError:
            color = "red";
            break;
        case LogWarning:
            color = "yellow";
            break;
        default:
            color = "white";
            break;
        }
        auto fullText = QString("<span style=\" color:%1; \"><XMP>%2</XMP></span><br>").arg(color).arg(text);
        m_ui->logText->setTextFormat(Qt::RichText);
        m_ui->logText->setText(QString("%2%1").arg(m_ui->logText->text()).arg(fullText));
    }

#include "Qt/moc_LogContainer.cpp"

}
