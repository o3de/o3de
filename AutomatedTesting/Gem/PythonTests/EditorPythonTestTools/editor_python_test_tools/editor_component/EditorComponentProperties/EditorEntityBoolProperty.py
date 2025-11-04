"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

This util is intended to store common test values for editor component tests.
"""
from editor_python_test_tools.editor_component.EditorComponentProperties.EditorEntityComponentProperty import EditorEntityComponentProperty
from editor_python_test_tools.editor_entity_utils import EditorComponent


class EditorEntityBoolProperty(EditorEntityComponentProperty):
    """
    Defines the behaviors for interacting with an Editor Entity boolean property
    """
    def __init__(self, property_path: str, editor_component: EditorComponent):
        super().__init__(property_path, editor_component)

    def get(self) -> bool:
        return self.editor_component.get_component_property_value(self.property_path)

    def set(self, value: bool):
        self.editor_component.set_component_property_value(self.property_path, value)

