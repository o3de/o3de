"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Import builtin libraries
import pytest
import logging
import os
import json

# Import fixtures
from ..ap_fixtures.asset_processor_fixture import asset_processor as asset_processor
from ..ap_fixtures.ap_setup_fixture import ap_setup_fixture as ap_setup_fixture

# Import LyShared
import ly_test_tools.o3de.pipeline_utils as utils

# Use the following logging pattern to hook all test logging together:
logger = logging.getLogger(__name__)
# Configuring the logging is done in ly_test_tools at the following location:
# ~/dev/Tools/LyTestTools/ly_test_tools/log/py_logging_util.py

@pytest.fixture
def local_resources(request, workspace, ap_setup_fixture):
    ap_setup_fixture["tests_dir"] = os.path.dirname(os.path.realpath(__file__))

@pytest.mark.usefixtures("asset_processor")
@pytest.mark.usefixtures("ap_setup_fixture")
@pytest.mark.usefixtures("local_resources")
@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.SUITE_main
class TestsPythonAssetProcessing_APBatch(object):   

    @property
    def asset_processor_extra_params(self):
        return [
        # Disabling Atom assets disables most products, using the debugOutput flag ensures one product is output.
        "--debugOutput",
        # By default, if job priorities are equal, jobs run in an arbitrary order. This makes sure
        # jobs are run by sorting on the database source name, so they run in the same order each time
        # when this test is run.
        "--sortJobsByDBSourceName",
        # Disabling Atom products means this asset won't need a lot of source dependencies to be processed,
        # keeping the scope of this test down.
        "--regset=\"/O3DE/SceneAPI/AssetImporter/SkipAtomOutput=true\"",
        # The bug this regression test happened when the same builder processed FBX files with and without Python.
        # This flag ensures that only one builder is launched, so that situation can be replicated.
        "--regset=\"/Amazon/AssetProcessor/Settings/Jobs/maxJobs=1\""]

    def test_ProcessAssetWithoutScriptAfterAssetWithScript_ScriptOnlyRunsOnExpectedAsset(self, workspace, ap_setup_fixture, asset_processor):
        # This is a regression test. The situation it's testing is, the Python script to run
        # defined in scene manifest files was persisting in a single builder. So if
        # that builder processed file a.fbx, then b.fbx, and a.fbx has a Python script to run,
        # it was also running that Python script on b.fbx.

        asset_processor.prepare_test_environment(ap_setup_fixture["tests_dir"], "TwoSceneFiles_OneWithPythonOneWithout_PythonOnlyRunsOnFirstScene")

        result, _ = asset_processor.batch_process(extra_params=self.asset_processor_extra_params)
        assert result, "AP Batch failed"

        expected_product_list = [
            "a_simple_box_with_script.fbx.dbgsg",
            "b_simple_box_no_script.fbx.dbgsg"
        ]

        missing_assets, _ = utils.compare_assets_with_cache(expected_product_list,
                                                asset_processor.project_test_cache_folder())
        assert not missing_assets, f'The following assets were expected to be in, but not found in cache: {str(missing_assets)}'

        # The Python script loaded in the scene manifest will write a log file with the source file's name
        # to the temp folder. This is the easiest way to have the internal Python there communicate with this test.
        expected_path = os.path.join(asset_processor.project_test_source_folder(), "a_simple_box_with_script_fbx.log")
        unexpected_path = os.path.join(asset_processor.project_test_source_folder(), "b_simple_box_no_script_fbx.log")
        
        # Simple check to make sure the Python script in the scene manifest ran on the file it should have ran on.
        assert os.path.exists(expected_path), f"Did not find expected output test asset {expected_path}"
        # If this test fails here, it means the Python script from the first processed FBX file is being run
        # on the second FBX file, when it should not be.
        assert not os.path.exists(unexpected_path), f"Found unexpected output test asset {unexpected_path}"

    def find_user_defined_property(self, filename: str, text: str):
        # find the user defined property pattern in a file

        with open(filename) as f:
            content = f.readlines()
            for line in content:
                if line.rstrip().endswith(text):
                    return True
        return False

    def compute_udp_asset_debug_file(self, workspace, asset_processor, debug_filename, ap_setup_fixture):
        asset_folder_name = 'UserDefinedProperties'
        return self.compute_asset_debug_file(workspace, asset_processor, debug_filename, asset_folder_name, ap_setup_fixture)

    def compute_asset_debug_file(self, workspace, asset_processor, debug_filename, folder_name, ap_setup_fixture):
        # computes the file name of the debug file for a UserDefinedProperties test file

        asset_processor.prepare_test_environment(ap_setup_fixture["tests_dir"], folder_name)        
        result, _ = asset_processor.batch_process(extra_params=self.asset_processor_extra_params)
        assert result, "AP Batch failed"

        # compute the cache path
        cache_folder = asset_processor.project_test_cache_folder()

        # compute the file name to the debug file
        asset_debug_filename = os.path.join(cache_folder, debug_filename)
        if os.path.isfile(asset_debug_filename) == False:
            raise Exception(f"Missing file {asset_debug_filename}")

        return asset_debug_filename

    def test_ProcessSceneWithMetadata_SupportedMayaDataTypes_Work(self, workspace, ap_setup_fixture, asset_processor):
        # This test loads the debug output file for an FBX exported by Maya that has a few user defined properties

        asset_dbgsg = 'maya_with_attributes.fbx.dbgsg'
        dbgsg_file = self.compute_udp_asset_debug_file(workspace, asset_processor, asset_dbgsg, ap_setup_fixture)
        assert self.find_user_defined_property(dbgsg_file, 'o3de_atom_lod: false'), "Malformed o3de_atom_lod value"
        assert self.find_user_defined_property(dbgsg_file, 'o3de_atom_material: 0'), "Malformed o3de_atom_material value"
        assert self.find_user_defined_property(dbgsg_file, 'o3de_default_lod: 0.000000'), "Malformed o3de_default_lod value"
        assert self.find_user_defined_property(dbgsg_file, 'o3de_default_material: gem/sponza/assets/objects/sponza_mat_bricks.azmaterial'), "Malformed o3de_default_material value"

    def test_ProcessSceneWithMetadata_SupportedMaxDataTypes_Work(self, workspace, ap_setup_fixture, asset_processor):
        # This test loads the debug output file for an FBX exported by Max that has a few user defined properties

        asset_dbgsg = 'max_with_attributes.fbx.dbgsg'
        dbgsg_file = self.compute_udp_asset_debug_file(workspace, asset_processor, asset_dbgsg, ap_setup_fixture)
        assert self.find_user_defined_property(dbgsg_file, 'o3de_atom_material: 0'), "Malformed o3de_atom_material value"
        assert self.find_user_defined_property(dbgsg_file, 'o3de_phyx_lodY: 0.000000'), "Malformed o3de_phyx_lodY value"
        assert self.find_user_defined_property(dbgsg_file, 'o3de_default_lod: 0.000000'), "Malformed o3de_default_lod value"
        assert self.find_user_defined_property(dbgsg_file, 'o3de_atom_lod: false'), "Malformed o3de_atom_lod value"
        assert self.find_user_defined_property(dbgsg_file, 'o3de_default_material: gem/sponza/assets/objects/sponza_mat_bricks.azmaterial'), "Malformed o3de_default_material value"

    def test_ProcessSceneWithMetadata_SupportedBlenderDataTypes_Work(self, workspace, ap_setup_fixture, asset_processor):
        # This test loads the debug output file for an FBX exported by Blender that has a few user defined properties

        asset_dbgsg = 'blender_with_attributes.fbx.dbgsg'
        dbgsg_file = self.compute_udp_asset_debug_file(workspace, asset_processor, asset_dbgsg, ap_setup_fixture)
        assert self.find_user_defined_property(dbgsg_file, 'o3de_atom_material: 0'), "Malformed o3de_atom_material value"
        assert self.find_user_defined_property(dbgsg_file, 'o3de_default_lod: 0.000000'), "Malformed o3de_default_lod value"
        assert self.find_user_defined_property(dbgsg_file, 'o3de_default_material: gem/sponza/assets/objects/sponza_mat_bricks.azmaterial'), "Malformed o3de_default_material value"
        assert self.find_user_defined_property(dbgsg_file, 'o3de_atom_lod: 0'), "Malformed o3de_atom_lod value"

    def test_ProcessSceneWithMetadata_DebugSceneManifest_Work(self, workspace, ap_setup_fixture, asset_processor):
        # This test detects the debug .assetinfo file in the cahce folder

        assetinfo_dbg = 'blender_with_attributes.fbx.assetinfo.dbg'
        assetinfo_dbg_file = self.compute_udp_asset_debug_file(workspace, asset_processor, assetinfo_dbg, ap_setup_fixture)
        assert assetinfo_dbg_file, "The debug assetinfo file is missing"

    def test_ProcessSceneWithCommonParent_DefaultMeshGroups_Work(self, workspace, ap_setup_fixture, asset_processor):
        # This test validates that the correct meshes are included and excluded in the default mesh groups
        # of a scene containing mesh nodes that have a common parent transform node

        assetinfo_dbg_list = {
            "blender_with_common_parent.fbx.assetinfo.dbg",
            "maya_with_common_parent.fbx.assetinfo.dbg"
        }

        expected_mesh_nodes = {
            "RootNode.AstroVAC v10.CPU:1.CPU1.CUPCase",
            "RootNode.AstroVAC v10.CPU:1.CPU1.CPUUserDoor",
            "RootNode.AstroVAC v10.CPU:1.CPU1.CPUCover"
        }

        for assetinfo_dbg in assetinfo_dbg_list:
            asset_folder_name = 'MeshGroups'
            assetinfo_dbg_file = self.compute_asset_debug_file(workspace, asset_processor, assetinfo_dbg, asset_folder_name, ap_setup_fixture)
            assert assetinfo_dbg_file, "The debug assetinfo file is missing"

            with open(assetinfo_dbg_file, 'r') as file :
                filedata = file.read()

            dgb_dict = json.loads(filedata)

            found_mesh_nodes = set()

            # Check that each default mesh group contains one of the expected mesh nodes in its selected
            # node list and has the rest of the expected mesh nodes in its unselected node list
            groups = dgb_dict['values']
            for group in groups:
                if 'MeshGroup' in group.get('$type', str()) and group.get('name', str()).startswith('default_'):
                    selection_lists = group.get('nodeSelectionList')
                    selected_node_list = selection_lists.get('selectedNodes', [])
                    assert len(selected_node_list) == 1, "Selected node list does not contain exactly one mesh"
                    current_set = expected_mesh_nodes.copy()
                    current_set.remove(selected_node_list[0])
                    unselected_node_list = selection_lists.get('unselectedNodes', [])
                    assert current_set == set(unselected_node_list), "Unselected node list does not contain the correct meshes"
                    found_mesh_nodes.add(selected_node_list[0])
            assert found_mesh_nodes == expected_mesh_nodes, "Not all expected mesh groups found"

    def test_ProcessAssetScript_SimpleFbx_HasExpectedProducts(self, workspace, ap_setup_fixture, asset_processor):
        # This tests all of the entry points a scene builder can script:
        # - OnUpdateManifest() returns a JSON string to use as the scene manifest; this must succeed to continue the scene pipe
        # - OnPrepareForExport() returns an export product list; this test should produce a simple.fbx.fake_asset export product

        asset_processor.prepare_test_environment(ap_setup_fixture["tests_dir"], "script_basics")

        result, _ = asset_processor.batch_process(extra_params=self.asset_processor_extra_params)
        assert result, "AP Batch failed"

        expected_product_list = [
            "simple.fbx.fake_asset"
        ]

        missing_assets, _ = utils.compare_assets_with_cache(expected_product_list, asset_processor.project_test_cache_folder())
        assert not missing_assets, f'The following assets were expected to be in, but not found in cache: {str(missing_assets)}'

