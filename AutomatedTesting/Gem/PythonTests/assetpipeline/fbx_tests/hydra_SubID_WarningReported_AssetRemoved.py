"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:

    asset_id_missing_warning_reported = (
        "Successfully reported missing asset",
        "P0: Expected warning not reported from missing asset")

    asset_id_no_match = (
        "Missing asset successfully reports no matching asset id",
        "P0: Asset id matched for a missing asset")


def SubID_WarningReported_AssetRemoved():
    """
    Summary:
    Opens a level with an entity containing a mesh component.
    Verify updating a scene file where the product's asset subid remains the same still updates the mesh component.

    Expected Behavior:
    The updated asset's subid remains the same while the mesh referenced in the mesh component updates.

    Test Steps:
     1) Load the level
     2) Start the Tracer to catch any errors and warnings
     3) Record the original asset's subid and vert count
     4) Add a scene settings file to update the scene products
     5) Reload the level
     6) Record the updated asset's subid and vert count
     7) Verify there are no errors and warnings in the logs
     8) Verify the subids match while the vert counts do not
     9) Close the editor

    :return: None
    """

    import os
    import shutil
    import time
    from pathlib import Path

    import azlmbr.bus
    import azlmbr.editor as editor
    from assetpipeline.ap_fixtures.check_model_ready_fixture import OnModelReloaded
    from EditorPythonTestTools.editor_python_test_tools.utils import Report, TestHelper, Tracer
    from EditorPythonTestTools.editor_python_test_tools.asset_utils import Asset

    # -- Test Setup Begins --
    # Test Setup: Wait for Editor idle before loading the level and executing Python hydra scripts.
    TestHelper.init_idle()
    TestHelper.open_level("AssetPipeline", "SceneTests")

    # Test Setup: Set up source and destination for assetinfo file, then verify there is no file there already
    dirpath = editor.EditorToolsApplicationRequestBus(azlmbr.bus.Broadcast, 'GetGameFolder')
    src = os.path.join(dirpath, 'Objects', 'ShaderBall_simple', 'shaderball_simple_NoMesh_NoID.fbx.assetinfo')
    dst = os.path.join(dirpath, 'Objects', 'shaderball_simple.fbx.assetinfo')

    # Test Setup: Find the asset by asset path - Use azmaterial as we will be removing the azmodels during the test.
    model_path = os.path.join('objects', 'shaderball_simple_phong_0_17699592688871882463.fbx.azmaterial')
    model = Asset.find_asset_by_path(model_path)
    checkModel = OnModelReloaded()

    # Test Setup: Ensure there is no assetinfo file in the dst path, if there is, remove it.
    if os.path.exists(dst):
        os.remove(dst)
        checkModel.wait_for_on_model_reloaded(model.id)

    # -- Test Begins --
    # 1. Copy an assetinfo file to change scene output by removing all meshes
    shutil.copyfile(src, dst)
    checkModel.wait_for_on_model_reloaded(model.id)

    # 2. Start the Tracer to catch any errors and warnings
    with Tracer() as error_tracer:
        # 2.1 Reload the current level causing the Editor to attempt to reload the now missing entity.
        TestHelper.open_level("", "Base")
        time.sleep(0.2)
        TestHelper.open_level("AssetPipeline", "SceneTests")
        time.sleep(0.2)

    # 3. Search for the specific warnings we are expecting.
    warning_reported = False
    warning = "[AssetManager] GetAsset called for asset which does not exist in asset catalog and cannot be loaded. " \
              "Asset may be missing, not processed or moved. AssetId: {AA31FE94-4C6A-5355-B040-C1795BAED142}:10201c91"
    for warning_msg in error_tracer.warnings:
        if warning in str(warning_msg):
            warning_reported = True

    # 4. Look for other errors or asserts.
    TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
    for error_info in error_tracer.errors:
        Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
    for assert_info in error_tracer.asserts:
        Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")

    # 5. Clean-up
    os.remove(dst)
    checkModel.wait_for_on_model_reloaded(model.id)

    # 6. Report the test results gathered and end the test.
    Report.critical_result(Tests.asset_id_missing_warning_reported, warning_reported)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(SubID_WarningReported_AssetRemoved)
