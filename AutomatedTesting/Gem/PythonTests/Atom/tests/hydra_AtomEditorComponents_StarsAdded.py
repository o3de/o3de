"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    stars_creation = (
        "Stars Entity successfully created",
        "P0: Stars Entity failed to be created")
    stars_component = (
        "Entity has a Stars component",
        "P0: Entity failed to find Stars component")
    stars_component_removal = (
        "Stars component successfully removed",
        "P0: Stars component failed to be removed")
    removal_undo = (
        "UNDO Stars component removal success",
        "P0: UNDO Stars component removal failed")
    stars_disabled = (
        "Stars component disabled",
        "P0: Stars component was not disabled")
    stars_enabled = (
        "Stars component enabled",
        "P0: Stars component was not enabled")
    stars_asset = (
        "Stars Asset property set",
        "P1: 'Stars Asset' property failed to set")
    exposure_min = (
        "Exposure property set",
        "P1: 'Exposure' property failed to set minimum value")
    exposure_max = (
        "Exposure property set",
        "P1: 'Exposure' property failed to set maximum value")
    twinkle_rate_min = (
        "Twinkle rate property set",
        "P1: 'Twinkle rate' property failed to set minimum value")
    twinkle_rate_max = (
        "Twinkle rate property set",
        "P1: 'Twinkle rate' property failed to set maximum value")
    radius_factor_min = (
        "Radius factor property set",
        "P1: 'Radius factor' property failed to set minimum value")
    radius_factor_max = (
        "Radius factor property set",
        "P1: 'Radius factor' property failed to set maximum value")
    enter_game_mode = (
        "Entered game mode",
        "P0: Failed to enter game mode")
    exit_game_mode = (
        "Exited game mode",
        "P0: Couldn't exit game mode")
    is_visible = (
        "Entity is visible",
        "P0: Entity was not visible")
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


def AtomEditorComponents_Stars_AddedToEntity():
    """
    Summary:
    Tests the Stars component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create an Stars entity with no components.
    2) Add Stars component to Stars entity.
    3) Remove the Stars component.
    4) Undo Stars component removal.
    5) Verify Stars component is enabled.
    6) Clear the default 'Stars Asset' property then set it to default again
    7) Update the 'Exposure' parameter to min/max values.
    8) Update the 'Twinkle rate' parameter to min/max values.
    9) Update the 'Radius factor' parameter to min/max values.
    10) Enter/Exit game mode.
    11) Test IsHidden.
    12) Test IsVisible.
    13) Delete Stars entity.
    14) UNDO deletion.
    15) REDO deletion.
    16) Look for errors and asserts.

    :return: None
    """

    import azlmbr.legacy.general as general

    from editor_python_test_tools.asset_utils import Asset
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.wait_utils import PrefabWaiter
    from editor_python_test_tools.utils import Report, Tracer, TestHelper
    from Atom.atom_utils.atom_constants import AtomComponentProperties

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("Graphics", "base_empty")

        # Test steps begin.
        # 1. Create an Stars entity with no components.
        stars_entity = EditorEntity.create_editor_entity(AtomComponentProperties.stars())
        Report.critical_result(Tests.stars_creation, stars_entity.exists())

        # 2. Add Stars component to Stars entity.
        stars_component = stars_entity.add_component(AtomComponentProperties.stars())
        Report.critical_result(Tests.stars_component,
                               stars_entity.has_component(AtomComponentProperties.stars()))

        # 3. Remove the Stars component.
        stars_component.remove()
        general.idle_wait_frames(1)
        Report.critical_result(Tests.stars_component_removal,
                               not stars_entity.has_component(AtomComponentProperties.stars()))

        # 4. Undo Stars component removal.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.removal_undo,
                      stars_entity.has_component(AtomComponentProperties.stars()))

        # 5. Verify Stars component is enabled.
        Report.result(Tests.stars_enabled, stars_component.is_enabled())

        # 6. Clear the default 'Stars Asset' property then set it to default again
        default_stars = Asset(stars_component.get_component_property_value(
            AtomComponentProperties.stars('Stars Asset')))
        stars_component.set_component_property_value(
            AtomComponentProperties.stars('Stars Asset'), None)
        general.idle_wait_frames(1)
        
        # Set 'Stars Asset' back to the default.stars Asset.id
        stars_component.set_component_property_value(
            AtomComponentProperties.stars('Stars Asset'), default_stars.id)
        Report.result(
            Tests.stars_asset,
            stars_component.get_component_property_value(
                AtomComponentProperties.stars('Stars Asset')) == default_stars.id)

        # 7. Update the 'Exposure' parameter to min/max values.
        # Update 'Exposure' parameter to minimum value.
        stars_component.set_component_property_value(
            AtomComponentProperties.stars('Exposure'), 0.0)
        Report.result(
            Tests.exposure_min,
            stars_component.get_component_property_value(
                AtomComponentProperties.stars('Exposure')) == 0.0)

        # Update 'Exposure' parameter to maximum value.
        stars_component.set_component_property_value(
            AtomComponentProperties.stars('Exposure'), 32.0)
        Report.result(
            Tests.exposure_max,
            stars_component.get_component_property_value(
                AtomComponentProperties.stars('Exposure')) == 32.0)

        # 8. Update the 'Twinkle rate' parameter to min/max values.
        # Update the 'Twinkle rate' parameter to minimum value.
        stars_component.set_component_property_value(
            AtomComponentProperties.stars('Twinkle rate'), 0.0)
        Report.result(
            Tests.twinkle_rate_min,
            stars_component.get_component_property_value(
                AtomComponentProperties.stars('Twinkle rate')) == 0.0)

        # Update the 'Twinkle rate' parameter to maximum value.
        stars_component.set_component_property_value(
            AtomComponentProperties.stars('Twinkle rate'), 10.0)
        Report.result(
            Tests.twinkle_rate_max,
            stars_component.get_component_property_value(
                AtomComponentProperties.stars('Twinkle rate')) == 10.0)

        # 9. Update the 'Radius factor' parameter to min/max values.
        # Update the 'Radius factor' parameter to minimum value.
        stars_component.set_component_property_value(
            AtomComponentProperties.stars('Radius factor'), 0.0)
        Report.result(
            Tests.radius_factor_min,
            stars_component.get_component_property_value(
                AtomComponentProperties.stars('Radius factor')) == 0.0)

        # Update the 'Radius factor' parameter to maximum value.
        stars_component.set_component_property_value(
            AtomComponentProperties.stars('Radius factor'), 64.0)
        Report.result(
            Tests.radius_factor_max,
            stars_component.get_component_property_value(
                AtomComponentProperties.stars('Radius factor')) == 64.0)

        # 10. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 11. Test IsHidden.
        stars_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, stars_entity.is_hidden() is True)

        # 12. Test IsVisible.
        stars_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, stars_entity.is_visible() is True)

        # 13. Delete Stars entity.
        stars_entity.delete()
        PrefabWaiter.wait_for_propagation()
        Report.result(Tests.entity_deleted, not stars_entity.exists())

        # 14. UNDO deletion.
        general.undo()
        PrefabWaiter.wait_for_propagation()
        Report.result(Tests.deletion_undo, stars_entity.exists())

        # 15. REDO deletion.
        general.redo()
        PrefabWaiter.wait_for_propagation()
        Report.result(Tests.deletion_redo, not stars_entity.exists())

        # 16. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_Stars_AddedToEntity)
