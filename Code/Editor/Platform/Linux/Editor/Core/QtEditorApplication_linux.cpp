/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "QtEditorApplication_linux.h"

#ifdef PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
#include <AzFramework/XcbEventHandler.h>
#endif

namespace Editor
{
    EditorQtApplication* EditorQtApplication::newInstance(int& argc, char** argv)
    {
#ifdef PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
        return new EditorQtApplicationXcb(argc, argv);
#endif

        return nullptr;
    }

    bool EditorQtApplicationXcb::nativeEventFilter([[maybe_unused]] const QByteArray& eventType, void* message, long*)
    {
        if (GetIEditor()->IsInGameMode())
        {
#ifdef PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
            // We need to handle RAW Input events in a separate loop. This is a workaround to enable XInput2 RAW Inputs using Editor mode.
            // TODO To have this call here might be not be perfect.
            AzFramework::XcbEventHandlerBus::Broadcast(&AzFramework::XcbEventHandler::PollSpecialEvents);

            // Now handle the rest of the events.
            AzFramework::XcbEventHandlerBus::Broadcast(
                &AzFramework::XcbEventHandler::HandleXcbEvent, static_cast<xcb_generic_event_t*>(message));
#endif
            return true;
        }
        return false;
    }
} // namespace Editor
