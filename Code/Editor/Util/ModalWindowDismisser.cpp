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
#include <QEvent>
#include <qmetaobject.h>

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
static std::vector<QEvent::Type> types;

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

bool ModalWindowDismisser::eventFilter(QObject* object, QEvent* event)
{
    if (QDialog* dialog = qobject_cast<QDialog*>(object))
    {
        if (dialog->isModal())
        {
            // if we wait until a Paint event arrives, we can be sure the dialog is fully open
            // and that close() will work correctly.
            types.push_back(event->type());
            QString enumName = ToString(event->type());
//            AZ::Debug::Trace::Output("ModalWindowDismisser", enumName.toStdString().c_str());
            if (event->type() == QEvent::Timer || event->type() == QEvent::Paint)
            {
                auto it = AZStd::find(m_windows.begin(), m_windows.end(), dialog);
                if (it == m_windows.end())
                {
                    m_windows.push_back(dialog);
                }
                dialog->close();
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
