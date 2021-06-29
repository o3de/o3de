/*
 * Copyright (c) Contributors to the Open 3D Engine Project
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
        // We can correctly handle every on-screen pixel on linux
        return { QCursor::pos(), QCursor::pos() };
    }

} // namespace AzQtComponents
