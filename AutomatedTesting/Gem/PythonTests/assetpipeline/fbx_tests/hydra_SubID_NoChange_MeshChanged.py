"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import assetpipeline.ap_fixtures.ap_idle_fixture


class Tests:
    asset_id_no_change = (
        "Model AssetId matches after changes",
        "P0: Model AssetId does not match expected output")

def SubID_NoChange_MeshChanged():
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
    from Atom.atom_utils.atom_constants import AtomComponentProperties
    from EditorPythonTestTools.editor_python_test_tools.editor_entity_utils import EditorEntity
    from EditorPythonTestTools.editor_python_test_tools.utils import Report, TestHelper, Tracer
    from EditorPythonTestTools.editor_python_test_tools.asset_utils import Asset
    from assetpipeline.ap_fixtures.check_model_ready_fixture import OnModelReloaded


    with Tracer() as error_tracer:
        # -- Test Setup Begins --
        # Test Setup: Wait for Editor idle before loading the level and executing python hydra scripts.
        TestHelper.init_idle()
        TestHelper.open_level("AssetPipeline", "SceneTests")

        # Test Setup: Set source and destination paths for assetinfo file.
        dirpath = editor.EditorToolsApplicationRequestBus(azlmbr.bus.Broadcast, 'GetGameFolder')
        src = os.path.join(dirpath, 'Objects', 'ShaderBall_simple', 'shaderball_simple_MeshChange_SameID.fbx.assetinfo')
        dst = os.path.join(dirpath, 'Objects', 'shaderball_simple.fbx.assetinfo')

        # Test Setup: Find the asset by asset path
        model_path = os.path.join('objects', 'shaderball_simple.fbx.azmodel')
        model = Asset.find_asset_by_path(model_path)

        # Test Setup: Ensure there is no assetinfo file in the dst path, if there is, remove it.
        checkModel = OnModelReloaded()
        if os.path.exists(dst):
            os.remove(dst)
            checkModel.wait_for_on_model_reloaded(model.id)

        # Test Setup: Find the entity, and it's mesh component, within the level.
        find_entity = EditorEntity.find_editor_entity(AtomComponentProperties.mesh())
        find_component = find_entity.get_components_of_type([AtomComponentProperties.mesh()])[0]

        #Test Setup: Record the initial values for the Model Asset and the Vertex Count of LOD0.
        original_component_id = find_component.get_component_property_value(AtomComponentProperties.mesh('Model Asset'))
        original_vert_count = find_component.get_component_property_value(AtomComponentProperties.mesh('Vertex Count LOD0'))

        # -- Test Begins --
        # 1. Copy an assetinfo file to change scene output.
        shutil.copyfile(src, dst)
        checkModel.wait_for_on_model_reloaded(model.id)

        # 2. Reload the level to reflect changes.
        TestHelper.open_level("", "Base")
        time.sleep(0.2)
        TestHelper.open_level("AssetPipeline", "SceneTests")
        time.sleep(0.2)

        # 3. Record the current values for the Model Asset and the Vertex Count of LOD0.
        current_component_id = find_component.get_component_property_value(AtomComponentProperties.mesh('Model Asset'))
        current_vert_count = find_component.get_component_property_value(AtomComponentProperties.mesh('Vertex Count LOD0'))
        result = current_component_id == original_component_id and original_vert_count != current_vert_count

        # 4. Look for errors or asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")

        # 5. Clean-up.
        os.remove(dst)
        checkModel.wait_for_on_model_reloaded(model.id)

        # 6. Report the test results gathered and end the test.
        Report.critical_result(Tests.asset_id_no_change, result)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(SubID_NoChange_MeshChanged)
