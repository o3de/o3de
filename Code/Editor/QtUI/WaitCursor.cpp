/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "WaitCursor.h"

// Qt
#include <QApplication>


WaitCursor::WaitCursor()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
}

WaitCursor::~WaitCursor()
{
    QApplication::restoreOverrideCursor();
}
