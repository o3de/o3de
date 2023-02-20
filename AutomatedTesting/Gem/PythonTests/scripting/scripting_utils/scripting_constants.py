"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

We are deprecating this file and moving the constants to area specific files. See the EditorPythonTestTools > consts
folder for more info
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
Constants for Asset Editor
"""
SAVE_ASSET_AS = "SaveAssetAs"
DEFAULT_SCRIPT_EVENT = "EventName"
DEFAULT_METHOD_NAME = "MethodName"
PARAMETER_NAME = "ParameterName"

"""
Constants for QtWidgets.
Different from window/tab names because they do not have spaces
"""
NODE_PALETTE_QT = "NodePalette"
NODE_INSPECTOR_QT = "NodeInspector"
TREE_VIEW_QT = "treeView"
EVENTS_QT = "Events"
EVENT_NAME_QT = "EventName"
VARIABLE_PALETTE_QT = "variablePalette"
VARIABLE_MANAGER_QT = "VariableManager"
GRAPH_VARIABLES_QT = "graphVariables"
ADD_BUTTON_QT = "addButton"
SEARCH_FRAME_QT ="searchFrame"
SEARCH_FILTER_QT = "searchFilter"
PARAMETERS_QT = "Parameters"

"""
General constants
"""
BASE_LEVEL_NAME = "Base"
SAVE_STRING = "Save"
NAME_STRING = "Name"
WAIT_FRAMES = 200
WAIT_TIME_1 = 1
WAIT_TIME_3 = 3
WAIT_TIME_5 = 5

VARIABLE_TYPES = ["Boolean", "Color", "EntityId", "Number", "String", "Transform", "Vector2", "Vector3", "Vector4"]

VARIABLE_TYPES_DICT = {
    "Boolean": "Boolean",
    "Color": "Color",
    "EntityId": "EntityId",
    "Number": "Number",
    "String": "String",
    "Transform": "Transform",
    "Vector2": "Vector2",
    "Vector3": "Vector3",
    "Vector4": "Vector4"
    }

ENTITY_STATES = {
    "active": 0,
    "inactive": 1,
    "editor": 2,
    }

"""
File Paths
"""
SCRIPT_EVENT_FILE_PATH = os.path.join(paths.projectroot, "ScriptCanvas", "test_file.scriptevent")
SCRIPT_CANVAS_COMPONENT_PROPERTY_PATH = "Configuration|Source"
