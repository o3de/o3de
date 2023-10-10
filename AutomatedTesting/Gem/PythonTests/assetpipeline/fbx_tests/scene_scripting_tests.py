#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#


import pytest, logging, os

# Import fixtures
from ..ap_fixtures.asset_processor_fixture import asset_processor as asset_processor
from ..ap_fixtures.ap_setup_fixture import ap_setup_fixture as ap_setup_fixture

# Import LyShared
import ly_test_tools.o3de.pipeline_utils as utils

# Use the following logging pattern to hook all test logging together:
logger = logging.getLogger(__name__)

@pytest.fixture
def local_resources(request, workspace, ap_setup_fixture):
    ap_setup_fixture["tests_dir"] = os.path.dirname(os.path.realpath(__file__))

@pytest.mark.usefixtures("asset_processor")
@pytest.mark.usefixtures("ap_setup_fixture")
@pytest.mark.usefixtures("local_resources")
@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.SUITE_sandbox
class TestsSceneScripting(object):

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

    def test_DefaultBuilderScript_ScriptOnlyRunsOnExpectedAsset(self, workspace, ap_setup_fixture, asset_processor):
        # the AutomatedTesting/Gem/PythonTests/assetpipeline/fbx_tests/assets/default_script/default_script.py script exports
        # an additional product asset named 'cube_icosphere.fbx.default_script' in the temp asset test folder, so if it is found
        # then the default scene script file was executed as expected
        test_assets_source_folder, _ = asset_processor.prepare_test_environment(ap_setup_fixture["tests_dir"], "default_script")
        script_path = os.path.join(test_assets_source_folder, "default_script.py")
        extra_args = self.asset_processor_extra_params
        extra_args.append(f"--regset=\"/O3DE/AssetProcessor/SceneBuilder/defaultScripts/default_script*={script_path}\"")

        result, _ = asset_processor.batch_process(extra_params=extra_args)
        assert result, f"AP Batch failed with {extra_args}"

        expected_product_list = [
            'cube_icosphere.fbx.dbgsg',
            'cube_icosphere.fbx.default_script'
        ]

        missing_assets, existing_assets = utils.compare_assets_with_cache(expected_product_list, asset_processor.project_test_cache_folder())
        assert not missing_assets, f'The following assets were expected to be in, but not found in cache: {missing_assets} + {existing_assets}'

