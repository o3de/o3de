/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "QtEditorApplication.h"

namespace Editor
{
    bool EditorQtApplication::nativeEventFilter(const QByteArray& , void* , long* )
    {
        // TODO_KDAB_LINUX
        return false;
    }
}
