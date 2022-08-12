"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import azlmbr

from editor_python_test_tools.editor_entity_utils import EditorEntity
from editor_python_test_tools.utils import TestHelper as helper
from editor_python_test_tools.utils import Report

from .physics_constants import WAIT_TIME_1

def create_validated_entity(name, test_message):
    entity = EditorEntity.create_editor_entity(name)
    Report.critical_result(test_message, helper.wait_for_condition(lambda: entity.id.IsValid(), WAIT_TIME_1))

    return entity

def add_validated_component(parent_entity, component, test_message):
    created_component = parent_entity.add_component(component)
    Report.critical_result(test_message, helper.wait_for_condition(lambda: parent_entity.has_component(component), WAIT_TIME_1))

    return created_component