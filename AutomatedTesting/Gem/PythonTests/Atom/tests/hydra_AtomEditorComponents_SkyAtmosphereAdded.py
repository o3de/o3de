"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    sky_atmosphere_creation = (
        "Sky AtmosphereEntity successfully created",
        "P0: Sky AtmosphereEntity failed to be created")
    sky_atmosphere_component = (
        "Entity has a Sky Atmosphere component",
        "P0: Entity failed to find Sky Atmosphere component")
    sky_atmosphere_component_removal = (
        "Sky Atmosphere component successfully removed",
        "P0: Sky Atmosphere component failed to be removed")
    removal_undo = (
        "UNDO Sky Atmosphere component removal success",
        "P0: UNDO Sky Atmosphere component removal failed")
    sky_atmosphere_disabled = (
        "Sky Atmosphere component disabled",
        "P0: Sky Atmosphere component was not disabled")
    sky_atmosphere_enabled = (
        "Sky Atmosphere component enabled",
        "P0: Sky Atmosphere component was not enabled")
    ground_albedo = (
        "Ground albedo property set",
        "P1: 'Ground albedo' property failed to set")
    ground_radius = (
        "Ground radius property set",
        "P1: 'Ground radius' property failed to set")
    atmosphere_height = (
        "Atmosphere height property set",
        "P1: 'Atmosphere height' property failed to set")
    illuminance_factor = (
        "Illuminance factor property set",
        "P1: 'Illuminance factor' property failed to set")
    mie_absorption_scale = (
        "Mie absorption Scale property set",
        "P1: 'Mie absorption Scale' property failed to set")
    mie_absorption = (
        "Mie absorption property set",
        "P1: 'Mie absorption' property failed to set")
    mie_exponential_distribution = (
        "Mie exponential distribution property set",
        "P1: 'Mie exponential distribution' property failed to set")
    mie_scattering_scale = (
        "Mie scattering Scale property set",
        "P1: 'Mie scattering Scale' property failed to set")
    mie_scattering = (
        "Mie scattering property set",
        "P1: 'Mie scattering' property failed to set")
    ozone_absorption_scale = (
        "Ozone Absorption Scale property set",
        "P1: 'Ozone Absorption Scale' property failed to set")
    ozone_absorption = (
        "Ozone Absorption property set",
        "P1: 'Ozone Absorption' property failed to set")
    rayleigh_exponential_distribution = (
        "Rayleigh exponential distribution property set",
        "P1: 'Rayleigh exponential distribution' property failed to set")
    rayleigh_scattering_scale = (
        "Rayleigh scattering Scale property set",
        "P1: 'Rayleigh scattering Scale' property failed to set")
    rayleigh_scattering = (
        "Rayleigh scattering property set",
        "P1: 'Rayleigh scattering' property failed to set")
    show_sun = (
        "Show sun property set",
        "P1: 'Show sun' property failed to set")
    sun_color = (
        "Sun color property set",
        "P1: 'Sun color' property failed to set")
    sun_falloff_factor = (
        "Sun falloff factor property set",
        "P1: 'Sun falloff factor' property failed to set")
    sun_limb_color = (
        "Sun limb color property set",
        "P1: 'Sun limb color' property failed to set")
    sun_luminance_factor = (
        "Sun luminance factor property set",
        "P1: 'Sun luminance factor' property failed to set")
    sun_orientation = (
        "Sun orientation property set",
        "P1: 'Sun orientation' property failed to set")
    sun_radius_factor = (
        "Sun radius factor property set",
        "P1: 'Sun radius factor' property failed to set")
    enable_shadows = (
        "Enable shadows property set",
        "P1: 'Enable shadows' property failed to set")
    fast_sky = (
        "Fast sky property set",
        "P1: 'Fast sky' property failed to set")
    max_samples = (
        "Max samples property set",
        "P1: 'Max samples' property failed to set")
    min_samples = (
        "Min samples property set",
        "P1: 'Min samples' property failed to set")
    near_clip = (
        "Near Clip property set",
        "P1: 'Near Clip' property failed to set")
    near_fade_distance = (
        "Near Fade Distance property set",
        "P1: 'Near Fade Distance' property failed to set")
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


def AtomEditorComponents_SkyAtmosphere_AddedToEntity():
    """
    Summary:
    Tests the Sky Atmosphere component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The component can be added, used in game mode, hidden/shown, deleted, and has accurate required components.
    Creation and deletion undo/redo should also work.

    Test Steps:
    1) Create an Sky Atmosphere entity with no components.
    2) Add Sky Atmosphere component to Sky Atmosphere entity.
    3) Remove the Sky Atmosphere component.
    4) Undo Sky Atmosphere component removal.
    5) Verify Sky Atmosphere component is enabled.
    6) Set a value for 'Ground albedo' property
    7) Set a value for 'Ground radius' property
    8) Set a value for 'Origin' property
    9) Set a value for 'Atmosphere height' property
    10) Set a value for 'Illuminance factor' property
    11) Set a value for 'Mie absorption Scale' property
    12) Set a value for 'Mie absorption' property
    13) Set a value for 'Mie exponential distribution' property
    14) Set a value for 'Mie scattering Scale' property
    15) Set a value for 'Mie scattering' property
    16) Set a value for 'Ozone Absorption Scale' property
    17) Set a value for 'Ozone Absorption' property
    18) Set a value for 'Rayleigh exponential distribution' property
    19) Set a value for 'Rayleigh scattering Scale' property
    20) Set a value for 'Rayleigh scattering' property
    21) Set a value for 'Show sun' property
    22) Set a value for 'Sun color' property
    23) Set a value for 'Sun falloff factor' property
    24) Set a value for 'Sun limb color' property
    25) Set a value for 'Sun luminance factor' property
    26) Set a value for 'Sun orientation' property
    27) Set a value for 'Sun radius factor' property
    28) Set a value for 'Enable shadows' property
    29) Set a value for 'Fast sky' property
    30) Set a value for 'Max samples' property
    31) Set a value for 'Min samples' property
    32) Set a value for 'Near Clip' property
    33) Set a value for 'Near Fade Distance' property
    34) Enter/Exit game mode.
    35) Test IsHidden.
    36) Test IsVisible.
    37) Delete Sky Atmosphere entity.
    38) UNDO deletion.
    39) REDO deletion.
    40) Look for errors and asserts.

    :return: None
    """

    import azlmbr.legacy.general as general

    from azlmbr.math import Vector3, Color

    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.wait_utils import PrefabWaiter
    from editor_python_test_tools.utils import Report, Tracer, TestHelper
    from Atom.atom_utils.atom_constants import AtomComponentProperties, ATMOSPHERE_ORIGIN

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("Graphics", "base_empty")

        # Test steps begin.
        # 1. Create an Sky Atmosphere entity with no components.
        sky_atmosphere_entity = EditorEntity.create_editor_entity(AtomComponentProperties.sky_atmosphere())
        Report.critical_result(Tests.sky_atmosphere_creation, sky_atmosphere_entity.exists())

        # 2. Add Sky Atmosphere component to Sky Atmosphere entity.
        sky_atmosphere_component = sky_atmosphere_entity.add_component(AtomComponentProperties.sky_atmosphere())
        Report.critical_result(Tests.sky_atmosphere_component,
                               sky_atmosphere_entity.has_component(AtomComponentProperties.sky_atmosphere()))

        # 3. Remove the Sky Atmosphere component.
        sky_atmosphere_component.remove()
        PrefabWaiter.wait_for_propagation()
        Report.critical_result(Tests.sky_atmosphere_component_removal,
                               not sky_atmosphere_entity.has_component(AtomComponentProperties.sky_atmosphere()))

        # 4. Undo Sky Atmosphere component removal.
        general.undo()
        PrefabWaiter.wait_for_propagation()
        Report.result(Tests.removal_undo,
                      sky_atmosphere_entity.has_component(AtomComponentProperties.sky_atmosphere()))

        # 5. Verify Sky Atmosphere component is enabled.
        Report.result(Tests.sky_atmosphere_enabled, sky_atmosphere_component.is_enabled())

        # 6. Set a value for 'Ground albedo' property
        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Ground albedo'), Vector3(0.0, 1.0, 0.0))
        Report.result(
            Tests.ground_albedo,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Ground albedo')) == Vector3(0.0, 1.0, 0.0))

        # 7. Set a value for 'Ground radius' property
        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Ground radius'), 100000.0)
        Report.result(
            Tests.ground_radius,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Ground radius')) == 100000.0)

        # 8. Set a value for 'Origin' property
        for origin_type in ATMOSPHERE_ORIGIN:
            test_origin = (
                f"Origin property set to {origin_type}",
                f"P1: 'Origin' property failed to set {origin_type}")
            sky_atmosphere_component.set_component_property_value(
                AtomComponentProperties.sky_atmosphere('Origin'), ATMOSPHERE_ORIGIN[origin_type])
            Report.result(
                test_origin,
                sky_atmosphere_component.get_component_property_value(
                    AtomComponentProperties.sky_atmosphere('Origin')) == ATMOSPHERE_ORIGIN[origin_type])
        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Origin'), ATMOSPHERE_ORIGIN['GroundAtWorldOrigin'])

        # 9. Set a value for 'Atmosphere height' property
        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Atmosphere height'), 10000.0)
        Report.result(
            Tests.atmosphere_height,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Atmosphere height')) == 10000.0)

        # 10. Set a value for 'Illuminance factor' property
        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Illuminance factor'), Vector3(100.0,100.0,100.0))
        Report.result(
            Tests.illuminance_factor,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Illuminance factor')) == Vector3(100.0,100.0,100.0))

        # 11. Set a value for 'Mie absorption Scale' property
        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Mie absorption scale'), 1.0)
        Report.result(
            Tests.mie_absorption_scale,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Mie absorption scale')) == 1.0)

        # 12. Set a value for 'Mie absorption' property
        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Mie absorption'), Vector3(0.5,0.0,0.5))
        Report.result(
            Tests.mie_absorption,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Mie absorption')) == Vector3(0.5,0.0,0.5))

        # 13. Set a value for 'Mie exponential distribution' property
        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Mie exponential distribution'), 400.0)
        Report.result(
            Tests.mie_exponential_distribution,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Mie exponential distribution')) == 400.0)

        # 14. Set a value for 'Mie scattering Scale' property
        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Mie scattering scale'), 1.0)
        Report.result(
            Tests.mie_scattering_scale,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Mie scattering scale')) == 1.0)

        # 15. Set a value for 'Mie scattering' property
        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Mie scattering'), Vector3(1.0,0.0,0.0))
        Report.result(
            Tests.mie_scattering,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Mie scattering')) == Vector3(1.0,0.0,0.0))

        # 16. Set a value for 'Ozone Absorption Scale' property
        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Ozone absorption scale'), 1.0)
        Report.result(
            Tests.ozone_absorption_scale,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Ozone absorption scale')) == 1.0)

        # 17. Set a value for 'Ozone Absorption' property
        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Ozone absorption'), Vector3(0.5,0.5,0.0))
        Report.result(
            Tests.ozone_absorption,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Ozone absorption')) == Vector3(0.5,0.5,0.0))

        # 18. Set a value for 'Rayleigh exponential distribution' property
        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Rayleigh exponential distribution'), 400.0)
        Report.result(
            Tests.rayleigh_exponential_distribution,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Rayleigh exponential distribution')) == 400.0)

        # 19. Set a value for 'Rayleigh scattering Scale' property
        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Rayleigh scattering scale'), 1.0)
        Report.result(
            Tests.rayleigh_scattering_scale,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Rayleigh scattering scale')) == 1.0)

        # 20. Set a value for 'Rayleigh scattering' property
        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Rayleigh scattering'), Vector3(1.0,1.0,1.0))
        Report.result(
            Tests.rayleigh_scattering,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Rayleigh scattering')) == Vector3(1.0,1.0,1.0))

        # 21. Set a value for 'Show sun' property
        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Show sun'), False)
        Report.result(
            Tests.show_sun,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Show sun')) is False)

        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Show sun'), True)
        Report.result(
            Tests.show_sun,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Show sun')) is True)

        # 22. Set a value for 'Sun color' property
        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Sun color'), Color(0.0,0.0,0.0,255.0))
        Report.result(
            Tests.sun_color,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Sun color')) == Color(0.0,0.0,0.0,255.0))

        # 23. Set a value for 'Sun falloff factor' property
        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Sun falloff factor'), 200.0)
        Report.result(
            Tests.sun_falloff_factor,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Sun falloff factor')) == 200.0)

        # 24. Set a value for 'Sun limb color' property
        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Sun limb color'), Color(0.0,0.0,0.0,255.0))
        Report.result(
            Tests.sun_limb_color,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Sun limb color')) == Color(0.0,0.0,0.0,255.0))

        # 25. Set a value for 'Sun luminance factor' property
        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Sun luminance factor'), 100000.0)
        Report.result(
            Tests.sun_luminance_factor,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Sun luminance factor')) == 100000.0)

        # 26. Set a value for 'Sun orientation' property
        sun_entity = EditorEntity.create_editor_entity_at((10.0,0.0,0.0), name='Sun')
        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Sun orientation'), sun_entity.id)
        Report.result(
            Tests.sun_orientation,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Sun orientation')) == sun_entity.id)

        # 27. Set a value for 'Sun radius factor' property
        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Sun radius factor'), 100.0)
        Report.result(
            Tests.sun_radius_factor,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Sun radius factor')) == 100.0)

        # 28. Set a value for 'Enable shadows' property
        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Enable shadows'), True)
        Report.result(
            Tests.enable_shadows,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Enable shadows')) is True)

        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Enable shadows'), False)
        Report.result(
            Tests.enable_shadows,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Enable shadows')) is False)

        # 29. Set a value for 'Fast sky' property
        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Fast sky'), False)
        Report.result(
            Tests.fast_sky,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Fast sky')) is False)

        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Fast sky'), True)
        Report.result(
            Tests.fast_sky,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Fast sky')) is True)

        # 30. Set a value for 'Max samples' property
        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Max samples'), 64)
        Report.result(
            Tests.max_samples,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Max samples')) == 64)

        # 31. Set a value for 'Min samples' property
        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Min samples'), 64)
        Report.result(
            Tests.min_samples,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Min samples')) == 64)

        # 32. Set a value for 'Near Clip' property
        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Near clip'), 10.0)
        Report.result(
            Tests.near_clip,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Near clip')) == 10.0)

        # 33. Set a value for 'Near Fade Distance' property
        sky_atmosphere_component.set_component_property_value(
            AtomComponentProperties.sky_atmosphere('Near fade distance'), 20.0)
        Report.result(
            Tests.near_fade_distance,
            sky_atmosphere_component.get_component_property_value(
                AtomComponentProperties.sky_atmosphere('Near fade distance')) == 20.0)

        # 34. Enter/Exit game mode.
        TestHelper.enter_game_mode(Tests.enter_game_mode)
        PrefabWaiter.wait_for_propagation()
        TestHelper.exit_game_mode(Tests.exit_game_mode)

        # 35. Test IsHidden.
        sky_atmosphere_entity.set_visibility_state(False)
        Report.result(Tests.is_hidden, sky_atmosphere_entity.is_hidden() is True)

        # 36. Test IsVisible.
        sky_atmosphere_entity.set_visibility_state(True)
        PrefabWaiter.wait_for_propagation()
        Report.result(Tests.is_visible, sky_atmosphere_entity.is_visible() is True)

        # 37. Delete Sky Atmosphere entity.
        sky_atmosphere_entity.delete()
        PrefabWaiter.wait_for_propagation()
        Report.result(Tests.entity_deleted, not sky_atmosphere_entity.exists())

        # 38. UNDO deletion.
        general.undo()
        PrefabWaiter.wait_for_propagation()
        Report.result(Tests.deletion_undo, sky_atmosphere_entity.exists())

        # 39. REDO deletion.
        general.redo()
        PrefabWaiter.wait_for_propagation()
        Report.result(Tests.deletion_redo, not sky_atmosphere_entity.exists())

        # 40. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomEditorComponents_SkyAtmosphere_AddedToEntity)
