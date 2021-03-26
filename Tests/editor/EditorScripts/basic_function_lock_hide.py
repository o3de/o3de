"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

"""
C17218881: Basic Function: Locking & Hiding
https://testrail.agscollab.com/index.php?/cases/view/17218881
"""

import azlmbr.bus as bus
import azlmbr.math as math
import azlmbr.editor as editor
from PySide2 import QtWidgets
import Tests.ly_shared.pyside_utils as pyside_utils
import Tests.ly_shared.hydra_editor_utils as hydra
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class BasicFunctionLockHideTest(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="basic_function_lock_hide", args=["level"])

    def run_test(self):
        """
        Summary:
        Basic Function: Locking & Hiding

        Expected Behavior:
        1) Entities can be hidden.
        2) Hiding a parent entity hides its children.
        3) Entities can be shown.
        4) Showing a parent entity shows its children.
        5) Entities can be locked.
        6) Locking a parent entity locks its children.
        7) Locked entities cannot be interacted with in the Viewport.
        8) Entities can be unlocked.
        9) Unlocking a parent entity unlocks its children.
        10)Unlocked entities can be interacted with in the Viewport.
        11)Entities can be reordered even when locked or hidden.

        Test Steps:
        1) Open a new level
        2) Create an entity and 3 child entity
        3) Click on show/hide button to hide entities and verify if they are hidden
        4) Click on show/hide button to show entities and verify if they are hidden
        5) Click on lock/unlock entities to lock entities and verify if they are locked
        6) Click on lock/unlock entities to unlock entities and verify if they are unlocked
        7) Create 2 new entities
        8) Hide/Lock the newly created entities
        9) Move the newly created entites under parent entity


        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def is_hidden(id):
            return editor.EditorEntityInfoRequestBus(bus.Event, "IsHidden", id)

        def is_locked(id):
            return editor.EditorEntityInfoRequestBus(bus.Event, "IsLocked", id)

        # 1) Open a new level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )

        editor_window = pyside_utils.get_editor_main_window()
        outliner_widget = editor_window.findChild(QtWidgets.QDockWidget, "Entity Outliner (PREVIEW)").widget()

        # 2) Create an entity and 3 child entity
        entity_position = math.Vector3(200.0, 200.0, 38.0)
        component_to_add = ["Mesh"]
        parent = hydra.Entity("Entity2")
        parent.create_entity(entity_position, component_to_add)

        child_1 = hydra.Entity("Entity3")
        child_1.create_entity(entity_position, component_to_add, parent.id)

        child_2 = hydra.Entity("Entity4")
        child_2.create_entity(entity_position, component_to_add, parent.id)

        child_3 = hydra.Entity("Entity5")
        child_3.create_entity(entity_position, component_to_add, parent.id)

        # 3) Click on show/hide button to hide entities and verify if they are hidden
        tree = pyside_utils.find_child_by_hierarchy(outliner_widget, ..., "m_objectTree")
        self.wait_for_condition(lambda: pyside_utils.get_item_view_index(tree, 0).data() == "Entity2")
        parent_model_index = pyside_utils.find_child_by_pattern(tree, "Entity2")
        show_hide_parent = parent_model_index.siblingAtColumn(1)
        pyside_utils.item_view_index_mouse_click(tree, show_hide_parent)
        self.wait_for_condition(lambda: is_hidden(parent.id))
        entities_hidden = (
            is_hidden(parent.id) and is_hidden(child_1.id) and is_hidden(child_2.id) and is_hidden(child_3.id)
        )
        print(f"Child entities are hidden when parent entity is hidden: {entities_hidden}")

        # 4) Click on show/hide button to show entities and verify if they are hidden
        pyside_utils.item_view_index_mouse_click(tree, show_hide_parent)
        self.wait_for_condition(lambda: not is_hidden(parent.id))
        entities_hidden = (
            not is_hidden(parent.id)
            and not is_hidden(child_1.id)
            and not is_hidden(child_2.id)
            and not is_hidden(child_3.id)
        )
        print(f"Child entities are shown when parent entity is shown: {entities_hidden}")

        # 5) Click on lock/unlock entities to lock entities and verify if they are locked
        lock_unlock_parent = parent_model_index.siblingAtColumn(2)
        pyside_utils.item_view_index_mouse_click(tree, lock_unlock_parent)
        self.wait_for_condition(lambda: is_locked(parent.id))
        entities_locked = (
            is_locked(parent.id) and is_locked(child_1.id) and is_locked(child_2.id) and is_locked(child_3.id)
        )
        print(f"Child entities are locked when parent entity is locked: {entities_locked}")

        # 6) Click on lock/unlock entities to unlock entities and verify if they are unlocked
        pyside_utils.item_view_index_mouse_click(tree, lock_unlock_parent)
        self.wait_for_condition(lambda: not is_locked(parent.id))
        entities_unlocked = (
            not is_locked(parent.id)
            and not is_locked(child_1.id)
            and not is_locked(child_2.id)
            and not is_locked(child_3.id)
        )
        print(f"Child entities are unlocked when parent entity is unlocked: {entities_unlocked}")

        # 7) Create 2 new entities
        new_entity_1 = hydra.Entity("Entity6")
        new_entity_1.create_entity(entity_position, component_to_add)
        new_entity_2 = hydra.Entity("Entity7")
        new_entity_2.create_entity(entity_position, component_to_add)

        self.wait_for_condition(lambda: pyside_utils.get_item_view_index(tree, 0).data() == "Entity6")

        # 8) Hide/Lock the newly created entities
        new_entity_1_mi = pyside_utils.find_child_by_pattern(tree, "Entity6")
        show_hide_new_entity = new_entity_1_mi.siblingAtColumn(1)

        new_entity_2_mi = pyside_utils.find_child_by_pattern(tree, "Entity7")
        lock_unlock_new_entity = new_entity_2_mi.siblingAtColumn(2)

        pyside_utils.item_view_index_mouse_click(tree, show_hide_new_entity)
        pyside_utils.item_view_index_mouse_click(tree, lock_unlock_new_entity)

        # 9) Move the newly created entites under parent entity
        new_entity_1.set_test_parent_entity(parent)
        new_entity_2.set_test_parent_entity(parent)


test = BasicFunctionLockHideTest()
test.run()
