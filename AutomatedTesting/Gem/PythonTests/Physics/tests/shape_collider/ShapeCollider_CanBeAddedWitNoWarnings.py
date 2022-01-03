"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Test case ID : C19578021
Test Case Title : Verify that a shape collider component may be added to an entity along with one or more PhysX collider components

"""


# fmt: off
class Tests():
    create_collider_entity   = ("Created Collider Entity",    "Failed to create Collider Entity")
    add_physx_shape_collider = ("PhysX Shape Collider added", "Failed to add PhysX Shape Collider")
    add_box_shape            = ("Box Shape added",            "Failed to add Box Shape")
    add_physx_collider       = ("PhysX Collider added",       "Failed to add PhysX Collider")
    no_warnings_found        = ("Trace found no warnings",    "One or more components has been removed")
# fmt: on


def ShapeCollider_CanBeAddedWitNoWarnings():
    """
    Summary:
     Adding a PhysX Collider component when a PhysX Shape Collider and Box Shape components are already present

    Expected Behavior:
     When adding the PhysX Collider, there should be no warnings in the entity outliner

    Test Steps:
     1) Load the empty level
     2) Create an entity
     3) Add the PhysX Shape Collider and a Box Shape components
     4) Start the Tracer to catch any warnings while adding the PhysX Collider
     5) Add the PhysX Collider component
     6) Verify there are no warnings in the entity outliner

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    # Helper Files

    from editor_python_test_tools.editor_entity_utils import EditorEntity as Entity
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.utils import Tracer

    # Open 3D Engine Imports
    import azlmbr.legacy.general as general

    helper.init_idle()
    # 1) Load the empty level
    helper.open_level("Physics", "Base")

    # 2) Create an entity
    collider_entity = Entity.create_editor_entity("Collider")
    Report.result(Tests.create_collider_entity, collider_entity.id.IsValid())

    # 3) Add the PhysX Shape Collider and a Box Shape components
    collider_entity.add_component("PhysX Shape Collider")
    Report.result(Tests.add_physx_shape_collider, collider_entity.has_component("PhysX Shape Collider"))

    collider_entity.add_component("Box Shape")
    Report.result(Tests.add_box_shape, collider_entity.has_component("Box Shape"))

    # 4) Start the Tracer to catch any warnings while adding the PhysX Collider
    with Tracer() as section_tracer:
        # 5) Add the PhysX Collider component
        collider_entity.add_component("PhysX Collider")
        Report.result(Tests.add_physx_collider, collider_entity.has_component("PhysX Collider"))

    # 6) Verify there are no warnings in the entity outliner
    success_condition = not section_tracer.has_warnings and not section_tracer.has_errors
    Report.result(Tests.no_warnings_found, success_condition)

    if not success_condition:
        exception_str = ""
        if section_tracer.has_warnings:
            exception_str += f"Warnings found: {section_tracer.warnings}\n"
        if section_tracer.has_errors:
            exception_str += f"Errors found: {section_tracer.errors}"
        Report.failure(exception_str)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ShapeCollider_CanBeAddedWitNoWarnings)
