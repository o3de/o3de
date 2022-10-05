"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class Tests:
    asset_id_no_change = (
        "Model AssetId matches after changes",
        "P0: Model AssetId does not match expected output")

def SubID_NoChange_MeshChanged():

    import os
    import shutil
    import time
    from pathlib import Path

    from Atom.atom_utils.atom_constants import AtomComponentProperties
    from EditorPythonTestTools.editor_python_test_tools.editor_entity_utils import EditorEntity
    from EditorPythonTestTools.editor_python_test_tools.utils import Report, TestHelper, Tracer

    with Tracer() as error_tracer:
        # Test Setup: Wait for Editor idle before loading the level and executing Python hydra scripts.
        TestHelper.init_idle()
        TestHelper.open_level("AssetPipeline", "SceneTests")

        # Test Setup: Set up source and destination for assetinfo file, then verify there is no file there already.
        dirpath = Path(__file__).parents[4]
        src = os.path.join(dirpath, 'Objects', 'ShaderBall_simple', 'shaderball_simple_MeshChange_SameID.fbx.assetinfo')
        dst = os.path.join(dirpath, 'Objects', 'shaderball_simple.fbx.assetinfo')
        if os.path.exists(dst):
            os.remove(dst)
            time.sleep(5) # Allow enough time for AP to pick up and process the test asset, replace with APIdle check once bus is exposed to python bindings.

        # Test Setup: Ensure level has enough time to load before attempting to find the entity
        time.sleep(5)
        find_entity = EditorEntity.find_editor_entity(AtomComponentProperties.mesh())
        find_component = find_entity.get_components_of_type([AtomComponentProperties.mesh()])[0]
        original_component_id = find_component.get_component_property_value(AtomComponentProperties.mesh('Model Asset'))
        original_vert_count = find_component.get_component_property_value(AtomComponentProperties.mesh('Vertex Count LOD0'))

        # 1. Copy an assetinfo file to change scene output
        shutil.copyfile(src, dst)
        time.sleep(5)  # Replace with APIdle check once bus is exposed to python bindings.

        # 2. Reload the level to reflect changes
        TestHelper.open_level("", "Base")
        time.sleep(0.2)
        TestHelper.open_level("AssetPipeline", "SceneTests")
        time.sleep(0.2)

        # 3. Search the level for the expected entity, then record the component property results.
        find_entity = EditorEntity.find_editor_entity(AtomComponentProperties.mesh())
        find_component = find_entity.get_components_of_type([AtomComponentProperties.mesh()])[0]
        current_component_id = find_component.get_component_property_value(AtomComponentProperties.mesh('Model Asset'))
        current_vert_count = find_component.get_component_property_value(AtomComponentProperties.mesh('Vertex Count LOD0'))
        result = current_component_id == original_component_id and original_vert_count != current_vert_count

        # 4. Look for errors or asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")

        # 5. Clean-up
        os.remove(dst)
        time.sleep(5)  # Replace with APIdle check once bus is exposed to python bindings.

        # 6. Report the test results gathered and end the test.
        Report.critical_result(Tests.asset_id_no_change, result)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(SubID_NoChange_MeshChanged)
