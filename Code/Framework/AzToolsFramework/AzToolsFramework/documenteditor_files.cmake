#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    DocumentEditor/CreateDocumentDialog.h
    DocumentEditor/CreateDocumentDialog.cpp
    DocumentEditor/ToolsAnyDocument.h
    DocumentEditor/ToolsAnyDocument.cpp
    DocumentEditor/ToolsAnyDocumentNotificationBus.h
    DocumentEditor/ToolsAnyDocumentRequestBus.h
    DocumentEditor/ToolsDocument.h
    DocumentEditor/ToolsDocument.cpp
    DocumentEditor/ToolsDocumentApplication.h
    DocumentEditor/ToolsDocumentApplication.cpp
    DocumentEditor/ToolsDocumentInspector.h
    DocumentEditor/ToolsDocumentInspector.cpp
    DocumentEditor/ToolsDocumentMainWindow.h
    DocumentEditor/ToolsDocumentMainWindow.cpp
    DocumentEditor/ToolsDocumentNotificationBus.h
    DocumentEditor/ToolsDocumentObjectInfo.h
    DocumentEditor/ToolsDocumentRequestBus.h
    DocumentEditor/ToolsDocumentSystem.h
    DocumentEditor/ToolsDocumentSystem.cpp
    DocumentEditor/ToolsDocumentSystemRequestBus.h
    DocumentEditor/ToolsDocumentTypeInfo.h
    DocumentEditor/ToolsDocumentTypeInfo.cpp
    DocumentEditor/TraceRecorder.h
    DocumentEditor/TraceRecorder.cpp
    DocumentEditor/ToolsMainWindow.h
    DocumentEditor/ToolsMainWindow.cpp
    DocumentEditor/ToolsMainWindowNotificationBus.h
    DocumentEditor/ToolsMainWindowRequestBus.h
    DocumentEditor/ToolsMainWindowSystemComponent.h
    DocumentEditor/ToolsMainWindowSystemComponent.cpp

    #Inspector
    DocumentEditor/Inspector/PropertyWidgets/PropertyStringBrowseEditCtrl.h
    DocumentEditor/Inspector/PropertyWidgets/PropertyStringBrowseEditCtrl.cpp
    DocumentEditor/Inspector/InspectorGroupHeaderWidget.cpp
    DocumentEditor/Inspector/InspectorGroupHeaderWidget.h
    DocumentEditor/Inspector/InspectorGroupWidget.cpp
    DocumentEditor/Inspector/InspectorGroupWidget.h
    DocumentEditor/Inspector/InspectorNotificationBus.h
    DocumentEditor/Inspector/InspectorPropertyGroupWidget.cpp
    DocumentEditor/Inspector/InspectorPropertyGroupWidget.h
    DocumentEditor/Inspector/InspectorRequestBus.h
    DocumentEditor/Inspector/InspectorWidget.h
    DocumentEditor/Inspector/InspectorWidget.cpp
    DocumentEditor/Inspector/InspectorWidget.ui
    DocumentEditor/Inspector/InspectorWidget.qrc

    #AssetBrowser
    DocumentEditor/AssetBrowser/ToolsAssetBrowser.cpp
    DocumentEditor/AssetBrowser/ToolsAssetBrowser.h
    DocumentEditor/AssetBrowser/ToolsAssetBrowser.qrc
    DocumentEditor/AssetBrowser/ToolsAssetBrowser.ui
    DocumentEditor/AssetBrowser/ToolsAssetBrowserInteractions.cpp
    DocumentEditor/AssetBrowser/ToolsAssetBrowserInteractions.h

    #DynamicProperty
    DocumentEditor/DynamicProperty/DynamicProperty.cpp
    DocumentEditor/DynamicProperty/DynamicProperty.h
    DocumentEditor/DynamicProperty/DynamicPropertyGroup.cpp
    DocumentEditor/DynamicProperty/DynamicPropertyGroup.h

    #Application
    DocumentEditor/Application/DocumentToolsApplication.cpp
    DocumentEditor/Application/DocumentToolsApplication.h

    #SettingsDialog
    DocumentEditor/SettingsDialog/SettingsDialog.h
    DocumentEditor/SettingsDialog/SettingsDialog.cpp

)