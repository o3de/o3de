/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Description : Utility for dismissing every modal windows

#include "EditorDefs.h"

#include "ModalWindowDismisser.h"

// Qt
#include <QDialog>
#include <QTimer>

ModalWindowDismisser::ModalWindowDismisser()
{
    qApp->installEventFilter(this);
}

ModalWindowDismisser::~ModalWindowDismisser()
{
    if (qApp)
    {
        qApp->removeEventFilter(this);
    }
}

bool ModalWindowDismisser::eventFilter(QObject* object, QEvent* event)
{
    if (QDialog* dialog = qobject_cast<QDialog*>(object))
    {
        if (dialog->isModal())
        {
            // if we wait until a PolishRequest event arrives, we can be sure the dialog is fully open
            // and that close() will work correctly. However, for the Import FileDialog to work correctly
            // there must be a delay on Jenkins. I have tried to remove this SingleShot but nothing seems
            // to make the closing of dialogs happen consistently without it.
            if (event->type() == QEvent::PolishRequest)
            {
                auto it = AZStd::find(m_windows.begin(), m_windows.end(), dialog);
                if (it == m_windows.end())
                {
                    m_windows.push_back(dialog);
                    QTimer::singleShot(2, this, [dialog](){ dialog->close(); });
                }
            }
            else if (event->type() == QEvent::Close)
            {
                auto it = AZStd::find(m_windows.begin(), m_windows.end(), dialog);
                if (it != m_windows.end())
                {
                    m_windows.erase(it);
                }
            }
        }
    }
    return false;
}
