/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "QtEditorApplication.h"

#ifdef PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
#include <AzFramework/XcbEventHandler.h>
#endif

namespace Editor
{
    bool EditorQtApplication::nativeEventFilter([[maybe_unused]] const QByteArray& eventType, void* message, long*)
    {
        if (GetIEditor()->IsInGameMode())
        {
#ifdef PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
            AzFramework::XcbEventHandlerBus::Broadcast(&AzFramework::XcbEventHandler::HandleXcbEvent, static_cast<xcb_generic_event_t*>(message));
#endif
            return true;
        }
        return false;
    }
}
