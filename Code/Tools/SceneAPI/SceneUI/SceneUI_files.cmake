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
    SceneUIConfiguration.h
    SceneUI.qrc
    ManifestMetaInfoHandler.h
    ManifestMetaInfoHandler.cpp
    GraphMetaInfoHandler.h
    GraphMetaInfoHandler.cpp
    SceneUIStandaloneAllocator.h
    SceneUIStandaloneAllocator.cpp
    CommonWidgets/OverlayWidget.h
    CommonWidgets/OverlayWidgetLayer.h
    CommonWidgets/JobWatcher.h
    CommonWidgets/JobWatcher.cpp
    CommonWidgets/ProcessingOverlayWidget.h
    CommonWidgets/ProcessingOverlayWidget.cpp
    CommonWidgets/ProcessingOverlayWidget.ui
    CommonWidgets/ExpandCollapseToggler.h
    CommonWidgets/ExpandCollapseToggler.cpp
    Handlers/ProcessingHandlers/ProcessingHandler.h
    Handlers/ProcessingHandlers/ProcessingHandler.cpp
    Handlers/ProcessingHandlers/ExportJobProcessingHandler.h
    Handlers/ProcessingHandlers/ExportJobProcessingHandler.cpp
    Handlers/ProcessingHandlers/AsyncOperationProcessingHandler.h
    Handlers/ProcessingHandlers/AsyncOperationProcessingHandler.cpp
    SceneWidgets/ManifestWidget.h
    SceneWidgets/ManifestWidget.cpp
    SceneWidgets/ManifestWidget.ui
    SceneWidgets/ManifestWidgetPage.h
    SceneWidgets/ManifestWidgetPage.cpp
    SceneWidgets/ManifestWidgetPage.ui
    SceneWidgets/SceneGraphWidget.h
    SceneWidgets/SceneGraphWidget.cpp
    SceneWidgets/SceneGraphWidget.ui
    SceneWidgets/SceneGraphInspectWidget.h
    SceneWidgets/SceneGraphInspectWidget.cpp
    SceneWidgets/SceneGraphInspectWidget.ui
    RowWidgets/HeaderWidget.h
    RowWidgets/HeaderWidget.cpp
    RowWidgets/HeaderWidget.ui
    RowWidgets/HeaderHandler.h
    RowWidgets/HeaderHandler.cpp
    RowWidgets/ManifestVectorHandler.h
    RowWidgets/ManifestVectorHandler.cpp
    RowWidgets/ManifestVectorWidget.ui
    RowWidgets/ManifestVectorWidget.h
    RowWidgets/ManifestVectorWidget.cpp
    RowWidgets/NodeListSelectionHandler.h
    RowWidgets/NodeListSelectionHandler.cpp
    RowWidgets/NodeListSelectionWidget.h
    RowWidgets/NodeListSelectionWidget.cpp
    RowWidgets/NodeTreeSelectionHandler.h
    RowWidgets/NodeTreeSelectionHandler.cpp
    RowWidgets/NodeTreeSelectionWidget.h
    RowWidgets/NodeTreeSelectionWidget.cpp
    RowWidgets/NodeTreeSelectionWidget.ui
    RowWidgets/ManifestNameHandler.h
    RowWidgets/ManifestNameHandler.cpp
    RowWidgets/ManifestNameWidget.h
    RowWidgets/ManifestNameWidget.cpp
    RowWidgets/TransformRowHandler.h
    RowWidgets/TransformRowHandler.cpp
    RowWidgets/TransformRowWidget.h
    RowWidgets/TransformRowWidget.cpp
    DllMain.cpp
)
