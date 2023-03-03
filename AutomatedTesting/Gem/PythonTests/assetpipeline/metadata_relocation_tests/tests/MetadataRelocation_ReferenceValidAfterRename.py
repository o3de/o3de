"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# Test Case Title : Check that renaming a material maintains references

class Results:
    material = ("Correct material", "Incorrect material")


def MetadataRelocation_ReferenceValidAfterRename():
    # Description: This test checks that renaming an asset and its metadata retains the same UUID and does not break
    # existing references.  This specifically tests the material component

    # Import report and test helper utilities
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    # All exposed python bindings are in azlmbr
    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.legacy.general as general
    import azlmbr.globals as globals
    import azlmbr.default
    import azlmbr.render
    import azlmbr.asset

    # Required for automated tests
    helper.init_idle()

    # Open the level called "MetadataTest".
    # This level contains a test entity with a reference to the material
    helper.open_level("", "MetadataTest")

    bunny_entity = general.find_editor_entity("Bunny")

    base_color = azlmbr.render.MaterialComponentRequestBus(bus.Event, "GetPropertyValueColor",
                                                           bunny_entity, azlmbr.render.DefaultMaterialAssignmentId,
                                                           "baseColor.color")
    color_code = base_color.ToU32()
    # This is a specific color set on the material assigned to the bunny
    expected_color_code = 4289747234
    is_correct_asset = color_code == expected_color_code

    Report.result(Results.material, is_correct_asset)


if __name__ == "__main__":
    # This utility starts up the test and sets up the state for knowing what test is currently being run
    from editor_python_test_tools.utils import Report

    Report.start_test(MetadataRelocation_ReferenceValidAfterRename)
