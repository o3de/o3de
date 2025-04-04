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
#include <QModelIndexList>

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

    //! ModelIndexListHasExactlyOneRow returns true only if the given list of indices has exactly one row represented.
    //! It will return false in all other cases, including when it is empty, or it has multiple different rows represented.
    //! A list of model indexes from for example a selection model can contain different indices representing the same row,
    //! but different columns.  Often, such controls will select an entire row (all columns) when the user clicks on an item,
    //! resulting in for example, 5 modelindexes selected (but they are all referring to the same row, which is the same logical item),
    //! and we need to differentiate this from the case where the user has selected multiple different rows in a multi-select.
    inline bool ModelIndexListHasExactlyOneRow(const QModelIndexList& indexes)
    {
        int numberOfUniqueRows = 0;
        int lastSeenRow = -1;
        for (const auto& element : indexes)
        {
            if (element.row() != lastSeenRow)
            {
                lastSeenRow = element.row();
                numberOfUniqueRows++;
                if (numberOfUniqueRows > 1) // we only care if its exactly 1 row so can early out
                {
                    return false;
                }
            }
        }
        return (numberOfUniqueRows == 1);
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
