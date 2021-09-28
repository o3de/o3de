"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Test case ID : C4976236
Test Case Title : Verify that you can add the physX collider component to an entity
                  without it throwing an error or warning

"""

# fmt: off
class Tests():
    create_test_entity           = ("Entity created successfully",    "Failed to create Entity")
    add_physx_collider           = ("PhysX Collider component added", "Failed to add PhysX Collider component")
    enter_game_mode              = ("Entered game mode",              "Failed to enter game mode")
    no_errors_and_warnings_found = ("No errors and warnings found",   "Found errors and warnings")
    exit_game_mode               = ("Exited game mode",               "Failed to exit game mode")
# fmt: on


def Collider_AddColliderComponent():
    """
    Summary:
    Opens an empty level and creates an Entity with PhysX Collider. Verify that editor remains stable in Game mode.

    Expected Behavior:
    The Editor is stable there are no warnings or errors.

    Test Steps:
     1) Load the level
     2) Create test entity
     3) Start the Tracer to catch any errors and warnings
     4) Add the PhysX Collider component and change shape to box
     5) Enter game mode
     6) Verify there are no errors and warnings in the logs
     7) Exit game mode
     8) Close the editor

    :return: None
    """

    # Helper file Imports

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.utils import Tracer
    from editor_python_test_tools.asset_utils import Asset
    
    helper.init_idle()
    # 1) Load the level
    helper.open_level("Physics", "Base")

    # 2) Create test entity
    test_entity = EditorEntity.create_editor_entity("TestEntity")
    Report.result(Tests.create_test_entity, test_entity.id.IsValid())

    # 3) Start the Tracer to catch any errors and warnings
    with Tracer() as section_tracer:
        # 4) Add the PhysX Collider component and change shape to box
        collider_component = test_entity.add_component("PhysX Collider")
        Report.result(Tests.add_physx_collider, test_entity.has_component("PhysX Collider"))
        collider_component.set_component_property_value('Shape Configuration|Shape', azlmbr.physics.ShapeType_Box)

        # 5) Enter game mode
        helper.enter_game_mode(Tests.enter_game_mode)

    # 6) Verify there are no errors and warnings in the logs
    success_condition = not (section_tracer.has_errors or section_tracer.has_warnings)
    Report.result(Tests.no_errors_and_warnings_found, success_condition)
    if not success_condition:
        if section_tracer.has_warnings:
            Report.info(f"Warnings found: {section_tracer.warnings}")
        if section_tracer.has_errors:
            Report.info(f"Errors found: {section_tracer.errors}")

    # 7) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Collider_AddColliderComponent)
