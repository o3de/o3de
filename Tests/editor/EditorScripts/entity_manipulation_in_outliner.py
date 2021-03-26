"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

C6384955: Basic Workflow: Entity Manipulation in the Outliner
https://testrail.agscollab.com/index.php?/cases/view/6384955
"""

import os
import sys
from typing import Optional

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.bus as bus
import azlmbr.entity as entity
import azlmbr.editor as editor
import azlmbr.math as math
import azlmbr.legacy.general as general

import Tests.ly_shared.pyside_utils as pyside_utils
import Tests.ly_shared.hydra_editor_utils as editor_utils
from PySide2 import QtWidgets
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class BasicWorkflow(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="entity_manipulation_in_outliner", args=["level"])

    def run_test(self):
        """
        Summary:
        Manipulate entities in the Entity Outliner

        Expected:
        Entities/Children can be created/restructured

        Test Steps:
        0) Open new test level
        1) Create the Entities/Child Entities
            1.1) Create Parent Entity
            1.2) Create Child Entity
            1.3) Create Grandchild Entity
            1.4) Entities/Children can be created in Outliner and are displayed properly
        2) Move the grandchild entity (child of the child entity) to be a child of the Parent (top level) entity
        3) Verify Entity Heirarchies can be restructured from the Entity Outliner
        4) Undo Entity Restructure (Ctrl+Z)
        5) Verify Entity Hierarchy Restructure is undone

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def verify_parent(entity_to_check, expected_parent_entity, additional_pretext: Optional[str] = ""):
            entity_to_check.get_parent_info()
            actual_parent_id = entity_to_check.parent_id
            actual_parent_name = entity_to_check.parent_name

            # fmt:off
            print(f"{additional_pretext} The parent entity of {entity_to_check.name}: Expected: {expected_parent_entity.name}; Actual: {actual_parent_name}")
            assert actual_parent_id == expected_parent_entity.id, \
                "The IDs of the actual and expected parent entities did not match"
            # fmt:on

        # 0) Create or open any level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )
        general.idle_wait(3.0)

        # 1) Create the Entities/Child Entities
        entity_postition = math.Vector3(125.0, 136.0, 32.0)
        components_to_add = []

        # 1.1) Create an entity called "Parent"
        parent_entity = editor_utils.Entity("Parent")
        parent_entity.create_entity(entity_postition, components_to_add)

        # 1.2) Create a child entity for "Parent" named "Child"
        child_entity = editor_utils.Entity("Child")
        child_entity.create_entity(entity_postition, components_to_add, parent_entity.id)

        # 1.3) Create a child entity for "Child" named "Grandchild"
        grandchild_entity = editor_utils.Entity("Grandchild")
        grandchild_entity.create_entity(entity_postition, components_to_add, child_entity.id)

        # 1.4) Verify the hierarchy of the entities (Parent>Child>Grandchild)
        # Child should be a child to Parent
        verify_parent(child_entity, parent_entity, "Original Check:")
        # Grandchild should be a child to Child
        verify_parent(grandchild_entity, child_entity, "Original Check:")

        # 2) Click and drag the Grandchild from the Child entity to the Parent entity
        grandchild_entity.set_test_parent_entity(parent_entity)

        # 3) Verify the Granchild entity is now a child of the Parent Entity
        verify_parent(grandchild_entity, parent_entity, "After Move:")

        # 4) Press Ctrl+Z using hydra utilities to undo the moving of the Grandchild
        general.undo()

        # 5) Verify the hierarchy has returned to the original orientation
        verify_parent(grandchild_entity, child_entity, "After Undo:")


test = BasicWorkflow()
test.run()
