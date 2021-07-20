/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "MultiMonHelper.h"

// Qt
#include <QScreen>

////////////////////////////////////////////////////////////////////////////
void ClipOrCenterRectToMonitor(QRect *prc, const UINT flags)
{
    const QScreen* currentScreen = nullptr;
    QRect rc;

    Q_ASSERT(prc);

    const auto screens = qApp->screens();
    for (auto screen : screens)
    {
        if (screen->geometry().contains(prc->center()))
        {
            currentScreen = screen;
            break;
        }
    }

    if (!currentScreen)
    {
        return;
    }

    const int w = prc->width();
    const int h = prc->height();

    if (flags & MONITOR_WORKAREA)
    {
        rc = currentScreen->availableGeometry();
    }
    else
    {
        rc = currentScreen->geometry();
    }

    // center or clip the passed rect to the monitor rect
    if (flags & MONITOR_CENTER)
    {
        prc->setLeft(rc.left() + (rc.right()  - rc.left() - w) / 2);
        prc->setTop(rc.top() + (rc.bottom() - rc.top() - h) / 2);
        prc->setRight(prc->left() + w);
        prc->setBottom(prc->top() + h);
    }
    else
    {
        prc->setLeft(qMax(rc.left(), qMin(rc.right() - w, prc->left())));
        prc->setTop(qMax(rc.top(), qMin(rc.bottom() - h, prc->top())));
        prc->setRight(prc->left() + w);
        prc->setBottom(prc->top() + h);
    }
}
