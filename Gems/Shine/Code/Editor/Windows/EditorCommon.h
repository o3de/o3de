/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QString>
#include <type_traits>
#include <QKeySequence>

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

// IMPORTANT: This is NOT the permanent location for these values.
#define AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME "O3DE"
#define AZ_QCOREAPPLICATION_SETTINGS_APPLICATION_NAME "O3DE"

// See: http://en.wikipedia.org/wiki/Internet_media_type#Prefix_x
#define UICANVASEDITOR_MIMETYPE "application/x-amazon-o3de-uicanvaseditor"

bool ClipboardContainsOurDataType();

#define UICANVASEDITOR_NAME_SHORT   "UiCanvasEditor"

#define UICANVASEDITOR_COORDINATE_SYSTEM_CYCLE_SHORTCUT_KEY_SEQUENCE    QKeySequence(0x0 | Qt::CTRL | Qt::Key_W)
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
