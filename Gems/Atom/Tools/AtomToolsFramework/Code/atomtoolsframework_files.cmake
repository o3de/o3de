#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Source/AtomToolsFrameworkSystemComponent.cpp
    Source/AtomToolsFrameworkSystemComponent.h

    Include/AtomToolsFramework/Application/AtomToolsApplication.h
    Source/Application/AtomToolsApplication.cpp

    Include/AtomToolsFramework/AssetBrowser/AtomToolsAssetBrowser.h
    Include/AtomToolsFramework/AssetBrowser/AtomToolsAssetBrowserInteractions.h
    Source/AssetBrowser/AtomToolsAssetBrowser.cpp
    Source/AssetBrowser/AtomToolsAssetBrowser.qrc
    Source/AssetBrowser/AtomToolsAssetBrowser.ui
    Source/AssetBrowser/AtomToolsAssetBrowserInteractions.cpp

    Include/AtomToolsFramework/AtomToolsFrameworkSystemRequestBus.h

    Include/AtomToolsFramework/AssetSelection/AssetSelectionComboBox.h
    Include/AtomToolsFramework/AssetSelection/AssetSelectionGrid.h
    Source/AssetSelection/AssetSelectionComboBox.cpp
    Source/AssetSelection/AssetSelectionGrid.cpp
    Source/AssetSelection/AssetSelectionGrid.ui

    Include/AtomToolsFramework/Communication/LocalServer.h
    Include/AtomToolsFramework/Communication/LocalSocket.h
    Source/Communication/LocalServer.cpp
    Source/Communication/LocalSocket.cpp

    Include/AtomToolsFramework/Debug/TraceRecorder.h
    Source/Debug/TraceRecorder.cpp

    Include/AtomToolsFramework/Document/AtomToolsAnyDocument.h
    Include/AtomToolsFramework/Document/AtomToolsAnyDocumentNotificationBus.h
    Include/AtomToolsFramework/Document/AtomToolsAnyDocumentRequestBus.h
    Include/AtomToolsFramework/Document/AtomToolsDocument.h
    Include/AtomToolsFramework/Document/AtomToolsDocumentApplication.h
    Include/AtomToolsFramework/Document/AtomToolsDocumentInspector.h
    Include/AtomToolsFramework/Document/AtomToolsDocumentMainWindow.h
    Include/AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h
    Include/AtomToolsFramework/Document/AtomToolsDocumentObjectInfo.h
    Include/AtomToolsFramework/Document/AtomToolsDocumentRequestBus.h
    Include/AtomToolsFramework/Document/AtomToolsDocumentSystem.h
    Include/AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h
    Include/AtomToolsFramework/Document/AtomToolsDocumentTypeInfo.h
    Include/AtomToolsFramework/Document/CreateDocumentDialog.h
    Source/Document/AtomToolsAnyDocument.cpp
    Source/Document/AtomToolsDocument.cpp
    Source/Document/AtomToolsDocumentApplication.cpp
    Source/Document/AtomToolsDocumentInspector.cpp
    Source/Document/AtomToolsDocumentMainWindow.cpp
    Source/Document/AtomToolsDocumentSystem.cpp
    Source/Document/AtomToolsDocumentTypeInfo.cpp
    Source/Document/CreateDocumentDialog.cpp

    Include/AtomToolsFramework/DynamicProperty/DynamicProperty.h
    Include/AtomToolsFramework/DynamicProperty/DynamicPropertyGroup.h
    Source/DynamicProperty/DynamicProperty.cpp
    Source/DynamicProperty/DynamicPropertyGroup.cpp

    Include/AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportContent.h
    Include/AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportInputController.h
    Include/AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportScene.h
    Include/AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettings.h
    Include/AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsInspector.h
    Include/AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsNotificationBus.h
    Include/AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsRequestBus.h
    Include/AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsSystem.h
    Include/AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportToolBar.h
    Include/AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportWidget.h
    Source/EntityPreviewViewport/EntityPreviewViewportContent.cpp
    Source/EntityPreviewViewport/EntityPreviewViewportInputController.cpp
    Source/EntityPreviewViewport/EntityPreviewViewportScene.cpp
    Source/EntityPreviewViewport/EntityPreviewViewportSettings.cpp
    Source/EntityPreviewViewport/EntityPreviewViewportSettingsInspector.cpp
    Source/EntityPreviewViewport/EntityPreviewViewportSettingsSystem.cpp
    Source/EntityPreviewViewport/EntityPreviewViewportToolBar.cpp
    Source/EntityPreviewViewport/EntityPreviewViewportWidget.cpp

    Include/AtomToolsFramework/Graph/DynamicNode/DynamicNode.h
    Include/AtomToolsFramework/Graph/DynamicNode/DynamicNodeConfig.h
    Include/AtomToolsFramework/Graph/DynamicNode/DynamicNodeManager.h
    Include/AtomToolsFramework/Graph/DynamicNode/DynamicNodeManagerRequestBus.h
    Include/AtomToolsFramework/Graph/DynamicNode/DynamicNodePaletteItem.h
    Include/AtomToolsFramework/Graph/DynamicNode/DynamicNodeSlotConfig.h
    Include/AtomToolsFramework/Graph/DynamicNode/DynamicNodeUtil.h
    Source/Graph/DynamicNode/DynamicNode.cpp
    Source/Graph/DynamicNode/DynamicNodeConfig.cpp
    Source/Graph/DynamicNode/DynamicNodeManager.cpp
    Source/Graph/DynamicNode/DynamicNodePaletteItem.cpp
    Source/Graph/DynamicNode/DynamicNodeSlotConfig.cpp
    Source/Graph/DynamicNode/DynamicNodeUtil.cpp

    Include/AtomToolsFramework/Graph/AssetStatusReporter.h
    Include/AtomToolsFramework/Graph/AssetStatusReporterState.h
    Include/AtomToolsFramework/Graph/AssetStatusReporterSystem.h
    Include/AtomToolsFramework/Graph/AssetStatusReporterSystemRequestBus.h
    Include/AtomToolsFramework/Graph/GraphCompiler.h
    Include/AtomToolsFramework/Graph/GraphDocument.h
    Include/AtomToolsFramework/Graph/GraphDocumentNotificationBus.h
    Include/AtomToolsFramework/Graph/GraphDocumentRequestBus.h
    Include/AtomToolsFramework/Graph/GraphDocumentView.h
    Include/AtomToolsFramework/Graph/GraphTemplateFileData.h
    Include/AtomToolsFramework/Graph/GraphTemplateFileDataCache.h
    Include/AtomToolsFramework/Graph/GraphTemplateFileDataCacheRequestBus.h
    Include/AtomToolsFramework/Graph/GraphUtil.h
    Include/AtomToolsFramework/Graph/GraphView.h
    Include/AtomToolsFramework/Graph/GraphViewConstructPresets.h
    Include/AtomToolsFramework/Graph/GraphViewSettings.h
    Source/Graph/AssetStatusReporter.cpp
    Source/Graph/AssetStatusReporterSystem.cpp
    Source/Graph/GraphCompiler.cpp
    Source/Graph/GraphDocument.cpp
    Source/Graph/GraphDocumentView.cpp
    Source/Graph/GraphTemplateFileData.cpp
    Source/Graph/GraphTemplateFileDataCache.cpp
    Source/Graph/GraphUtil.cpp
    Source/Graph/GraphView.cpp
    Source/Graph/GraphView.qrc
    Source/Graph/GraphView.qss
    Source/Graph/GraphViewConstructPresets.cpp
    Source/Graph/GraphViewSettings.cpp

    Include/AtomToolsFramework/Inspector/InspectorGroupHeaderWidget.h
    Include/AtomToolsFramework/Inspector/InspectorGroupWidget.h
    Include/AtomToolsFramework/Inspector/InspectorNotificationBus.h
    Include/AtomToolsFramework/Inspector/InspectorPropertyGroupWidget.h
    Include/AtomToolsFramework/Inspector/InspectorRequestBus.h
    Include/AtomToolsFramework/Inspector/InspectorWidget.h
    Source/Inspector/InspectorGroupHeaderWidget.cpp
    Source/Inspector/InspectorGroupWidget.cpp
    Source/Inspector/InspectorPropertyGroupWidget.cpp
    Source/Inspector/InspectorWidget.cpp
    Source/Inspector/InspectorWidget.qrc
    Source/Inspector/InspectorWidget.ui
    Source/Inspector/PropertyWidgets/PropertyStringBrowseEditCtrl.cpp
    Source/Inspector/PropertyWidgets/PropertyStringBrowseEditCtrl.h

    Include/AtomToolsFramework/PerformanceMonitor/PerformanceMetrics.h
    Include/AtomToolsFramework/PerformanceMonitor/PerformanceMonitorRequestBus.h
    Source/PerformanceMonitor/PerformanceMonitorSystemComponent.cpp
    Source/PerformanceMonitor/PerformanceMonitorSystemComponent.h

    Include/AtomToolsFramework/PreviewRenderer/PreviewContent.h
    Include/AtomToolsFramework/PreviewRenderer/PreviewerFeatureProcessorProviderBus.h
    Include/AtomToolsFramework/PreviewRenderer/PreviewRendererCaptureRequest.h
    Include/AtomToolsFramework/PreviewRenderer/PreviewRendererInterface.h
    Include/AtomToolsFramework/PreviewRenderer/PreviewRendererSystemRequestBus.h
    Source/PreviewRenderer/PreviewRenderer.cpp
    Source/PreviewRenderer/PreviewRenderer.h
    Source/PreviewRenderer/PreviewRendererCaptureState.cpp
    Source/PreviewRenderer/PreviewRendererCaptureState.h
    Source/PreviewRenderer/PreviewRendererIdleState.cpp
    Source/PreviewRenderer/PreviewRendererIdleState.h
    Source/PreviewRenderer/PreviewRendererLoadState.cpp
    Source/PreviewRenderer/PreviewRendererLoadState.h
    Source/PreviewRenderer/PreviewRendererState.h
    Source/PreviewRenderer/PreviewRendererSystemComponent.cpp
    Source/PreviewRenderer/PreviewRendererSystemComponent.h

    Include/AtomToolsFramework/SettingsDialog/SettingsDialog.h
    Source/SettingsDialog/SettingsDialog.cpp

    Include/AtomToolsFramework/Util/MaterialPropertyUtil.h
    Include/AtomToolsFramework/Util/Util.h
    Source/Util/MaterialPropertyUtil.cpp
    Source/Util/Util.cpp

    Include/AtomToolsFramework/Viewport/ModularViewportCameraController.h
    Include/AtomToolsFramework/Viewport/ModularViewportCameraControllerRequestBus.h
    Include/AtomToolsFramework/Viewport/RenderViewportWidget.h

    Include/AtomToolsFramework/Viewport/ViewportInputBehaviorController/DollyCameraBehavior.h
    Include/AtomToolsFramework/Viewport/ViewportInputBehaviorController/IdleBehavior.h
    Include/AtomToolsFramework/Viewport/ViewportInputBehaviorController/OrbitCameraBehavior.h
    Include/AtomToolsFramework/Viewport/ViewportInputBehaviorController/PanCameraBehavior.h
    Include/AtomToolsFramework/Viewport/ViewportInputBehaviorController/RotateCameraBehavior.h
    Include/AtomToolsFramework/Viewport/ViewportInputBehaviorController/RotateEnvironmentBehavior.h
    Include/AtomToolsFramework/Viewport/ViewportInputBehaviorController/RotateObjectBehavior.h
    Include/AtomToolsFramework/Viewport/ViewportInputBehaviorController/ViewportInputBehavior.h
    Include/AtomToolsFramework/Viewport/ViewportInputBehaviorController/ViewportInputBehaviorController.h
    Include/AtomToolsFramework/Viewport/ViewportInputBehaviorController/ViewportInputBehaviorControllerInterface.h
    Source/Viewport/ViewportInputBehaviorController/DollyCameraBehavior.cpp
    Source/Viewport/ViewportInputBehaviorController/IdleBehavior.cpp
    Source/Viewport/ViewportInputBehaviorController/OrbitCameraBehavior.cpp
    Source/Viewport/ViewportInputBehaviorController/PanCameraBehavior.cpp
    Source/Viewport/ViewportInputBehaviorController/RotateCameraBehavior.cpp
    Source/Viewport/ViewportInputBehaviorController/RotateEnvironmentBehavior.cpp
    Source/Viewport/ViewportInputBehaviorController/RotateObjectBehavior.cpp
    Source/Viewport/ViewportInputBehaviorController/ViewportInputBehavior.cpp
    Source/Viewport/ViewportInputBehaviorController/ViewportInputBehaviorController.cpp

    Include/AtomToolsFramework/Viewport/ViewportInteractionImpl.h
    Source/Viewport/ModularViewportCameraController.cpp
    Source/Viewport/RenderViewportWidget.cpp
    Source/Viewport/ViewportInteractionImpl.cpp

    Include/AtomToolsFramework/Window/AtomToolsMainWindow.h
    Include/AtomToolsFramework/Window/AtomToolsMainWindowRequestBus.h
    Source/Window/AtomToolsMainWindow.cpp
    Source/Window/AtomToolsMainWindowSystemComponent.cpp
    Source/Window/AtomToolsMainWindowSystemComponent.h
)