"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Holds asset editor related constants
"""
import os
import azlmbr.paths as paths

"""
Constants for Asset Editor
"""
SAVE_ASSET_AS = "SaveAssetAs"
DEFAULT_SCRIPT_EVENT = "EventName"
DEFAULT_METHOD_NAME = "MethodName"
PARAMETER_NAME = "ParameterName"
EVENTS_QT = "Events"
EVENT_NAME_QT = "EventName"

"""
specific path constant. Move this file path to the associated test 
"""
SCRIPT_EVENT_FILE_PATH = os.path.join(paths.projectroot, "ScriptCanvas", "test_file.scriptevent")

