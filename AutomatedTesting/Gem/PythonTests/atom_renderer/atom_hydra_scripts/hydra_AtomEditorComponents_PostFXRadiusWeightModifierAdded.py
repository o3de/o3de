"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Hydra script that creates an entity & attaches the PostFX Radius Weight Modifier component to it for test verification.
"""

# fmt: off
class Tests:
    camera_creation = ("camera_entity Entity successfully created", "camera_entity Entity failed to be created")
    camera_component_added = ("Camera component was added to entity", "Camera component failed to be added to entity")
    camera_component_check = ("Entity has a Camera component", "Entity failed to find Camera component")
    creation_undo = ("UNDO Entity creation success", "UNDO Entity creation failed")
    creation_redo = ("REDO Entity creation success", "REDO Entity creation failed")
    postfx_radius_weight_creation = (
        "PostFX Radius Weight Modifier Entity successfully created",
        "PostFX Radius Weight Modifier Entity failed to be created")
    postfx_radius_weight_component = (
        "Entity has a PostFX Radius Weight Modifier component",
        "Entity failed to find PostFX Radius Weight Modifier component")
    enter_game_mode = ("Entered game mode", "Failed to enter game mode")
    exit_game_mode = ("Exited game mode", "Couldn't exit game mode")
    is_visible = ("Entity is visible", "Entity was not visible")
    is_hidden = ("Entity is hidden", "Entity was not hidden")
    entity_deleted = ("Entity deleted", "Entity was not deleted")
    deletion_undo = ("UNDO deletion success", "UNDO deletion failed")
    deletion_redo = ("REDO deletion success", "REDO deletion failed")
    no_error_occurred = ("No errors detected", "Errors were detected")
# fmt: on


def AtomEditorComponents_PostFXRadiusWeightModifier_AddedToEntity():
    """
    Summary:
    Tests the PostFX Radius Weight Modifier component can be added to an entity and has the expected functionality.
    First it will setup the test by deleting existing entities. After that it follows the test steps for pass/fail.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Creation of entity with PostFX Radius Weight Modifier component.
    2) UNDO the entity creation.
    3) REDO the entity creation.
    4) Enter/Exit game mode.
    5) Hide test.
    6) Visible test.
    7) Delete PostFX Radius Weight Modifier entity.
    8) UNDO deletion.
    9) REDO deletion.
    10) Look for errors.

    :return: None
    """
    import azlmbr.bus as bus
    import azlmbr.entity as entity
    import azlmbr.editor as editor
    import azlmbr.legacy.general as general
    import azlmbr.math as math
    import azlmbr.paths

    from editor_python_test_tools import hydra_editor_utils as hydra
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.utils import Tracer

    with Tracer() as error_tracer:
        # Wait for Editor idle loop before executing Python hydra scripts.
        helper.init_idle()

        # Delete all existing entities initially for test setup.
        search_filter = azlmbr.entity.SearchFilter()
        all_entities = entity.SearchBus(azlmbr.bus.Broadcast, "SearchEntities", search_filter)
        editor.ToolsApplicationRequestBus(bus.Broadcast, "DeleteEntities", all_entities)

        # Set up camera entity and component.
        camera_entity = hydra.Entity("camera_entity")
        camera_entity.create_entity(math.Vector3(512.0, 512.0, 34.0), ["Camera"])
        Report.result(Tests.camera_creation, camera_entity.id.IsValid())

        # 1. Creation of entity with PostFX Radius Weight Modifier component.
        postfx_radius_weight = "PostFX Radius Weight Modifier"
        postfx_radius_weight_entity = hydra.Entity(f"{postfx_radius_weight}")
        postfx_radius_weight_entity.create_entity(math.Vector3(512.0, 512.0, 34.0), [postfx_radius_weight])
        has_component = hydra.has_components(postfx_radius_weight_entity.id, [postfx_radius_weight])
        Report.result(Tests.postfx_radius_weight_creation, postfx_radius_weight_entity.id.IsValid())
        Report.critical_result(Tests.postfx_radius_weight_component, has_component)

        # 2. UNDO the entity creation.
        general.undo()
        Report.result(Tests.creation_undo, postfx_radius_weight_entity.id.IsValid())

        # 3. REDO the entity creation.
        general.redo()
        Report.result(Tests.creation_redo, postfx_radius_weight_entity.id.isValid())

        # 4. Enter/Exit game mode.
        helper.enter_game_mode(Tests.enter_game_mode)
        helper.exit_game_mode(Tests.exit_game_mode)

        # 5. Hide test.
        editor.EditorEntityAPIBus(bus.Event, "SetVisibilityState", postfx_radius_weight_entity.id, False)
        is_hidden = editor.EditorEntityInfoRequestBus(bus.Event, 'IsHidden', postfx_radius_weight_entity.id)
        Report.result(Tests.is_hidden, is_hidden is True)

        # 6. Visible test.
        editor.EditorEntityAPIBus(bus.Event, "SetVisibilityState", postfx_radius_weight_entity.id, True)
        is_visible = editor.EditorEntityInfoRequestBus(bus.Event, 'IsVisible', postfx_radius_weight_entity.id)
        Report.result(Tests.is_visible, is_visible is True)

        # 7. Delete PostFX Radius Weight Modifier entity.
        editor.ToolsApplicationRequestBus(bus.Broadcast, "DeleteEntityById", postfx_radius_weight_entity.id)
        deleted_entity = hydra.find_entity_by_name(postfx_radius_weight)
        Report.result(Tests.entity_deleted, len(deleted_entity) == 0)

        # 8. UNDO deletion.
        general.undo()
        Report.result(Tests.deletion_undo, postfx_radius_weight_entity.id.isValid())

        # 9. REDO deletion.
        general.redo()
        Report.result(Tests.deletion_redo, not postfx_radius_weight_entity.id.isValid())

        # 10. Look for errors.
        helper.wait_for_condition(lambda: error_tracer.has_errors, 1.0)
        Report.result(Tests.no_error_occurred, not error_tracer.has_errors)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_PostFXRadiusWeightModifier_AddedToEntity)
