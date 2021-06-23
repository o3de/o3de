/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>

#include <QCursor>

#include <AzQtComponents/Utilities/QtWindowUtilities.h>

namespace AzQtComponents
{

    MappedPoint MappedCursorPosition()
    {
        POINT point;
        GetCursorPos(&point);
        return { { point.x, point.y }, QCursor::pos() };
    }

} // namespace AzQtComponents
