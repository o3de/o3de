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
#include <QMetaEnum>

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

template<typename EnumType>
QString ToString(const EnumType& enumValue)
{
    const char* enumName = qt_getEnumName(enumValue);
    const QMetaObject* metaObject = qt_getEnumMetaObject(enumValue);
    if (metaObject)
    {
        const int enumIndex = metaObject->indexOfEnumerator(enumName);
        return QString("%1::%2::%3").arg(metaObject->className(), enumName, metaObject->enumerator(enumIndex).valueToKey(enumValue));
    }

    return QString("%1::%2").arg(enumName).arg(static_cast<int>(enumValue));
}

void ModalWindowDismisser::DismissWindows()
{
    for (QDialog* dialog : m_windows)
    {
        Log("************ AssetImporterManager:: Rejecting dialog");
//        dialog->done(QDialog::Rejected);
        dialog->close();
    }
    m_windows.clear();
    m_dismiss = false;
}

bool ModalWindowDismisser::eventFilter(QObject* object, QEvent* event)
{
    if (QDialog* dialog = qobject_cast<QDialog*>(object))
    {
        if (dialog->isModal())
        {
            QEvent::Type test = event->type();
            Log("************ AssetImporterManager:: Event = %s", ToString(test).toStdString().c_str());
            if (test == QEvent::PolishRequest)
            {
                auto it = AZStd::find(m_windows.begin(), m_windows.end(), dialog);
                if (it == m_windows.end())
                {
                    m_windows.push_back(dialog);
                }
                if (!m_dismiss)
                {
                    // Closing the window at the same moment is opened leads to crashes and is unstable,
                    // so do it after a long 1 ms
                    QTimer::singleShot(50, this, &ModalWindowDismisser::DismissWindows);
                    m_dismiss = true;
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
