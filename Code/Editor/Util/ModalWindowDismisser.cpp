/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

void ModalWindowDismisser::DismissWindows()
{
    for (QDialog* dialog : m_windows)
    {
        dialog->close();
    }
    m_windows.clear();
    m_dissmiss = false;
}

bool ModalWindowDismisser::eventFilter(QObject* object, QEvent* event)
{
    if (QDialog* dialog = qobject_cast<QDialog*>(object))
    {
        if (dialog->isModal())
        {
            if (event->type() == QEvent::Show)
            {
                auto it = AZStd::find(m_windows.begin(), m_windows.end(), dialog);
                if (it == m_windows.end())
                {
                    m_windows.push_back(dialog);
                }
                if (!m_dissmiss)
                {
                    // Closing the window at the same moment is opened leads to crashes and is unstable,
                    // so do it after a long 1 ms
                    QTimer::singleShot(1, this, &ModalWindowDismisser::DismissWindows);
                    m_dissmiss = true;
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
