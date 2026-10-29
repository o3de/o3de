/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QIcon>
#include <QSize>
#include <QString>

namespace AzQtComponents
{
    //! The icon of an image file (in the given size when it is valid, with the second file for the disabled mode when one is given),
    //! built once and shared: a new QIcon of an SVG file parses the file again, a shared one paints from its pixmap cache. A null icon
    //! (no such file yet) is not kept. For the GUI thread only.
    AZ_QT_COMPONENTS_API QIcon CachedIcon(const QString& filePath, const QSize& size = QSize(), const QString& disabledFilePath = QString());
} // namespace AzQtComponents
