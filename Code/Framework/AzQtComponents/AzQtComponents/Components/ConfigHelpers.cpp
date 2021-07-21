/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/ConfigHelpers.h>

#include <QPoint>
#include <QPixmap>
#include <QCursor>

namespace AzQtComponents
{
    namespace ConfigHelpers
    {
        /* Template specialization for QPixmap.
         *
         * Entry in *Config.ini should look like this:
         * [key]
         * Path=path/to/pixmap
         */
        template <>
        void read(QSettings& settings, const QString& key, QPixmap& pixmap)
        {
            QString path;
            ConfigHelpers::read<QString>(settings, key, path);

            const QPixmap testPixmap(path);
            if (!testPixmap.isNull())
            {
                pixmap = testPixmap;
            }
        }

        /* Template specialization for QCursor.
         *
         * Entry in *Config.ini should look like this:
         * [key]
         * Path=path/to/cursor/pixmap
         */
        template <>
        void read(QSettings& settings, const QString& key, QCursor& cursor)
        {
            QPixmap cursorPixmap;
            ConfigHelpers::read<QPixmap>(settings, key, cursorPixmap);

            if (!cursorPixmap.isNull())
            {
                cursor = QCursor(cursorPixmap);
            }
        }
    } // namespace ConfigHelpers
} // namespace AzQtComponents
