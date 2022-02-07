"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# fmt: off
class Tests:
    camera_creation =           ("Camera Entity successfully created",           "Camera Entity failed to be created")
    camera_component_added =    ("Camera component was added to entity",         "Camera component failed to be added to entity")
    camera_component_check =    ("Entity has a Camera component",                "Entity failed to find Camera component")
    creation_undo =             ("UNDO Entity creation success",                 "UNDO Entity creation failed")
    creation_redo =             ("REDO Entity creation success",                 "REDO Entity creation failed")
    decal_creation =            ("Decal Entity successfully created",            "Decal Entity failed to be created")
    decal_component =           ("Entity has a Decal component",                 "Entity failed to find Decal component")
    material_property_set =     ("Material property set on Decal component",     "Couldn't set Material property on Decal component")
    enter_game_mode =           ("Entered game mode",                            "Failed to enter game mode")
    exit_game_mode =            ("Exited game mode",                             "Couldn't exit game mode")
    is_visible =                ("Entity is visible",                            "Entity was not visible")
    is_hidden =                 ("Entity is hidden",                             "Entity was not hidden")
    entity_deleted =            ("Entity deleted",                               "Entity was not deleted")
    deletion_undo =             ("UNDO deletion success",                        "UNDO deletion failed")
    deletion_redo =             ("REDO deletion success",                        "REDO deletion failed")
    no_error_occurred =         ("No errors detected",                           "Errors were detected")
# fmt: on


def AtomEditorComponents_Decal_AddedToEntity():
    """
    Summary:
    Tests the Decal component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create a Decal entity with no components.
    2) Add Decal component to Decal entity.
    3) UNDO the entity creation and component addition.
    4) REDO the entity creation and component addition.
    5) Enter/Exit game mode.
    6) Test IsHidden.
    7) Test IsVisible.
    8) Set Material property on Decal component.
    9) Delete Decal entity.
    10) UNDO deletion.
    11) REDO deletion.
    12) Look for errors.

    :return: None
    """
    import os

    import azlmbr.asset as asset
    import azlmbr.bus as bus
    import azlmbr.legacy.general as general
    import azlmbr.math as math

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, Tracer, TestHelper as helper

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        helper.init_idle()
        helper.open_level("", "Base")

        # Test steps begin.
        # 1. Create a Decal entity with no components.
        decal_name = "Decal"
        decal_entity = EditorEntity.create_editor_entity_at(math.Vector3(512.0, 512.0, 34.0), decal_name)
        Report.critical_result(Tests.decal_creation, decal_entity.exists())

        # 2. Add Decal component to Decal entity.
        decal_component = decal_entity.add_component(decal_name)
        Report.critical_result(Tests.decal_component, decal_entity.has_component(decal_name))

        # 3. UNDO the entity creation and component addition.
        # -> UNDO component addition.
        general.undo()
        # -> UNDO naming entity.
        general.undo()
        # -> UNDO selecting entity.
        general.undo()
        # -> UNDO entity creation.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.creation_undo, not decal_entity.exists())

        # 4. REDO the entity creation and component addition.
        # -> REDO entity creation.
        general.redo()
        # -> REDO selecting entity.
        general.redo()
        # -> REDO naming entity.
        general.redo()
        # -> REDO component addition.
        general.redo()
        general.idle_wait_frames(1)
        Report.result(Tests.creation_redo, decal_entity.exists())

        # 5. Enter/Exit game mode.
        helper.enter_game_mode(Tests.enter_game_mode)
        general.idle_wait_frames(1)
        helper.exit_game_mode(Tests.exit_game_mode)

        # 6. Test IsHidden.
        decal_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, decal_entity.is_hidden() is True)

        # 7. Test IsVisible.
        decal_entity.set_visibility_state(True)
        general.idle_wait_frames(1)
        Report.result(Tests.is_visible, decal_entity.is_visible() is True)

        # 8. Set Material property on Decal component.
        decal_material_property_path = "Controller|Configuration|Material"
        decal_material_asset_path = os.path.join("AutomatedTesting", "Materials", "basic_grey.material")
        decal_material_asset = asset.AssetCatalogRequestBus(
            bus.Broadcast, "GetAssetIdByPath", decal_material_asset_path, math.Uuid(), False)
        decal_component.set_component_property_value(decal_material_property_path, decal_material_asset)
        get_material_property = decal_component.get_component_property_value(decal_material_property_path)
        Report.result(Tests.material_property_set, get_material_property == decal_material_asset)

        # 9. Delete Decal entity.
        decal_entity.delete()
        Report.result(Tests.entity_deleted, not decal_entity.exists())

        # 10. UNDO deletion.
        general.undo()
        general.idle_wait_frames(1)
        Report.result(Tests.deletion_undo, decal_entity.exists())

        # 11. REDO deletion.
        general.redo()
        Report.result(Tests.deletion_redo, not decal_entity.exists())

        # 12. Look for errors.
        helper.wait_for_condition(lambda: error_tracer.has_errors, 1.0)
        Report.result(Tests.no_error_occurred, not error_tracer.has_errors)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_Decal_AddedToEntity)
