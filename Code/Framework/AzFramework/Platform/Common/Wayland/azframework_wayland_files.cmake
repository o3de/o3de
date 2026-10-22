#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    AzFramework/WaylandApplication.cpp
    AzFramework/WaylandApplication.h
    AzFramework/WaylandConnectionManager.h

    AzFramework/WaylandNativeWindow.cpp
    AzFramework/WaylandNativeWindow.h

    AzFramework/WaylandInputDeviceMouse.cpp
    AzFramework/WaylandInputDeviceMouse.h
    AzFramework/WaylandInputDeviceKeyboard.cpp
    AzFramework/WaylandInputDeviceKeyboard.h

    AzFramework/Protocols/ProtocolManager.cpp
    AzFramework/Protocols/ProtocolManager.h
    AzFramework/Protocols/CursorShapeManager.h
    AzFramework/Protocols/CursorShapeManager.cpp
    AzFramework/Protocols/OutputManager.h
    AzFramework/Protocols/OutputManager.cpp
    AzFramework/Protocols/SeatManager.h
    AzFramework/Protocols/SeatManager.cpp

    #XDG
    AzFramework/Protocols/XdgManager.cpp
    AzFramework/Protocols/XdgManager.h

    #Helper interfaces
    AzFramework/Protocols/CursorShapeInterface.h
    AzFramework/Protocols/RelativePointerInterface.h
)
