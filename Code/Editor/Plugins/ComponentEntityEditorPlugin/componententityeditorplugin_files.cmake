#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    dllmain.cpp
    ComponentEntityEditorPlugin.h
    ComponentEntityEditorPlugin.cpp
    ContextMenuHandlers.h
    ContextMenuHandlers.cpp
    SandboxIntegration.h
    SandboxIntegration.cpp
    UI/QComponentEntityEditorMainWindow.h
    UI/QComponentEntityEditorMainWindow.cpp
    UI/QComponentLevelEntityEditorMainWindow.h
    UI/QComponentLevelEntityEditorMainWindow.cpp
    UI/QComponentEntityEditorOutlinerWindow.h
    UI/QComponentEntityEditorOutlinerWindow.cpp
    UI/AssetCatalogModel.h
    UI/AssetCatalogModel.cpp
    UI/ComponentPalette/ComponentPaletteSettings.h
    UI/Outliner/OutlinerDisplayOptionsMenu.h
    UI/Outliner/OutlinerDisplayOptionsMenu.cpp
    UI/Outliner/OutlinerTreeView.hxx
    UI/Outliner/OutlinerTreeView.cpp
    UI/Outliner/OutlinerWidget.hxx
    UI/Outliner/OutlinerWidget.cpp
    UI/Outliner/OutlinerCacheBus.h
    UI/Outliner/OutlinerListModel.hxx
    UI/Outliner/OutlinerListModel.cpp
    UI/Outliner/OutlinerSearchWidget.h
    UI/Outliner/OutlinerSearchWidget.cpp
    UI/Outliner/OutlinerSortFilterProxyModel.hxx
    UI/Outliner/OutlinerSortFilterProxyModel.cpp
    UI/Outliner/OutlinerWidget.ui
    UI/Outliner/resources.qrc
    UI/Outliner/EntityOutliner.qss
)
