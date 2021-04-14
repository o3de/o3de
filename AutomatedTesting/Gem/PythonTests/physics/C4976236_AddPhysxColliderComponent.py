"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Test case ID : C4976236
Test Case Title : Verify that you can add the physX collider component to an entity
                  without it throwing an error or warning
URL of the test case : https://testrail.agscollab.com/index.php?/cases/view/4976236
"""

# fmt: off
class Tests():
    create_test_entity           = ("Entity created successfully",    "Failed to create Entity")
    add_physx_collider           = ("PhysX Collider component added", "Failed to add PhysX Collider component")
    enter_game_mode              = ("Entered game mode",              "Failed to enter game mode")
    no_errors_and_warnings_found = ("No errors and warnings found",   "Found errors and warnings")
    exit_game_mode               = ("Exited game mode",               "Failed to exit game mode")
# fmt: on


def C4976236_AddPhysxColliderComponent():
    """
    Summary:
    Load level with Entity having PhysX Collider component. Verify that editor remains stable in Game mode.

    Expected Behavior:
    The Editor is stable there are no warnings or errors.

    Test Steps:
     1) Load the level
     2) Create test entity
     3) Start the Tracer to catch any errors and warnings
     4) Add the PhysX Collider component and change shape to box
     5) Add Mesh component and an asset
     6) Enter game mode
     7) Verify there are no errors and warnings in the logs
     8) Exit game mode
     9) Close the editor

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    # Helper file Imports
    import ImportPathHelper as imports

    imports.init()
    from utils import Report
    from utils import TestHelper as helper
    from utils import Tracer
    from editor_entity_utils import EditorEntity
    from asset_utils import Asset

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
        collider_component.set_component_property_value('Shape Configuration|Shape', 1)

        # 5) Add Mesh component and an asset
        mesh_component = test_entity.add_component("Mesh")
        asset = Asset.find_asset_by_path(r"Objects\default\primitive_cube.cgf")
        mesh_component.set_component_property_value('MeshComponentRenderNode|Mesh asset', asset.id)

        # 6) Enter game mode
        helper.enter_game_mode(Tests.enter_game_mode)

    # 7) Verify there are no errors and warnings in the logs
    success_condition = not (section_tracer.has_errors or section_tracer.has_warnings)
    Report.result(Tests.no_errors_and_warnings_found, success_condition)
    if not success_condition:
        if section_tracer.has_warnings:
            Report.info(f"Warnings found: {section_tracer.warnings}")
        if section_tracer.has_errors:
            Report.info(f"Errors found: {section_tracer.errors}")
        Report.failure(Tests.no_errors_and_warnings_found)

    # 8) Exit game mode
    helper.exit_game_mode(Tests.exit_game_mode)



if __name__ == "__main__":
    import ImportPathHelper as imports
    imports.init()

    from utils import Report
    Report.start_test(C4976236_AddPhysxColliderComponent)
