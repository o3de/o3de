"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Test case ID : C15425929
# Test Case Title : Verify that undo - redo operations do not create any error



# fmt: off
class Tests:
    entity_found      = ("Entity was initially found",         "Entity COULD NOT be found initially")
    entity_deleted    = ("Entity was deleted",                 "Entity WAS NOT deleted")
    deletion_undone   = ("Undo worked",                        "Undo DID NOT work")
    deletion_redone   = ("Redo worked",                        "Redo DID NOT work")
    no_error_occurred = ("Undo/redo completed without errors", "An error occurred during undo/redo")
# fmt: off


def Physics_UndoRedoWorksOnEntityWithPhysComponents():
    """
    Summary:
    Tests that no error messages arise when using the undo and redo functions in the editor.

    Level Description:
    DeleteMe - an entity that just exists above the terrain with a sphere shape component on it.

    Steps:
    1) Load level
    2) Initially find the entity
    3) Delete the entity
    4) Undo the deletion
    5) Redo the deletion
    6) Look for errors
    7) Close the editor

    Expected Behavior:
    The entity should be deleted, un-deleted, and re-deleted.

    :return: None
    """
    import os
    import sys
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.utils import Tracer

    import azlmbr.legacy.general as general

    helper.init_idle()

    # 1) Load the level
    helper.open_level("Physics", "Physics_UndoRedoWorksOnEntityWithPhysComponents")

    with Tracer() as error_tracer:
        # Entity to delete and undo and re-delete
        entity_name = "DeleteMe"

        # 2) Find entity initially
        entity_id = general.find_editor_entity(entity_name)
        Report.critical_result(Tests.entity_found, entity_id.IsValid())

        # 3) Delete entity
        general.select_objects([entity_name])
        general.delete_selected()
        entity_id = general.find_editor_entity(entity_name)
        Report.result(Tests.entity_deleted, not entity_id.IsValid())

        # 4) Undo deletion
        general.undo()
        entity_id = general.find_editor_entity(entity_name)
        Report.result(Tests.deletion_undone, entity_id.IsValid())

        # 5) Redo deletion
        general.redo()
        entity_id = general.find_editor_entity(entity_name)
        Report.result(Tests.deletion_redone, not entity_id.IsValid())

        # 6) Look for errors
        helper.wait_for_condition(lambda: error_tracer.has_errors, 1.0)
        Report.result(Tests.no_error_occurred, not error_tracer.has_errors)
        


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Physics_UndoRedoWorksOnEntityWithPhysComponents)
