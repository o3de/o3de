"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    tbd = (
        "tbd",
        "tbd")


def AtomGPU_LightComponent_SpotLightScreenshotsMatchGoldenImages():
    """
    Summary:
    Light component test using the Spot (disk) Light type property option and modifying the shadows and colors.
    Sets each scene up and then takes a screenshot of each scene for test comparison.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.
    - Close error windows and display helpers then update the viewport size.
    - Runs the create_basic_atom_rendering_scene() function to setup the test scene.

    Expected Behavior:
    The test scripts sets up the scenes correctly and takes accurate screenshots.

    Test Steps:
    1. Add Directional Light component to the existing Directional Light entity.
    2.
    :return: None
    """

    import azlmbr.legacy.general as general
    import azlmbr.paths

    from editor_python_test_tools.utils import Report, Tracer, TestHelper

    from Atom.atom_utils.atom_component_helper import initial_viewport_setup, create_basic_atom_rendering_scene

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("", "Base")

        # Setup: Close error windows and display helpers then update the viewport size.
        TestHelper.close_error_windows()
        TestHelper.close_display_helpers()
        initial_viewport_setup()
        general.update_viewport()

        # Setup: Runs the create_basic_atom_rendering_scene() function to setup the test scene.
        create_basic_atom_rendering_scene()

        # Test steps begin.
        # 1.

    # ???. Look for errors.
    TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
    for error_info in error_tracer.errors:
        Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
    for assert_info in error_tracer.asserts:
        Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AtomGPU_LightComponent_SpotLightScreenshotsMatchGoldenImages)
