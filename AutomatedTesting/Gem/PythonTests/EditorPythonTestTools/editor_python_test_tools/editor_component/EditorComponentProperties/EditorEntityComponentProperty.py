"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

This util is intended to store common test values for editor component tests.
"""
from editor_python_test_tools.editor_entity_utils import EditorComponent


class EditorEntityComponentProperty:

    def __init__(self, editor_component: EditorComponent, path: str):
        self.property_path = path
        self.editor_component = editor_component
