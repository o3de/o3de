/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <QString>
#include <CryCommon/StlUtils.h>

#include <QApplication>
#include <QDropEvent>
#include <QWidget>

#ifdef LoadCursor
#undef LoadCursor
#endif

class QWaitCursor
{
public:
    QWaitCursor()
    {
        QGuiApplication::setOverrideCursor(Qt::BusyCursor);
    }
    ~QWaitCursor()
    {
        QGuiApplication::restoreOverrideCursor();
    }
};

namespace QtUtil
{
    inline QString trimRight(const QString& str)
    {
        // We prepend a char, so that the left doesn't get trimmed, then we remove it after trimming
        return QString(QStringLiteral("A") + str).trimmed().remove(0, 1);
    }

    template<typename ... Args>
    struct Select
    {
        template<typename C, typename R>
        static auto OverloadOf(R(C::* pmf)(Args...))->decltype(pmf) {
            return pmf;
        }
    };
}

namespace stl
{
    //! Case insensitive less key for QString
    template <>
    struct less_stricmp<QString>
    {
        bool operator()(const QString& left, const QString& right) const
        {
            return left.compare(right, Qt::CaseInsensitive) < 0;
        }
    };
}
