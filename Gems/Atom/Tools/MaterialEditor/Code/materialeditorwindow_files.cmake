#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

set(FILES
    Include/Atom/Window/MaterialEditorWindowModule.h
    Include/Atom/Window/MaterialEditorWindowNotificationBus.h
    Include/Atom/Window/MaterialEditorWindowRequestBus.h
    Include/Atom/Window/MaterialEditorWindowFactoryRequestBus.h
    Source/Window/MaterialBrowserInteractions.h
    Source/Window/MaterialBrowserInteractions.cpp
    Source/Window/MaterialEditorWindow.h
    Source/Window/MaterialEditorWindow.cpp
    Source/Window/MaterialEditorWindowModule.cpp
    Source/Window/MaterialBrowserWidget.h
    Source/Window/MaterialBrowserWidget.cpp
    Source/Window/MaterialBrowserWidget.ui
    Source/Window/MaterialEditor.qrc
    Source/Window/MaterialEditorWindowComponent.h
    Source/Window/MaterialEditorWindowComponent.cpp
    Source/Window/PerformanceMonitor/PerformanceMonitorWidget.cpp
    Source/Window/PerformanceMonitor/PerformanceMonitorWidget.h
    Source/Window/PerformanceMonitor/PerformanceMonitorWidget.ui
    Source/Window/ToolBar/FovSliderWidget.h
    Source/Window/ToolBar/FovSliderWidget.cpp
    Source/Window/ToolBar/MaterialEditorToolBar.h
    Source/Window/ToolBar/MaterialEditorToolBar.cpp
    Source/Window/ToolBar/ModelPresetComboBox.h
    Source/Window/ToolBar/ModelPresetComboBox.cpp
    Source/Window/ToolBar/LightingPresetComboBox.h
    Source/Window/ToolBar/LightingPresetComboBox.cpp
    Source/Window/StatusBar/StatusBarWidget.cpp
    Source/Window/StatusBar/StatusBarWidget.h
    Source/Window/StatusBar/StatusBarWidget.ui
    Source/Window/MaterialTypesLibrary.h
    Source/Window/MaterialTypesLibrary.cpp
    Source/Window/MaterialInspector/MaterialInspector.h
    Source/Window/MaterialInspector/MaterialInspector.cpp
    Source/Window/ViewportSettingsInspector/ViewportSettingsInspector.h
    Source/Window/ViewportSettingsInspector/ViewportSettingsInspector.cpp
)
