"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

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
     1) Create temp level
     2) Create test entity
     3) Start Tracer
     4) Add Script Canvas component to test entity
     5) Search for errors and warnings


    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    from utils import TestHelper as helper
    from utils import Tracer
    from editor_entity_utils import EditorEntity
    import azlmbr.legacy.general as general

    LEVEL_NAME = "tmp_level"
    WAIT_TIME = 3.0  # SECONDS

    # 1) Create temp level
    general.idle_enable(True)
    result = general.create_level_no_prompt(LEVEL_NAME, 128, 1, 512, True)
    Report.critical_result(Tests.level_created, result == 0)
    helper.wait_for_condition(lambda: general.get_current_level_name() == LEVEL_NAME, WAIT_TIME)
    general.close_pane("Error Report")

    # 2) Create new entity
    test_entity = EditorEntity.create_editor_entity("test_entity")
    Report.result(Tests.entity_created, test_entity.id.IsValid())

    # 3) Start Tracer
    with Tracer() as section_tracer:

        # 4) Add Script Canvas component to test entity
        test_entity.add_component("Script Canvas")
        Report.result(Tests.add_sc_component, test_entity.has_component("Script Canvas"))

    # 5) Search for errors and warnings
    Report.result(Tests.no_errors_found, not section_tracer.has_errors)
    Report.result(Tests.no_warnings_found, not section_tracer.has_warnings)


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from utils import Report

    Report.start_test(Entity_HappyPath_AddScriptCanvasComponent)
