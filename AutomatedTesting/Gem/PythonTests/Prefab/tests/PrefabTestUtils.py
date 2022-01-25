"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os

from azlmbr.entity import EntityId
from azlmbr.math import Vector3
from editor_python_test_tools.editor_entity_utils import EditorEntity
from editor_python_test_tools.utils import Report
from editor_python_test_tools.utils import TestHelper as helper

import azlmbr.bus as bus
import azlmbr.components as components
import azlmbr.entity as entity
import azlmbr.legacy.general as general

def check_entity_children_count(entity_id, expected_children_count):
    entity_children_count_matched_result = (
        "Entity with a unique name found",
        "Entity with a unique name *not* found")

    entity = EditorEntity(entity_id)
    children_entity_ids = entity.get_children_ids()
    entity_children_count_matched = len(children_entity_ids) == expected_children_count
    Report.result(entity_children_count_matched_result, entity_children_count_matched)

    if not entity_children_count_matched:
        Report.info(f"Entity '{entity_id.ToString()}' actual children count: {len(children_entity_ids)}. Expected children count: {expected_children_count}")

    return entity_children_count_matched

def open_base_tests_level():
    helper.init_idle()
    helper.open_level("Prefab", "Base")
