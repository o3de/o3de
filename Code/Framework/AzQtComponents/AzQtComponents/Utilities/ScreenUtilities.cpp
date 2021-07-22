/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Utilities/ScreenUtilities.h>

#include <QApplication>

namespace AzQtComponents
{
    namespace Utilities
    {

        //! Return the screen a point lies in, or the primary screen if no screen was found.
        QScreen* ScreenAtPoint(const QPoint& point)
        {
            QScreen* screen = QApplication::screenAt(point);
            if (!screen)
            {
                return QApplication::primaryScreen();
            }

            return screen;
        }

    } // namespace Utilities
} // namespace AzQtComponents
