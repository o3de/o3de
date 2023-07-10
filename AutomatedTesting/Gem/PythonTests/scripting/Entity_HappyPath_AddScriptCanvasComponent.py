"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# fmt: off
class Tests():
    level_created     = ("New level created",                       "Failed to create new level")
    entity_created    = ("Test Entity created",                     "Failed to create test entity")
    add_sc_component  = ("Script Canvas component added to entity", "Failed to add SC component to entity")
    no_errors_found   = ("Tracer found no errors",                  "One or more errors found by Tracer")
    no_warnings_found = ("Tracer found no warnings",                "One or more warnings found by Tracer")
# fmt: on


def Entity_HappyPath_AddScriptCanvasComponent():
    """
    Summary:
     Script Canvas Component can be added to an entity

    Expected Behavior:
     Script Canvas Component is added to the entity successfully without issue

    Test Steps:
     1) Open the base level
     2) Create test entity
     3) Start Tracer
     4) Add Script Canvas component to test entity and check for errors


    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    from utils import TestHelper as helper
    from utils import Tracer
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    import azlmbr.legacy.general as general
    import azlmbr.math as math
    from scripting_utils.scripting_constants import (SCRIPT_CANVAS_UI)

    TEST_ENTITY_NAME = "test_entity"

    general.idle_enable(True)

    # 1) Open the base level
    helper.open_level("", "Base")
    general.close_pane("Error Report")

    # 2) Create new entity
    position = math.Vector3(512.0, 512.0, 32.0)
    editor_entity = EditorEntity.create_editor_entity_at(position, TEST_ENTITY_NAME)
    Report.result(Tests.entity_created, editor_entity.id.IsValid())

    # 3) Start Tracer
    with Tracer() as section_tracer:

        # 4) Add Script Canvas component to test entity and check for errors
        editor_entity.add_component(SCRIPT_CANVAS_UI)
        Report.result(Tests.add_sc_component, editor_entity.has_component(SCRIPT_CANVAS_UI))


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from utils import Report

    Report.start_test(Entity_HappyPath_AddScriptCanvasComponent)
