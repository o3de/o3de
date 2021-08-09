#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Include/AtomToolsFramework/Application/AtomToolsApplication.h
    Include/AtomToolsFramework/Communication/LocalServer.h
    Include/AtomToolsFramework/Communication/LocalSocket.h
    Include/AtomToolsFramework/Debug/TraceRecorder.h
    Include/AtomToolsFramework/DynamicProperty/DynamicProperty.h
    Include/AtomToolsFramework/DynamicProperty/DynamicPropertyGroup.h
    Include/AtomToolsFramework/Inspector/InspectorWidget.h
    Include/AtomToolsFramework/Inspector/InspectorRequestBus.h
    Include/AtomToolsFramework/Inspector/InspectorNotificationBus.h
    Include/AtomToolsFramework/Inspector/InspectorGroupWidget.h
    Include/AtomToolsFramework/Inspector/InspectorGroupHeaderWidget.h
    Include/AtomToolsFramework/Inspector/InspectorPropertyGroupWidget.h
    Include/AtomToolsFramework/Util/MaterialPropertyUtil.h
    Include/AtomToolsFramework/Util/Util.h
    Include/AtomToolsFramework/Viewport/RenderViewportWidget.h
    Include/AtomToolsFramework/Viewport/ModularViewportCameraController.h
    Include/AtomToolsFramework/Viewport/ModularViewportCameraControllerRequestBus.h
    Include/AtomToolsFramework/Window/AtomToolsMainWindow.h
    Include/AtomToolsFramework/Window/AtomToolsMainWindowRequestBus.h
    Include/AtomToolsFramework/Window/AtomToolsMainWindowFactoryRequestBus.h
    Include/AtomToolsFramework/Window/AtomToolsMainWindowNotificationBus.h
    Source/Application/AtomToolsApplication.cpp
    Source/Communication/LocalServer.cpp
    Source/Communication/LocalSocket.cpp
    Source/Debug/TraceRecorder.cpp
    Source/DynamicProperty/DynamicProperty.cpp
    Source/DynamicProperty/DynamicPropertyGroup.cpp
    Source/Inspector/InspectorWidget.cpp
    Source/Inspector/InspectorWidget.ui
    Source/Inspector/InspectorWidget.qrc
    Source/Inspector/InspectorGroupWidget.cpp
    Source/Inspector/InspectorGroupHeaderWidget.cpp
    Source/Inspector/InspectorPropertyGroupWidget.cpp
    Source/Util/MaterialPropertyUtil.cpp
    Source/Util/Util.cpp
    Source/Viewport/RenderViewportWidget.cpp
    Source/Viewport/ModularViewportCameraController.cpp
    Source/Window/AtomToolsMainWindow.cpp
)