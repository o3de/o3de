"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Test case ID : C12905528
Test Case Title : Check that user is warned if non-trigger collider component is used with force region

"""


# fmt: off
class Tests():
    create_test_entity     = ("Entity created successfully",        "Failed to create Entity")
    add_physx_force_region = ("PhysX Force Region component added", "Failed to add PhysX Force Region component")
    add_physx_collider     = ("PhysX Primitive Collider component added", "Failed to add PhysX Primitive Collider component")
    warnings_found         = ("Warnings found in logs",             "No warnings found in logs")
# fmt: on


def ForceRegion_WithNonTriggerColliderWarning():
    """
    Summary:
     Create entity with PhysX Force Region component. Check that user is warned if new PhysX Primitive Collider component is
     added to Entity.

    Expected Behavior:
     User is warned by message in the console that the PhysX Primitive Collider component was not marked as a trigger

    Test Steps:
     1) Load the empty level
     2) Create test entity
     3) Add PhysX Force Region component
     4) Start the Tracer to catch any errors and warnings
     5) Add PhysX Primitive Collider component to the Entity
     6) Verify there is warning in the logs

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    import azlmbr.legacy.general as general
    import editor_python_test_tools.hydra_editor_utils as hydra
    
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.utils import Tracer
    from consts.physics import PHYSX_PRIMITIVE_COLLIDER

    # 1) Load the empty level
    hydra.open_base_level()

    # 2) Create test entity
    test_entity = EditorEntity.create_editor_entity("TestEntity")
    Report.result(Tests.create_test_entity, test_entity.id.IsValid())

    # 3) Add PhysX Force Region component
    test_entity.add_component("PhysX Force Region")
    test_entity.add_component("PhysX Static Rigid Body")
    Report.result(Tests.add_physx_force_region, test_entity.has_component("PhysX Force Region"))

    # 4) Start the Tracer to catch any errors and warnings
    Report.info("Starting warning monitoring")
    with Tracer() as section_tracer:
        # 5) Add the PhysX Primitive Collider component
        test_entity.add_component(PHYSX_PRIMITIVE_COLLIDER)
        Report.result(Tests.add_physx_collider, test_entity.has_component(PHYSX_PRIMITIVE_COLLIDER))
        general.idle_wait_frames(1)
    Report.info("Ending warning monitoring")
    
    # 6) Verify there is warning in the logs
    success_condition = section_tracer.has_warnings
    # Checking if warning exist and the exact warning is caught in the expected lines in Test file
    Report.result(Tests.warnings_found, success_condition)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ForceRegion_WithNonTriggerColliderWarning)
