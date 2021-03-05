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

#include "GoToButton.h"
#include <native/ui/ui_GoToButton.h>

namespace AssetProcessor
{

    GoToButton::GoToButton(QWidget* parent) : QWidget(parent), m_ui(new Ui::GoToButton)
    {
        m_ui->setupUi(this);
        m_ui->goToPushButton->installEventFilter(this);
    }

    GoToButton::~GoToButton()
    {

    }

    bool GoToButton::eventFilter(QObject* watched, QEvent* event)
    {
        QPushButton* button = qobject_cast<QPushButton*>(watched);
        if (!button)
        {
            return false;
        }

        if (event->type() == QEvent::Enter)
        {
            button->setIcon(QIcon(":/AssetProcessor_goto_hover.svg"));
            return true;
        }
        else if (event->type() == QEvent::Leave)
        {
            button->setIcon(QIcon(":/AssetProcessor_goto.svg"));
            return true;
        }
        return false;
    }
}

