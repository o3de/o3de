"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    slice_created = (
        "Slice created successfully",
        "Failed to create slice"
    )
    slice_instantiated = (
        "Slice instantiated successfully",
        "Failed to instantiate slice"
    )


def Slice_CreateInstantiate():
    """
    Summary:
    A slice containing the LandscapeCanvas component can be created/instantiated.

    Expected Result:
    Slice is created/processed/instantiated successfully and free of errors/warnings.

    Test Steps:
     1) Open a simple level
     2) Create a new entity with a Landscape Canvas component
     3) Create a slice of the new entity
     4) Instantiate a new copy of the slice

     Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.
    :return: None
    """

    import os

    import azlmbr.math as math
    import azlmbr.bus as bus
    import azlmbr.asset as asset
    import azlmbr.slice as slice

    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    def path_is_valid_asset(asset_path):
        asset_id = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", asset_path, math.Uuid(), False)
        return asset_id.invoke("IsValid")

    # Open an existing simple level
    helper.init_idle()
    helper.open_level("Physics", "Base")

    # Create entity with Landscape Canvas component
    position = math.Vector3(512.0, 512.0, 32.0)
    landscape_canvas = hydra.Entity("landscape_canvas_entity")
    landscape_canvas.create_entity(position, ["Landscape Canvas"])

    # Create slice from the created entity
    slice_path = os.path.join("slices", "TestSlice.slice")
    slice.SliceRequestBus(bus.Broadcast, "CreateNewSlice", landscape_canvas.id, slice_path)

    # Verify if slice is created
    helper.wait_for_condition(lambda: path_is_valid_asset(slice_path), 5.0)
    Report.result(Tests.slice_created, path_is_valid_asset(slice_path))

    # Instantiate slice
    transform = math.Transform_CreateIdentity()
    asset_id = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", slice_path, math.Uuid(), False)
    test_slice = slice.SliceRequestBus(bus.Broadcast, "InstantiateSliceFromAssetId", asset_id, transform)
    helper.wait_for_condition(lambda: test_slice.IsValid(), 5.0)
    Report.result(Tests.slice_instantiated, test_slice.IsValid())


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(Slice_CreateInstantiate)
