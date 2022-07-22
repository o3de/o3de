"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class Tests:
    postfx_layer_entity_creation = (
        "PostFX Layer Entity successfully created",
        "P0: PostFX Layer Entity failed to be created")
    postfx_layer_component_added = (
        "Entity has a PostFX Layer component",
        "P0: Entity failed to find PostFX Layer component")
    postfx_layer_component_removed = (
        "Remove PostFX Layer component success",
        "P0: Remove PostFX Layer component failed")
    enter_game_mode = (
        "Entered game mode",
        "P0: Failed to enter game mode")
    exit_game_mode = (
        "Exited game mode",
        "P0: Couldn't exit game mode")
    is_visible = (
        "P0: Entity is visible",
        "Entity was not visible")
    is_hidden = (
        "Entity is hidden",
        "P0: Entity was not hidden")
    entity_deleted = (
        "Entity deleted",
        "P0: Entity was not deleted")
    deletion_undo = (
        "UNDO deletion success",
        "P0: UNDO deletion failed")
    deletion_redo = (
        "REDO deletion success",
        "P0: REDO deletion failed")
    priority = (
        "'Priority' property set",
        "P1: 'Priority' property failed to set")
    weight = (
        "'Weight' property set",
        "P1: 'Weight' property failed to set")
    selected_container = (
        "'Select Camera Tags Only' is a container property",
        "P1: 'Select Camera Tags Only' is not a container property")
    selected_append = (
        "'Select Camera Tags Only' container property appended string value",
        "P1: 'Select Camera Tags Only' container property failed to append string value")
    excluded_container = (
        "'Excluded Camera Tags' is a container property",
        "P1: 'Excluded Camera Tags' is not a container property")
    excluded_append = (
        "'Excluded Camera Tags' container property appended string value",
        "P1: 'Excluded Camera Tags' container property failed to append string value")


def AtomEditorComponents_postfx_layer_AddedToEntity():
    """
    Summary:
    Tests the PostFX Layer component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create a PostFX Layer entity with no components.
    2) Add a PostFX Layer component to PostFX Layer entity.
    3) Remove PostFX Layer component.
    4) REDO the entity creation and component addition.
    5) Set each 'Layer Category' value.
    6) Set 'Priority' property.
    7) Set 'Weight' property.
    8) Set a 'Select Camera Tags Only' tag.
    9) Set an 'Excluded Camera Tags' tag.
    10) Enter/Exit game mode.
    11) Test IsHidden.
    12) Test IsVisible.
    13) Delete PostFX Layer entity.
    14) UNDO deletion.
    15) REDO deletion.
    16) Look for errors.

    :return: None
    """

    import azlmbr.legacy.general as general

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper
    from Atom.atom_utils.atom_constants import AtomComponentProperties, POSTFX_LAYER_CATEGORY

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("Graphics", "base_empty")

        # Test steps begin.
        # 1. Create a PostFX Layer entity with no components.
        postfx_layer_entity = EditorEntity.create_editor_entity(AtomComponentProperties.postfx_layer())
        Report.critical_result(Tests.postfx_layer_entity_creation, postfx_layer_entity.exists())

        # 2. Add a PostFX Layer component to PostFX Layer entity.
        postfx_layer_component = postfx_layer_entity.add_component(AtomComponentProperties.postfx_layer())
        Report.critical_result(
            Tests.postfx_layer_component_added,
            postfx_layer_entity.has_component(AtomComponentProperties.postfx_layer()))

        # 3. Remove PostFX Layer component.
        postfx_layer_component.remove()
        general.idle_wait_frames(1)
        Report.result(
            Tests.postfx_layer_component_removed,
            not postfx_layer_entity.has_component(AtomComponentProperties.postfx_layer()))

        # 4. UNDO component removal.
        general.undo()
        general.idle_wait_frames(1)
        Report.critical_result(
            Tests.postfx_layer_component_added,
            postfx_layer_entity.has_component(AtomComponentProperties.postfx_layer()))

        # 5. Set each 'Layer Category' value.
        for category in POSTFX_LAYER_CATEGORY:
            postfx_layer_component.set_component_property_value(
                AtomComponentProperties.postfx_layer('Layer Category'), POSTFX_LAYER_CATEGORY[category])
            general.idle_wait_frames(1)
            layer_category = (f"'Layer Category' set to {category}", f"P1: 'Layer Category' failed to set {category}")
            Report.result(
                layer_category,
                postfx_layer_component.get_component_property_value(
                    AtomComponentProperties.postfx_layer('Layer Category')) == POSTFX_LAYER_CATEGORY[category])

        # 6. Set 'Priority' property.
        postfx_layer_component.set_component_property_value(AtomComponentProperties.postfx_layer('Priority'), 20)
        Report.result(
            Tests.priority,
            postfx_layer_component.get_component_property_value(AtomComponentProperties.postfx_layer('Priority')) == 20)

        # 7. Set 'Weight' property.
        postfx_layer_component.set_component_property_value(AtomComponentProperties.postfx_layer('Weight'), 0.5)
        Report.result(
            Tests.weight,
            postfx_layer_component.get_component_property_value(AtomComponentProperties.postfx_layer('Weight')) == 0.5)

        # 8. Set a 'Select Camera Tags Only' tag.
        Report.result(
            Tests.selected_container,
            postfx_layer_component.is_property_container(
                AtomComponentProperties.postfx_layer('Select Camera Tags Only')))
        # https://github.com/o3de/o3de/issues/10464
        # postfx_layer_component.reset_container(AtomComponentProperties.postfx_layer('Select Camera Tags Only'))
        # postfx_layer_component.append_container_item(
        #     AtomComponentProperties.postfx_layer('Select Camera Tags Only'), value='yes')
        # Report.result(
        #     Tests.selected_append,
        #     postfx_layer_component.get_container_item(
        #         AtomComponentProperties.postfx_layer('Select Camera Tags Only'), key=0) == 'yes')

        # 9. Set an 'Excluded Camera Tags' tag.
        Report.result(
            Tests.excluded_container,
            postfx_layer_component.is_property_container(
                AtomComponentProperties.postfx_layer('Excluded Camera Tags')))
        # https://github.com/o3de/o3de/issues/10464
        # postfx_layer_component.reset_container(AtomComponentProperties.postfx_layer('Excluded Camera Tags'))
        # postfx_layer_component.append_container_item(
        #     AtomComponentProperties.postfx_layer('Excluded Camera Tags'), 'not this camera')
        # Report.result(
        #     Tests.excluded_append,
        #     postfx_layer_component.get_container_item(
        #         AtomComponentProperties.postfx_layer('Excluded Camera Tags'), key=0) == 'not this camera')

        # 10. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 11. Test IsHidden.
        postfx_layer_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, postfx_layer_entity.is_hidden() is True)

        # 12. Test IsVisible.
        postfx_layer_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, postfx_layer_entity.is_visible() is True)

        # 13. Delete PostFX Layer entity.
        postfx_layer_entity.delete()
        Report.result(Tests.entity_deleted, not postfx_layer_entity.exists())

        # 14. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, postfx_layer_entity.exists())

        # 15. REDO deletion.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_redo, not postfx_layer_entity.exists())

        # 16. Look for errors or asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_postfx_layer_AddedToEntity)
