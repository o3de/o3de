"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Test case ID : C17411467
Test Case Title : Check that Physx Ragdoll component can be added without errors/warnings

"""

# fmt: off
class Tests():
    create_test_entity  = ("Entity created successfully",        "Failed to create Entity")
    add_actor_component = ("Actor component added",              "Failed to add Actor component")
    add_animgraph       = ("AnimGraph component added",          "Failed to add AnimGraph component")
    add_physx_ragdoll   = ("PhysX Ragdoll added",                "Failed to add PhysX Ragdoll")
    no_warnings_errors  = ("Tracer found no errors or warnings", "Tracer found errors or warnings")
# fmt: on


def Ragdoll_AddPhysxRagdollComponentWorks():
    """
    Summary:
    Load level with Entity having Actor, AnimGraph and PhysX Ragdoll components.
    Verify that editor remains stable.

    Expected Behavior:
    Physx Ragdoll component can be added without any errors.

    Test Steps:
     1) Load the level
     2) Create test entity
     3) Add Actor and AnimGraph components
     4) Start the Tracer to catch any warnings while adding the PhysX Ragdoll component
     5) Add PhysX Ragdoll component
     6) Verify there are no errors/warnings in the entity outliner

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.utils import Tracer

    helper.init_idle()
    # 1) Load the level
    helper.open_level("Physics", "Base")

    # 2) Create test entity
    test_entity = EditorEntity.create_editor_entity("TestEntity")
    Report.result(Tests.create_test_entity, test_entity.id.IsValid())

    # 3) Add Actor and AnimGraph components
    test_entity.add_component("Actor")
    Report.result(Tests.add_actor_component, test_entity.has_component("Actor"))

    test_entity.add_component("Anim Graph")
    Report.result(Tests.add_animgraph, test_entity.has_component("Anim Graph"))

    # 4) Start the Tracer to catch any errors while adding the PhysX Ragdoll component
    with Tracer() as section_tracer:
        # 5) Add the PhysX Ragdoll component
        ragdoll_component = test_entity.add_component("PhysX Ragdoll")
        success_check = (
            ragdoll_component.id.get_entity_id() == test_entity.id
            and ragdoll_component.get_component_name() == "PhysX Ragdoll"
        ) 
        # Using this alternate way to check if PhysX Ragdoll is added to entity since there is an issue with the
        # usual method in case of this component. Returned False for test_entity.has_component("PhysX Ragdoll")
        Report.result(Tests.add_physx_ragdoll, success_check)

    # 6) Verify there are no errors in the entity outliner
    success_condition = not (section_tracer.has_errors or section_tracer.has_warnings)
    Report.result(Tests.no_warnings_errors, success_condition)
    if not success_condition:
        if section_tracer.has_warnings:
            Report.info(f"Warnings found: {section_tracer.warnings}")
        if section_tracer.has_errors:
            Report.info(f"Errors found: {section_tracer.errors}")
        Report.failure(Tests.no_warnings_found)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Ragdoll_AddPhysxRagdollComponentWorks)
