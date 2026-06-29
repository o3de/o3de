/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QRect>
#include <QColor>
#include <QPixmap>
#include <QString>
#include <QPointF>
#include <QRectF>
#include <QVector>
#include <QWidget>
#include <QWindow>
#include <QApplication>

#ifdef Q_OS_WIN
# include <objidl.h>
#endif // Q_OS_WIN

#include "Util/EditorUtils.h"

namespace QtUtil
{

    //! a helper class which captures the HWND of a given widget for the duration of its life cycle.
    //! used mainly to set the parent of popup dialogs like file dialogs which are still MFC classes.
    class QtMFCScopedHWNDCapture
    {
    public:
        QtMFCScopedHWNDCapture(QWidget* source = nullptr)
        {
            if (!source)
            {
                source = qApp->activeWindow();
            }

            if (source)
            {
                m_widget = source;
            }
        }

        ~QtMFCScopedHWNDCapture()
        {
            m_widget = nullptr;
            m_attached = false;
        }

        // this is here so that it will also work for widgets that need parents if we upgrade the file dialog and other dialogs to be widget based
        operator QWidget*()
        {
            return m_widget;
        }

    private:
        bool m_attached = false;
        QWidget* m_widget = nullptr;
    };
}
