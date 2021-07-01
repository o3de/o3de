/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#import <Cocoa/Cocoa.h>

#include <QWidget>

void setCocoaMouseCursor(QWidget* widget)
{
    if (widget == nullptr)
    {
        // no widget, default cursor
        [[NSCursor arrowCursor] set];
    }
    else
    {
        // otherwhise activate the Cocoa mouse
        // cursor matching the one set via Qt
        switch (widget->cursor().shape())
        {
        case Qt::ArrowCursor:
            [[NSCursor arrowCursor] set];
            break;
        case Qt::SizeHorCursor:
        case Qt::SplitVCursor:
            [[NSCursor resizeUpDownCursor] set];
            break;
        case Qt::SizeVerCursor:
        case Qt::SplitHCursor:
            [[NSCursor resizeLeftRightCursor] set];
            break;
        default:
            // for all other cursors we do nothing
            // since this is only to fix the splitter handle cursors for now
            break;
        }
    }
}
