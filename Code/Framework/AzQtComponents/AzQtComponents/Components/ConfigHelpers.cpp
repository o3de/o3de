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
