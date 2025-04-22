/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Cry_Geo.h>
#include <Include/IPlugin.h>
#include <QtWidgets/QMainWindow>
#include <LyShine/IDraw2d.h>
#include <LyShine/ILyShine.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiEditorBus.h>
#include <LyShine/Bus/UiLayoutBus.h>
#include <LyShine/Bus/UiTransform2dBus.h>
#include <LyShine/Bus/UiVisualBus.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Vector3.h>

#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/Undo/UndoSystem.h>

class CanvasSizeToolbarSection;
class CommandCanvasPropertiesChange;
class CommandCanvasSizeToolbarIndex;
class CommandHierarchyItemCreate;
class CommandHierarchyItemCreateFromData;
class CommandHierarchyItemDelete;
class CommandHierarchyItemRename;
class CommandHierarchyItemReparent;
class CommandHierarchyItemToggleIsExpanded;
class CommandHierarchyItemToggleIsSelectable;
class CommandHierarchyItemToggleIsVisible;
class CommandPropertiesChange;
class ComponentButton;
class CoordinateSystemToolbarSection;
class EditorMenu;
class EditorWindow;
class EnterPreviewToolbar;
class HierarchyClipboard;
class HierarchyHeader;
class HierarchyItem;
class HierarchyMenu;
class HierarchyWidget;
class MainToolbar;
class ModeToolbar;
class NewElementToolbarSection;
class PreviewActionLog;
class PreviewAnimationList;
class PreviewToolbar;
class PropertiesContainer;
class PropertiesWidget;
class PropertiesWrapper;
class UndoStack;
class UndoStackExecutionScope;
class ViewportAnchor;
class ViewportCanvasBackground;
class ViewportElement;
class ViewportHighlight;
class ViewportIcon;
class ViewportInteraction;
class ViewportNudge;
class ViewportPivot;
class ViewportSnap;
class ViewportWidget;

QT_FORWARD_DECLARE_CLASS(QTreeWidgetItem)

using HierarchyItemRawPtrList = std::list< HierarchyItem* >;
using QTreeWidgetItemRawPtrList = std::list< QTreeWidgetItem* >;
using QTreeWidgetItemRawPtrQList = QList< QTreeWidgetItem* >;

// This allows iterating over an enum class.
#define ADD_ENUM_CLASS_ITERATION_OPERATORS(CLASS_NAME, FIRST_VALUE, LAST_VALUE)                                             \
                                                                                                                            \
    inline CLASS_NAME operator++(CLASS_NAME & m){ return m = (CLASS_NAME)(std::underlying_type<CLASS_NAME>::type(m) + 1); } \
    inline CLASS_NAME operator*(CLASS_NAME m){ return m; }                                                                  \
    inline CLASS_NAME begin([[maybe_unused]] CLASS_NAME m){ return FIRST_VALUE; }                                                            \
    inline CLASS_NAME end([[maybe_unused]] CLASS_NAME m){ return (CLASS_NAME)(std::underlying_type<CLASS_NAME>::type(LAST_VALUE) + 1); }

enum class UiEditorMode
{
    Edit, Preview
};

enum class FusibleCommand
{
    kViewportInteractionMode,
    kCanvasSizeToolbarIndex
};

#include "ViewportHelpers.h"
#include "EntityHelpers.h"
#include "SerializeHelpers.h"
#include "FileHelpers.h"
#include "ComponentHelpers.h"
#include "HierarchyHelpers.h"
#include "UiSliceManager.h"
#include "SelectionHelpers.h"
#include "ViewportInteraction.h"

#include "CanvasSizeToolbarSection.h"
#include "CommandCanvasPropertiesChange.h"
#include "CommandCanvasSizeToolbarIndex.h"
#include "CommandHierarchyItemCreate.h"
#include "CommandHierarchyItemCreateFromData.h"
#include "CommandHierarchyItemDelete.h"
#include "CommandHierarchyItemRename.h"
#include "CommandHierarchyItemReparent.h"
#include "CommandHierarchyItemToggleIsExpanded.h"
#include "CommandHierarchyItemToggleIsSelectable.h"
#include "CommandHierarchyItemToggleIsVisible.h"
#include "CommandPropertiesChange.h"
#include "ComponentButton.h"
#include "CoordinateSystemToolbarSection.h"
#include "EditorWindow.h"
#include "EnterPreviewToolbar.h"
#include "HierarchyClipboard.h"
#include "HierarchyHeader.h"
#include "HierarchyItem.h"
#include "HierarchyMenu.h"
#include "HierarchyWidget.h"
#include "MainToolbar.h"
#include "ModeToolbar.h"
#include "NewElementToolbarSection.h"
#include "PreviewActionLog.h"
#include "PreviewAnimationList.h"
#include "PreviewToolbar.h"
#include "PropertiesContainer.h"
#include "PropertiesWidget.h"
#include "PropertiesWrapper.h"
#include "PropertyHandlers.h"
#include "RecentFiles.h"
#include "UndoStack.h"
#include "UndoStackExecutionScope.h"
#include "ViewportAnchor.h"
#include "ViewportCanvasBackground.h"
#include "ViewportHighlight.h"
#include "ViewportIcon.h"
#include "ViewportWidget.h"

// IMPORTANT: This is NOT the permanent location for these values.
#define AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME "O3DE"
#define AZ_QCOREAPPLICATION_SETTINGS_APPLICATION_NAME "O3DE"

// See: http://en.wikipedia.org/wiki/Internet_media_type#Prefix_x
#define UICANVASEDITOR_MIMETYPE "application/x-amazon-o3de-uicanvaseditor"

bool ClipboardContainsOurDataType();

#define UICANVASEDITOR_NAME_SHORT   "UiCanvasEditor"

#define UICANVASEDITOR_COORDINATE_SYSTEM_CYCLE_SHORTCUT_KEY_SEQUENCE    QKeySequence(Qt::CTRL + Qt::Key_W)
#define UICANVASEDITOR_SNAP_TO_GRID_TOGGLE_SHORTCUT_KEY_SEQUENCE        QKeySequence(Qt::Key_G)

#define UICANVASEDITOR_CANVAS_DIRECTORY "UI/Canvases"
#define UICANVASEDITOR_CANVAS_EXTENSION "uicanvas"

#define UICANVASEDITOR_QMENU_ITEM_DISABLED_STYLESHEET   "QMenu::item:disabled { color: rgb(127, 127, 127); }"

enum HierarchyColumn
{
    kHierarchyColumnName,
    kHierarchyColumnIsVisible,
    kHierarchyColumnIsSelectable,
    kHierarchyColumnCount
};

#define UICANVASEDITOR_HIERARCHY_HEADER_ICON_SIZE       (16)

// Stores startup (original) location of localization folder
#define UICANVASEDITOR_SETTINGS_STARTUP_LOC_FOLDER_KEY  (QString("StartupLocFolder") + " " + FileHelpers::GetAbsoluteGameDir())
