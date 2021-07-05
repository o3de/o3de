/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

