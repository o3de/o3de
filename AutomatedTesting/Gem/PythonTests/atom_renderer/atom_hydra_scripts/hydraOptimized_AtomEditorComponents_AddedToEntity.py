"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Hydra script that creates an entity and attaches Atom components to it for test verification.
"""

import os

expected_lines = [
    # Decal (Atom) Component
    "Decal (Atom) Entity successfully created",
    "Decal (Atom)_test: Component added to the entity: True",
    "Decal (Atom)_test: Component removed after UNDO: True",
    "Decal (Atom)_test: Component added after REDO: True",
    "Decal (Atom)_test: Entered game mode: True",
    "Decal (Atom)_test: Exit game mode: True",
    "Decal (Atom) Controller|Configuration|Material: SUCCESS",
    "Decal (Atom)_test: Entity is hidden: True",
    "Decal (Atom)_test: Entity is shown: True",
    "Decal (Atom)_test: Entity deleted: True",
    "Decal (Atom)_test: UNDO entity deletion works: True",
    "Decal (Atom)_test: REDO entity deletion works: True",
    # DepthOfField Component
    "DepthOfField Entity successfully created",
    "DepthOfField_test: Component added to the entity: True",
    "DepthOfField_test: Component removed after UNDO: True",
    "DepthOfField_test: Component added after REDO: True",
    "DepthOfField_test: Entered game mode: True",
    "DepthOfField_test: Exit game mode: True",
    "DepthOfField_test: Entity disabled initially: True",
    "DepthOfField_test: Entity enabled after adding required components: True",
    "DepthOfField Controller|Configuration|Camera Entity: SUCCESS",
    "DepthOfField_test: Entity is hidden: True",
    "DepthOfField_test: Entity is shown: True",
    "DepthOfField_test: Entity deleted: True",
    "DepthOfField_test: UNDO entity deletion works: True",
    "DepthOfField_test: REDO entity deletion works: True",
    # Exposure Control Component
    "Exposure Control Entity successfully created",
    "Exposure Control_test: Component added to the entity: True",
    "Exposure Control_test: Component removed after UNDO: True",
    "Exposure Control_test: Component added after REDO: True",
    "Exposure Control_test: Entered game mode: True",
    "Exposure Control_test: Exit game mode: True",
    "Exposure Control_test: Entity disabled initially: True",
    "Exposure Control_test: Entity enabled after adding required components: True",
    "Exposure Control_test: Entity is hidden: True",
    "Exposure Control_test: Entity is shown: True",
    "Exposure Control_test: Entity deleted: True",
    "Exposure Control_test: UNDO entity deletion works: True",
    "Exposure Control_test: REDO entity deletion works: True",
    # Global Skylight (IBL) Component
    "Global Skylight (IBL) Entity successfully created",
    "Global Skylight (IBL)_test: Component added to the entity: True",
    "Global Skylight (IBL)_test: Component removed after UNDO: True",
    "Global Skylight (IBL)_test: Component added after REDO: True",
    "Global Skylight (IBL)_test: Entered game mode: True",
    "Global Skylight (IBL)_test: Exit game mode: True",
    "Global Skylight (IBL) Controller|Configuration|Diffuse Image: SUCCESS",
    "Global Skylight (IBL) Controller|Configuration|Specular Image: SUCCESS",
    "Global Skylight (IBL)_test: Entity is hidden: True",
    "Global Skylight (IBL)_test: Entity is shown: True",
    "Global Skylight (IBL)_test: Entity deleted: True",
    "Global Skylight (IBL)_test: UNDO entity deletion works: True",
    "Global Skylight (IBL)_test: REDO entity deletion works: True",
    # Physical Sky Component
    "Physical Sky Entity successfully created",
    "Physical Sky component was added to entity",
    "Entity has a Physical Sky component",
    "Physical Sky_test: Component added to the entity: True",
    "Physical Sky_test: Component removed after UNDO: True",
    "Physical Sky_test: Component added after REDO: True",
    "Physical Sky_test: Entered game mode: True",
    "Physical Sky_test: Exit game mode: True",
    "Physical Sky_test: Entity is hidden: True",
    "Physical Sky_test: Entity is shown: True",
    "Physical Sky_test: Entity deleted: True",
    "Physical Sky_test: UNDO entity deletion works: True",
    "Physical Sky_test: REDO entity deletion works: True",
    # PostFX Layer Component
    "PostFX Layer Entity successfully created",
    "PostFX Layer_test: Component added to the entity: True",
    "PostFX Layer_test: Component removed after UNDO: True",
    "PostFX Layer_test: Component added after REDO: True",
    "PostFX Layer_test: Entered game mode: True",
    "PostFX Layer_test: Exit game mode: True",
    "PostFX Layer_test: Entity is hidden: True",
    "PostFX Layer_test: Entity is shown: True",
    "PostFX Layer_test: Entity deleted: True",
    "PostFX Layer_test: UNDO entity deletion works: True",
    "PostFX Layer_test: REDO entity deletion works: True",
    # PostFX Radius Weight Modifier Component
    "PostFX Radius Weight Modifier Entity successfully created",
    "PostFX Radius Weight Modifier_test: Component added to the entity: True",
    "PostFX Radius Weight Modifier_test: Component removed after UNDO: True",
    "PostFX Radius Weight Modifier_test: Component added after REDO: True",
    "PostFX Radius Weight Modifier_test: Entered game mode: True",
    "PostFX Radius Weight Modifier_test: Exit game mode: True",
    "PostFX Radius Weight Modifier_test: Entity is hidden: True",
    "PostFX Radius Weight Modifier_test: Entity is shown: True",
    "PostFX Radius Weight Modifier_test: Entity deleted: True",
    "PostFX Radius Weight Modifier_test: UNDO entity deletion works: True",
    "PostFX Radius Weight Modifier_test: REDO entity deletion works: True",
    # Light Component
    "Light Entity successfully created",
    "Light_test: Component added to the entity: True",
    "Light_test: Component removed after UNDO: True",
    "Light_test: Component added after REDO: True",
    "Light_test: Entered game mode: True",
    "Light_test: Exit game mode: True",
    "Light_test: Entity is hidden: True",
    "Light_test: Entity is shown: True",
    "Light_test: Entity deleted: True",
    "Light_test: UNDO entity deletion works: True",
    "Light_test: REDO entity deletion works: True",
    # Display Mapper Component
    "Display Mapper Entity successfully created",
    "Display Mapper_test: Component added to the entity: True",
    "Display Mapper_test: Component removed after UNDO: True",
    "Display Mapper_test: Component added after REDO: True",
    "Display Mapper_test: Entered game mode: True",
    "Display Mapper_test: Exit game mode: True",
    "Display Mapper_test: Entity is hidden: True",
    "Display Mapper_test: Entity is shown: True",
    "Display Mapper_test: Entity deleted: True",
    "Display Mapper_test: UNDO entity deletion works: True",
    "Display Mapper_test: REDO entity deletion works: True",
]


def run():
    """
    Summary:
    The below common tests are done for each of the components.
    1) Addition of component to the entity
    2) UNDO/REDO of addition of component
    3) Enter/Exit game mode
    4) Hide/Show entity containing component
    5) Deletion of component
    6) UNDO/REDO of deletion of component
    Some additional tests for specific components include
    1) Assigning value to some properties of each component
    2) Verifying if the component is activated only when the required components are added

    Expected Result:
    1) Component can be added to an entity.
    2) The addition of component can be undone and redone.
    3) Game mode can be entered/exited without issue.
    4) Entity with component can be hidden/shown.
    5) Component can be deleted.
    6) The deletion of component can be undone and redone.
    7) Component is activated only when the required components are added
    8) Values can be assigned to the properties of the component

    :return: None
    """
    import azlmbr.math as math
    import azlmbr.bus as bus
    import azlmbr.paths
    import azlmbr.asset as asset
    import azlmbr.entity as entity
    import azlmbr.legacy.general as general
    import azlmbr.editor as editor

    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.utils import TestHelper

    def create_entity_undo_redo_component_addition(component_name):
        new_entity = hydra.Entity(f"{component_name}")
        new_entity.create_entity(math.Vector3(512.0, 512.0, 34.0), [component_name])
        general.log(f"{component_name}_test: Component added to the entity: "
                    f"{hydra.has_components(new_entity.id, [component_name])}")

        # undo component addition
        general.undo()
        TestHelper.wait_for_condition(lambda: not hydra.has_components(new_entity.id, [component_name]), 2.0)
        general.log(f"{component_name}_test: Component removed after UNDO: "
                    f"{not hydra.has_components(new_entity.id, [component_name])}")

        # redo component addition
        general.redo()
        TestHelper.wait_for_condition(lambda: hydra.has_components(new_entity.id, [component_name]), 2.0)
        general.log(f"{component_name}_test: Component added after REDO: "
                    f"{hydra.has_components(new_entity.id, [component_name])}")

        return new_entity

    def verify_enter_exit_game_mode(component_name):
        general.enter_game_mode()
        TestHelper.wait_for_condition(lambda: general.is_in_game_mode(), 2.0)
        general.log(f"{component_name}_test: Entered game mode: {general.is_in_game_mode()}")
        general.exit_game_mode()
        TestHelper.wait_for_condition(lambda: not general.is_in_game_mode(), 2.0)
        general.log(f"{component_name}_test: Exit game mode: {not general.is_in_game_mode()}")

    def verify_hide_unhide_entity(component_name, entity_obj):

        def is_entity_hidden(entity_id):
            return editor.EditorEntityInfoRequestBus(bus.Event, "IsHidden", entity_id)

        editor.EditorEntityAPIBus(bus.Event, "SetVisibilityState", entity_obj.id, False)
        general.idle_wait_frames(1)
        general.log(f"{component_name}_test: Entity is hidden: {is_entity_hidden(entity_obj.id)}")
        editor.EditorEntityAPIBus(bus.Event, "SetVisibilityState", entity_obj.id, True)
        general.idle_wait_frames(1)
        general.log(f"{component_name}_test: Entity is shown: {not is_entity_hidden(entity_obj.id)}")

    def verify_deletion_undo_redo(component_name, entity_obj):
        editor.ToolsApplicationRequestBus(bus.Broadcast, "DeleteEntityById", entity_obj.id)
        TestHelper.wait_for_condition(lambda: not hydra.find_entity_by_name(entity_obj.name), 2.0)
        general.log(f"{component_name}_test: Entity deleted: {not hydra.find_entity_by_name(entity_obj.name)}")

        general.undo()
        TestHelper.wait_for_condition(lambda: hydra.find_entity_by_name(entity_obj.name) is not None, 2.0)
        general.log(f"{component_name}_test: UNDO entity deletion works: "
                    f"{hydra.find_entity_by_name(entity_obj.name) is not None}")

        general.redo()
        TestHelper.wait_for_condition(lambda: not hydra.find_entity_by_name(entity_obj.name), 2.0)
        general.log(f"{component_name}_test: REDO entity deletion works: "
                    f"{not hydra.find_entity_by_name(entity_obj.name)}")

    def verify_required_component_addition(entity_obj, components_to_add, component_name):

        def is_component_enabled(entity_componentid_pair):
            return editor.EditorComponentAPIBus(bus.Broadcast, "IsComponentEnabled", entity_componentid_pair)

        general.log(
            f"{component_name}_test: Entity disabled initially: "
            f"{not is_component_enabled(entity_obj.components[0])}")
        for component in components_to_add:
            entity_obj.add_component(component)
        TestHelper.wait_for_condition(lambda: is_component_enabled(entity_obj.components[0]), 2.0)
        general.log(
            f"{component_name}_test: Entity enabled after adding "
            f"required components: {is_component_enabled(entity_obj.components[0])}"
        )

    def verify_set_property(entity_obj, path, value):
        entity_obj.get_set_test(0, path, value)

    # Wait for Editor idle loop before executing Python hydra scripts.
    TestHelper.init_idle()

    # Delete all existing entities initially
    search_filter = azlmbr.entity.SearchFilter()
    all_entities = entity.SearchBus(azlmbr.bus.Broadcast, "SearchEntities", search_filter)
    editor.ToolsApplicationRequestBus(bus.Broadcast, "DeleteEntities", all_entities)

    class ComponentTests:
        """Test launcher for each component."""
        def __init__(self, component_name, *additional_tests):
            self.component_name = component_name
            self.additional_tests = additional_tests
            self.run_component_tests()

        def run_component_tests(self):
            # Run common and additional tests
            entity_obj = create_entity_undo_redo_component_addition(self.component_name)

            # Enter/Exit game mode test
            verify_enter_exit_game_mode(self.component_name)

            # Any additional tests are executed here
            for test in self.additional_tests:
                test(entity_obj)

            # Hide/Unhide entity test
            verify_hide_unhide_entity(self.component_name, entity_obj)

            # Deletion/Undo/Redo test
            verify_deletion_undo_redo(self.component_name, entity_obj)

    # DepthOfField Component
    camera_entity = hydra.Entity("camera_entity")
    camera_entity.create_entity(math.Vector3(512.0, 512.0, 34.0), ["Camera"])
    depth_of_field = "DepthOfField"
    ComponentTests(
        depth_of_field,
        lambda entity_obj: verify_required_component_addition(entity_obj, ["PostFX Layer"], depth_of_field),
        lambda entity_obj: verify_set_property(
            entity_obj, "Controller|Configuration|Camera Entity", camera_entity.id))

    # Decal Component
    material_asset_path = os.path.join("AutomatedTesting", "Materials", "basic_grey.material")
    material_asset = asset.AssetCatalogRequestBus(
        bus.Broadcast, "GetAssetIdByPath", material_asset_path, math.Uuid(), False)
    ComponentTests(
        "Decal (Atom)", lambda entity_obj: verify_set_property(
            entity_obj, "Controller|Configuration|Material", material_asset))

    # Directional Light Component
    ComponentTests(
        "Directional Light",
        lambda entity_obj: verify_set_property(
            entity_obj, "Controller|Configuration|Shadow|Camera", camera_entity.id))

    # Exposure Control Component
    ComponentTests(
        "Exposure Control", lambda entity_obj: verify_required_component_addition(
            entity_obj, ["PostFX Layer"], "Exposure Control"))

    # Global Skylight (IBL) Component
    diffuse_image_path = os.path.join("LightingPresets", "greenwich_park_02_4k_iblskyboxcm.exr.streamingimage")
    diffuse_image_asset = asset.AssetCatalogRequestBus(
        bus.Broadcast, "GetAssetIdByPath", diffuse_image_path, math.Uuid(), False)
    specular_image_path = os.path.join("LightingPresets", "greenwich_park_02_4k_iblskyboxcm.exr.streamingimage")
    specular_image_asset = asset.AssetCatalogRequestBus(
        bus.Broadcast, "GetAssetIdByPath", specular_image_path, math.Uuid(), False)
    ComponentTests(
        "Global Skylight (IBL)",
        lambda entity_obj: verify_set_property(
            entity_obj, "Controller|Configuration|Diffuse Image", diffuse_image_asset),
        lambda entity_obj: verify_set_property(
            entity_obj, "Controller|Configuration|Specular Image", specular_image_asset))

    # Physical Sky Component
    ComponentTests("Physical Sky")

    # PostFX Layer Component
    ComponentTests("PostFX Layer")

    # PostFX Radius Weight Modifier Component
    ComponentTests("PostFX Radius Weight Modifier")

    # Light Component
    ComponentTests("Light")

    # Display Mapper Component
    ComponentTests("Display Mapper")


if __name__ == "__main__":
    run()
