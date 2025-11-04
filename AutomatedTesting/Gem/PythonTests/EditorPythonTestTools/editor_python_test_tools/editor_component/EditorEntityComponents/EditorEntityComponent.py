"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

This util is intended to store common test values for editor component tests.
"""

from editor_python_test_tools.editor_entity_utils import EditorEntity


class EditorEntityComponent:
    """
    Base class that defines the standard behavior shared between Editor Entity Components
    """
    def __init__(self, editor_entity: EditorEntity, component_name: str) -> None:
        self.component = editor_entity.add_component(component_name)

    def is_enabled(self) -> bool:
        return self.component.is_enabled()

    def enable(self) -> None:
        self.component.set_enable(True)

    def disable(self) -> None:
        self.component.set_enable(False)

    def set_enabled(self, enabled: bool) -> None:
        self.component.set_enabled(enabled)
