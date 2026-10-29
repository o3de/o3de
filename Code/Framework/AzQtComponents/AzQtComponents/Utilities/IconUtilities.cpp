/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Utilities/IconUtilities.h>

#include <QHash>

namespace AzQtComponents
{
    QIcon CachedIcon(const QString& filePath, const QSize& size, const QString& disabledFilePath)
    {
        // The icons built so far, by their files and size
        static QHash<QString, QIcon> s_icons;
        const QString key = (size.isValid() || !disabledFilePath.isEmpty())
            ? QStringLiteral("%1|%2x%3|%4").arg(filePath).arg(size.width()).arg(size.height()).arg(disabledFilePath)
            : filePath;
        if (const auto found = s_icons.constFind(key); found != s_icons.constEnd())
        {
            return found.value();
        }

        // As QIcon(filePath) builds it, and the disabled image after
        QIcon icon;
        icon.addFile(filePath, size, QIcon::Normal);
        if (!disabledFilePath.isEmpty())
        {
            icon.addFile(disabledFilePath, size, QIcon::Disabled);
        }
        if (!icon.isNull())
        {
            s_icons.insert(key, icon);
        }
        return icon;
    }
} // namespace AzQtComponents
