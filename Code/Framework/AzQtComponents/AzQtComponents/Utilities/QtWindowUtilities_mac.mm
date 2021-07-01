/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QCursor>

#include <AzQtComponents/Utilities/QtWindowUtilities.h>

namespace AzQtComponents
{

    MappedPoint MappedCursorPosition()
    {
        // We can correctly handle every on-screen pixel on macOS
        return { QCursor::pos(), QCursor::pos() };
    }

} // namespace AzQtComponents
