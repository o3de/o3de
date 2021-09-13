"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Test case ID : C15096740
Test Case Title : Verify that clearing a material library on all systems that use it,
                  assigns the default material library

"""


# fmt: off
class Tests():
    create_entity             = ("Entity created successfully",             "Failed to create Entity")
    add_physx_component       = ("PhysX Component added successfully",      "Failed to add PhysX Component")
    override_default_library  = ("Material library overrided successfully", "Failed to override material library")
    update_to_default_library = ("Library updated to default",              "Failed to update library to default")
    new_library_updated       = ("New library updated successfully",        "Failed to add new library")
# fmt: on


def Material_LibraryClearingAssignsDefault():

    """
    Summary:
     Load level with Entity having PhysX Component. Override the material library to be the same one as the
     default material library. Change the default material library into another one.

    Expected Behavior:
     The material library gets updated correctly when the default material is changed.

    Test Steps:
     1) Load the level
     2) Create new Entity with PhysX Character Controller
     3) Override the material library to be the same one as the default material library
     4) Switch it back again to the default material library.
     5) Change the default material library into another one.
     6) Close the editor

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    # Built-in Imports
    import os


    # Helper file Imports
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.asset_utils import Asset

    # Open 3D Engine Imports
    import azlmbr.asset as azasset

    # Constants
    library_property_path = "Configuration|Physics Material|Library"
    
    default_material_path = os.path.join("assets", "physics", "surfacetypemateriallibrary.physmaterial")
    new_material_path = os.path.join("physicssurfaces", "default_phys_materials.physmaterial")

    helper.init_idle()
    # 1) Load the level
    helper.open_level("Physics", "Base")

    # 2) Create new Entity with PhysX Character Controller
    test_entity = EditorEntity.create_editor_entity("TestEntity")
    Report.result(Tests.create_entity, test_entity.id.IsValid())

    test_component = test_entity.add_component("PhysX Character Controller")
    Report.result(Tests.add_physx_component, test_entity.has_component("PhysX Character Controller"))

    # 3) Override the material library to be the same one as the default material library
    default_asset = Asset.find_asset_by_path(default_material_path)
    test_component.set_component_property_value(library_property_path, default_asset.id)
    default_asset.id = test_component.get_component_property_value(library_property_path)
    Report.result(Tests.override_default_library, default_asset.get_path() == default_material_path.replace(os.sep, '/'))

    # 4) Switch it back again to the default material library.
    test_component.set_component_property_value(library_property_path, azasset.AssetId())
    Report.result(
        Tests.update_to_default_library,
        test_component.get_component_property_value(library_property_path) == azasset.AssetId(),
    )

    # 5) Change the default material library into another one.
    new_asset = Asset.find_asset_by_path(new_material_path)
    test_component.set_component_property_value(library_property_path, new_asset.id)
    new_asset.id = test_component.get_component_property_value(library_property_path)
    Report.result(Tests.new_library_updated, new_asset.get_path() == new_material_path.replace(os.sep, '/'))



if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Material_LibraryClearingAssignsDefault)
