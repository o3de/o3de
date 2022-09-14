"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Holds constants used across both hydra and non-hydra scripts.
"""


"""
General constants
"""
BASE_LEVEL_NAME = "Base"
SAVE_STRING = "Save"
NAME_STRING = "Name"
WAIT_FRAMES = 200
WAIT_TIME_3 = 3
WAIT_TIME_5 = 5
EVENTS_QT = "Events"
EVENT_NAME_QT = "EventName"

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
