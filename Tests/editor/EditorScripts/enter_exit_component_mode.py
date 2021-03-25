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
C2174442: Enter/Exit Component Mode
https://testrail.agscollab.com/index.php?/cases/view/2174442
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import azlmbr.entity as EntityId
import Tests.ly_shared.hydra_editor_utils as hydra
import azlmbr.math as math
import azlmbr.bus as bus
import azlmbr.editor as editor
from Tests.editor.editor_utils.editor_test_helper import EditorTestHelper


class TestEnterExitComponentMode(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="enter_exit_component_mode: ", args=["level"])

    def run_test(self):
        """
        Summary:
        Enter/Exit Component Mode

        Expected Behavior:
        Component Mode can be entered and exited.

        Test Steps:
         1) Open level
         2) Create an entity with Box Shape component
         3) Click the "Edit" button on the component to enter into component mode
         4) Click the "Done" button on the component to exit from component mode

        Note:
        - This test file must be called from the Lumberyard Editor command terminal
        - Any passed and failed tests are written to the Editor.log file.
                Parsing the file or running a log_monitor are required to observe the test results.

        :return: None
        """

        def on_entered_component_mode(parameters):
            print("Entered component mode")

        def on_left_component_mode(parameters):
            print("Left component mode")

        # 1) Open level
        self.test_success = self.create_level(
            self.args["level"],
            heightmap_resolution=1024,
            heightmap_meters_per_pixel=1,
            terrain_texture_resolution=4096,
            use_terrain=True,
        )
        general.idle_wait(3.0)

        # Setup the handler to listen for Editor Component Mode notifications
        context_id = editor.EditorEntityContextRequestBus(bus.Broadcast, "GetEditorEntityContextId")
        handler = bus.NotificationHandler("EditorComponentModeNotificationBus")
        handler.connect(context_id)
        handler.add_callback("EnteredComponentMode", on_entered_component_mode)
        handler.add_callback("LeftComponentMode", on_left_component_mode)

        # 2) Create an entity with Box Shape component
        entity_position = math.Vector3(125.0, 136.0, 32.0)
        entity_id = editor.ToolsApplicationRequestBus(
            bus.Broadcast, "CreateNewEntityAtPosition", entity_position, EntityId.EntityId()
        )
        general.clear_selection()
        general.select_object(editor.EditorEntityInfoRequestBus(bus.Event, "GetName", entity_id))
        hydra.add_component("Box Shape", entity_id)

        # 3) Click the "Edit" button on the component to enter into component mode
        # Retrieve the TypeId for the Box Shape component
        type_ids = editor.EditorComponentAPIBus(bus.Broadcast, "FindComponentTypeIdsByEntityType", ["Box Shape"],
                                                EntityId.EntityType().Game)
        box_shape_type_id = type_ids[0]

        # Enter component mode for the Box Shape (Same as pressing the "Edit" button on the Box Shape component)
        editor.ComponentModeSystemRequestBus(bus.Broadcast, "EnterComponentMode", box_shape_type_id)

        # 4) Click the "Done" button on the component to exit from component mode
        # End component mode (Same as pressing the "Done" button on the Box Shape component)
        editor.ComponentModeSystemRequestBus(bus.Broadcast, "EndComponentMode")


test = TestEnterExitComponentMode()
test.run()
