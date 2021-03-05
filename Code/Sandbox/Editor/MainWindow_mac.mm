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
