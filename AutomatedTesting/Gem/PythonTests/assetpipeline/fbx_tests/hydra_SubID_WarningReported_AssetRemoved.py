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

    import os
    import shutil
    import time
    from pathlib import Path

    from EditorPythonTestTools.editor_python_test_tools.utils import Report, TestHelper, Tracer

    # Test Setup: Wait for Editor idle before loading the level and executing Python hydra scripts.
    TestHelper.init_idle()
    TestHelper.open_level("AssetPipeline", "SceneTests")

    # Test Setup: Set up source and destination for assetinfo file, then verify there is no file there already
    dirpath = Path(__file__).parents[4]
    src = os.path.join(dirpath, 'Objects', 'ShaderBall_simple', 'shaderball_simple_NoMesh_NoID.fbx.assetinfo')
    dst = os.path.join(dirpath, 'Objects', 'shaderball_simple.fbx.assetinfo')
    if os.path.exists(dst):
        os.remove(dst)
        time.sleep(5)  # Allow enough time for AP to pick up and process the test asset, replace with AP Idle check once bus is exposed to python bindings.

    # Test Start:
    # 1. Copy an assetinfo file to change scene output by removing all meshes
    shutil.copyfile(src, dst)
    time.sleep(5)  # Replace with AP Idle check once bus is exposed to python bindings.

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
    time.sleep(5)  # Replace with AP Idle check once bus is exposed to python bindings.

    # 6. Report the test results gathered and end the test.
    Report.critical_result(Tests.asset_id_missing_warning_reported, warning_reported)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(SubID_WarningReported_AssetRemoved)
