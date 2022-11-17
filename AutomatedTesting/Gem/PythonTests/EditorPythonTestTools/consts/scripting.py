"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Holds script canvas related constants
"""
import os
import azlmbr.paths as paths

"""
Constants for window and tab names

"""
SCRIPT_CANVAS_UI = "Script Canvas"
NODE_PALETTE_UI = "Node Palette"
NODE_INSPECTOR_UI = "Node Inspector"
ASSET_EDITOR_UI = "Asset Editor"
SCRIPT_EVENT_UI = "Script Events"
VARIABLE_MANAGER_UI = "Variable Manager"

"""
Constants for Variable Manager
"""
RESTORE_DEFAULT_LAYOUT = "Restore Default Layout"


"""
String constants for Node Palette
"""
NODE_CATEGORY_MATH = "Math"
NODE_STRING_TO_NUMBER = "String To Number"
NODE_TEST_METHOD = "test_method_name"

"""
Constants for Node Inspector
"""
NODE_INSPECTOR_TITLE_KEY = "Title"

"""
Constants for QtWidgets.
Different from window/tab names because they do not have spaces
"""
NODE_PALETTE_QT = "NodePalette"
NODE_INSPECTOR_QT = "NodeInspector"
PROPERTY_EDITOR_QT = "PropertyEditor"
INITIAL_VALUE_SOURCE_QT = "Initial Value Source"
TREE_VIEW_QT = "treeView"
VARIABLE_PALETTE_QT = "variablePalette"
VARIABLE_MANAGER_QT = "VariableManager"
GRAPH_VARIABLES_QT = "graphVariables"
GRAPH_VARIABLES_PAGE_QT = "graphVariablesPage"
ADD_BUTTON_QT = "addButton"
SEARCH_FRAME_QT ="searchFrame"
SEARCH_FILTER_QT = "searchFilter"
PARAMETERS_QT = "Parameters"
NODE_PALETTE_CLEAR_BUTTON_QT = "ClearToolButton"

"""
File Paths
"""
SCRIPT_EVENT_FILE_PATH = os.path.join(paths.projectroot, "ScriptCanvas", "test_file.scriptevent")
SCRIPT_CANVAS_TEST_FILE_PATH = os.path.join(paths.projectroot, "ScriptCanvas", "test_file.scriptcanvas")
SCRIPT_CANVAS_COMPONENT_PROPERTY_PATH = "Configuration|Source"
