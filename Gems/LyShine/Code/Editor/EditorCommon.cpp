/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UiCanvasEditor_precompiled.h"

#include "EditorCommon.h"

#include <QApplication>
#include <QMimeData>
#include <QClipboard>

bool ClipboardContainsOurDataType()
{
    const QMimeData* mimeData = QApplication::clipboard()->mimeData();

    return mimeData->hasFormat(UICANVASEDITOR_MIMETYPE);
}
