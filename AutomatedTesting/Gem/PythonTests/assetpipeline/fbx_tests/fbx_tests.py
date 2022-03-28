"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import binascii
from dataclasses import dataclass
import logging
import os
import pytest
from typing import List

# Import LyTestTools
import ly_test_tools.builtin.helpers as helpers
from ly_test_tools.o3de import asset_processor as asset_processor_utils
from ly_test_tools.o3de.asset_processor import ASSET_PROCESSOR_PLATFORM_MAP

# Import fixtures
from ..ap_fixtures.asset_processor_fixture import asset_processor as asset_processor
from ..ap_fixtures.ap_setup_fixture import ap_setup_fixture as ap_setup_fixture
from ..ap_fixtures.ap_config_backup_fixture import ap_config_backup_fixture as ap_config_backup_fixture
from ..ap_fixtures.ap_config_default_platform_fixture \
    import ap_config_default_platform_fixture as ap_config_default_platform_fixture

# Import LyShared
import ly_test_tools.o3de.pipeline_utils as utils
from automatedtesting_shared import asset_database_utils as asset_db_utils

logger = logging.getLogger(__name__)

# Helper: variables we will use for parameter values in the test:
targetProjects = ["AutomatedTesting"]


@pytest.fixture
@pytest.mark.SUITE_sandbox
def local_resources(request, workspace, ap_setup_fixture):
    ap_setup_fixture["tests_dir"] = os.path.dirname(os.path.realpath(__file__))


@dataclass
@pytest.mark.SUITE_sandbox
class BlackboxAssetTest:
    test_name: str
    asset_folder: str
    override_asset_folder: str = ""
    scene_debug_file: str = ""
    override_scene_debug_file: str = ""
    assets: List[asset_db_utils.DBSourceAsset] = ()
    override_assets: List[asset_db_utils.DBSourceAsset] = ()


blackbox_fbx_tests = [
    pytest.param(
        BlackboxAssetTest(
            test_name="OneMeshOneMaterial_RunAP_SuccessWithMatchingProducts",
            asset_folder="OneMeshOneMaterial",
            scene_debug_file="onemeshonematerial.dbgsg",
            assets=[
                asset_db_utils.DBSourceAsset(
                    source_file_name="OneMeshOneMaterial.fbx",
                    uuid=b"8a9164adb84859be893e18aa819438e1",
                    jobs=[
                        asset_db_utils.DBJob(
                            job_key="Scene compilation",
                            builder_guid=b"bd8bf65894854fe3830e8ec3a23c35f3",
                            status=4,
                            error_count=0,
                            warning_count=1,
                            products=[
                                asset_db_utils.DBProduct(
                                    product_name='onemeshonematerial/onemeshonematerial.dbgsg',
                                    sub_id=1918494907,
                                    asset_type=b'07f289d14dc74c4094b40a53bbcb9f0b'),
                                asset_db_utils.DBProduct(
                                    product_name='onemeshonematerial/onemeshonematerial.dbgsg.xml',
                                    sub_id=556355570,
                                    asset_type=b'51f376140d774f369ac67ed70a0ac868'
                                )
                            ]
                        ),
                    ]
                )
            ]
        ),
        id="32250017",
        marks=pytest.mark.test_case_id("C32250017"),
    ),
    pytest.param(
        BlackboxAssetTest(
            # Verifies that the soft naming convention feature with level of detail meshes works.
            # https://docs.aws.amazon.com/lumberyard/latest/userguide/char-fbx-importer-soft-naming.html
            test_name="SoftNamingLOD_RunAP_SuccessWithMatchingProducts",
            asset_folder="SoftNamingLOD",
            scene_debug_file="lodtest.dbgsg",
            assets=[
                asset_db_utils.DBSourceAsset(
                    source_file_name="lodtest.fbx",
                    uuid=b"44c8627fe2c25aae91fe3ff9547be3b9",
                    jobs=[
                        asset_db_utils.DBJob(
                            job_key="Scene compilation",
                            builder_guid=b"bd8bf65894854fe3830e8ec3a23c35f3",
                            status=4,
                            error_count=0,
                            warning_count=22,
                            products=[
                                asset_db_utils.DBProduct(
                                    product_name='softnaminglod/lodtest.dbgsg',
                                    sub_id=-632012261,
                                    asset_type=b'07f289d14dc74c4094b40a53bbcb9f0b'),
                                asset_db_utils.DBProduct(
                                    product_name='softnaminglod/lodtest.dbgsg.xml',
                                    sub_id=-2036095434,
                                    asset_type=b'51f376140d774f369ac67ed70a0ac868'
                                )
                            ]
                        ),
                    ]
                )
            ]
        ),
        id="33887870",
        marks=pytest.mark.test_case_id("33887870"),
    ),
    pytest.param(
        BlackboxAssetTest(
            # Verifies that the soft naming convention feature with physics proxies works.
            # https://docs.aws.amazon.com/lumberyard/latest/userguide/char-fbx-importer-soft-naming.html
            test_name="SoftNamingPhysics_RunAP_SuccessWithMatchingProducts",
            asset_folder="SoftNamingPhysics",
            scene_debug_file="physicstest.dbgsg",
            assets=[
                asset_db_utils.DBSourceAsset(
                    source_file_name="physicstest.fbx",
                    uuid=b"df957b7918cf5b029806c73f630fa1c8",
                    jobs=[
                        asset_db_utils.DBJob(
                            job_key="Scene compilation",
                            builder_guid=b"bd8bf65894854fe3830e8ec3a23c35f3",
                            status=4,
                            error_count=0,
                            warning_count=14,
                            products=[
                                asset_db_utils.DBProduct(
                                    product_name='softnamingphysics/physicstest.dbgsg',
                                    sub_id=-740411732,
                                    asset_type=b'07f289d14dc74c4094b40a53bbcb9f0b'
                                ),
                                asset_db_utils.DBProduct(
                                    product_name='softnamingphysics/physicstest.dbgsg.xml',
                                    sub_id=330338417,
                                    asset_type=b'51f376140d774f369ac67ed70a0ac868'
                                ),
                                asset_db_utils.DBProduct(
                                    product_name="softnamingphysics/physicstest.pxmesh",
                                    sub_id=640975857,
                                    asset_type=b"7a2871b95eab4de0a901b0d2c6920ddb"
                                )
                            ]
                        ),
                    ]
                )
            ]
        ),
    ),
    pytest.param(
        BlackboxAssetTest(
            test_name="MultipleMeshOneMaterial_RunAP_SuccessWithMatchingProducts",
            asset_folder="TwoMeshOneMaterial",
            scene_debug_file="multiple_mesh_one_material.dbgsg",
            assets=[
                asset_db_utils.DBSourceAsset(
                    source_file_name="multiple_mesh_one_material.fbx",
                    uuid=b"597618fd497659a1b197a015fe47aa95",
                    jobs=[
                        asset_db_utils.DBJob(
                            job_key="Scene compilation",
                            builder_guid=b"bd8bf65894854fe3830e8ec3a23c35f3",
                            status=4,
                            error_count=0,
                            warning_count=2,
                            products=[
                                asset_db_utils.DBProduct(
                                    product_name='twomeshonematerial/multiple_mesh_one_material.dbgsg',
                                    sub_id=2077268018,
                                    asset_type=b'07f289d14dc74c4094b40a53bbcb9f0b'),
                                asset_db_utils.DBProduct(
                                    product_name='twomeshonematerial/multiple_mesh_one_material.dbgsg.xml',
                                    sub_id=1321067730,
                                    asset_type=b'51f376140d774f369ac67ed70a0ac868')
                            ]
                        ),
                    ]
                )
            ]
        ),
        id="34448178",
        marks=pytest.mark.test_case_id("C34448178"),
    ),
    pytest.param(
        BlackboxAssetTest(
            # Verifies whether multiple meshes can share linked materials
            test_name="MultipleMeshLinkedMaterials_RunAP_SuccessWithMatchingProducts",
            asset_folder="TwoMeshLinkedMaterials",
            scene_debug_file="multiple_mesh_linked_materials.dbgsg",
            assets=[
                asset_db_utils.DBSourceAsset(
                    source_file_name="multiple_mesh_linked_materials.fbx",
                    uuid=b"25d8301c2eef5dc7bded310db8ea608d",
                    jobs=[
                        asset_db_utils.DBJob(
                            job_key="Scene compilation",
                            platform="pc",
                            builder_guid=b"bd8bf65894854fe3830e8ec3a23c35f3",
                            status=4,
                            error_count=0,
                            warning_count=2,
                            products=[
                                asset_db_utils.DBProduct(
                                    product_name='twomeshlinkedmaterials/multiple_mesh_linked_materials.dbgsg',
                                    sub_id=-1898461950,
                                    asset_type=b'07f289d14dc74c4094b40a53bbcb9f0b'
                                ),
                                asset_db_utils.DBProduct(
                                    product_name='twomeshlinkedmaterials/multiple_mesh_linked_materials.dbgsg.xml',
                                    sub_id=-772341513,
                                    asset_type=b'51f376140d774f369ac67ed70a0ac868'
                                )
                            ]
                        ),
                    ]
                )
            ]
        ),
        id="32250018",
        marks=pytest.mark.test_case_id("C32250018"),
    ),
    pytest.param(
        BlackboxAssetTest(
            # Verifies a mesh with multiple materials
            test_name="SingleMeshMultipleMaterials_RunAP_SuccessWithMatchingProducts",
            asset_folder="OneMeshMultipleMaterials",
            scene_debug_file="single_mesh_multiple_materials.dbgsg",
            assets=[
                asset_db_utils.DBSourceAsset(
                    source_file_name="single_mesh_multiple_materials.fbx",
                    uuid=b"f08fd585dfa35881b4bf86637da5e858",
                    jobs=[
                        asset_db_utils.DBJob(
                            job_key="Scene compilation",
                            platform="pc",
                            builder_guid=b"bd8bf65894854fe3830e8ec3a23c35f3",
                            status=4,
                            error_count=0,
                            warning_count=1,
                            products=[
                                asset_db_utils.DBProduct(
                                    product_name='onemeshmultiplematerials/single_mesh_multiple_materials.dbgsg',
                                    sub_id=-262822238,
                                    asset_type=b'07f289d14dc74c4094b40a53bbcb9f0b'),
                                asset_db_utils.DBProduct(
                                    product_name='onemeshmultiplematerials/single_mesh_multiple_materials.dbgsg.xml',
                                    sub_id=1462358160,
                                    asset_type=b'51f376140d774f369ac67ed70a0ac868'
                                )
                            ]
                        ),
                    ]
                )
            ]
        ),
        id="32250020",
        marks=pytest.mark.test_case_id("C32250020"),
    ),
    pytest.param(
        BlackboxAssetTest(
            test_name="MeshWithVertexColors_RunAP_SuccessWithMatchingProducts",
            asset_folder="VertexColor",
            scene_debug_file="vertexcolor.dbgsg",
            assets=[
                asset_db_utils.DBSourceAsset(
                    source_file_name="VertexColor.fbx",
                    uuid=b"207e7e1540785a26b064e9be67361cdf",
                    jobs=[
                        asset_db_utils.DBJob(
                            job_key="Scene compilation",
                            builder_guid=b"bd8bf65894854fe3830e8ec3a23c35f3",
                            status=4,
                            error_count=0,
                            warning_count=1,
                            products=[
                                asset_db_utils.DBProduct(
                                    product_name='vertexcolor/vertexcolor.dbgsg',
                                    sub_id=-1543877170,
                                    asset_type=b'07f289d14dc74c4094b40a53bbcb9f0b'),
                                asset_db_utils.DBProduct(
                                    product_name='vertexcolor/vertexcolor.dbgsg.xml',
                                    sub_id=1743516586,
                                    asset_type=b'51f376140d774f369ac67ed70a0ac868'
                                )
                            ]
                        ),
                    ]
                )
            ]
        ),
        id="35796285",
        marks=pytest.mark.test_case_id("C35796285"),
    ),
    pytest.param(
        BlackboxAssetTest(
            test_name="MotionTest_RunAP_SuccessWithMatchingProducts",
            asset_folder="Motion",
            scene_debug_file="Jack_Idle_Aim_ZUp.dbgsg",
            assets=[
                asset_db_utils.DBSourceAsset(
                    source_file_name="Jack_Idle_Aim_ZUp.fbx",
                    uuid=b"eda904ae0e145f8b973d57fc5809918b",
                    jobs=[
                        asset_db_utils.DBJob(
                            job_key="Scene compilation",
                            builder_guid=b"bd8bf65894854fe3830e8ec3a23c35f3",
                            status=4,
                            error_count=0,
                            warning_count=0,
                            products=[
                                asset_db_utils.DBProduct(
                                    product_name='motion/jack_idle_aim_zup.dbgsg',
                                    sub_id=-517610290,
                                    asset_type=b'07f289d14dc74c4094b40a53bbcb9f0b'),
                                asset_db_utils.DBProduct(
                                    product_name='motion/jack_idle_aim_zup.dbgsg.xml',
                                    sub_id=-817863914,
                                    asset_type=b'51f376140d774f369ac67ed70a0ac868'
                                ),
                                asset_db_utils.DBProduct(
                                    product_name='motion/jack_idle_aim_zup.motion',
                                    sub_id=186392073,
                                    asset_type=b'00494b8e75784ba28b28272e90680787')
                            ]
                        ),
                    ]
                )
            ]
        ),
    ),
    pytest.param(
        BlackboxAssetTest(
            test_name="ShaderBall_RunAP_SuccessWithMatchingProducts",
            asset_folder="ShaderBall",
            scene_debug_file="shaderball.dbgsg",
            assets=[
                asset_db_utils.DBSourceAsset(
                    source_file_name="shaderball.fbx",
                    uuid=b"48181ba8038e5193997540fc8dffb06d",
                    jobs=[
                        asset_db_utils.DBJob(
                            job_key="Scene compilation",
                            builder_guid=b"bd8bf65894854fe3830e8ec3a23c35f3",
                            status=4,
                            error_count=0,
                            warning_count=30,
                            products=[
                                asset_db_utils.DBProduct(
                                    product_name='shaderball/shaderball.dbgsg',
                                    sub_id=-1607815784,
                                    asset_type=b'07f289d14dc74c4094b40a53bbcb9f0b'),
                                asset_db_utils.DBProduct(
                                    product_name='shaderball/shaderball.dbgsg.xml',
                                    sub_id=-1153118555,
                                    asset_type=b'51f376140d774f369ac67ed70a0ac868'),
                            ]
                        ),
                    ]
                )
            ]
        ),
    ),
]

blackbox_fbx_special_tests = [
    pytest.param(
        BlackboxAssetTest(
            test_name="MultipleMeshMultipleMaterial_MultipleAssetInfo_RunAP_SuccessWithMatchingProducts",
            asset_folder="TwoMeshTwoMaterial",
            override_asset_folder="OverrideAssetInfoForTwoMeshTwoMaterial",
            scene_debug_file="multiple_mesh_multiple_material.dbgsg",
            override_scene_debug_file="multiple_mesh_multiple_material_override.dbgsg",
            assets=[
                asset_db_utils.DBSourceAsset(
                    source_file_name="multiple_mesh_multiple_material.fbx",
                    uuid=b"b5915fb874af5c8a866ccabbddb57595",
                    jobs=[
                        asset_db_utils.DBJob(
                            job_key="Scene compilation",
                            builder_guid=b"bd8bf65894854fe3830e8ec3a23c35f3",
                            status=4,
                            error_count=0,
                            warning_count=2,
                            products=[
                                asset_db_utils.DBProduct(
                                    product_name='twomeshtwomaterial/multiple_mesh_multiple_material.dbgsg',
                                    sub_id=896980093,
                                    asset_type=b'07f289d14dc74c4094b40a53bbcb9f0b'),
                                asset_db_utils.DBProduct(
                                    product_name='twomeshtwomaterial/multiple_mesh_multiple_material.dbgsg.xml',
                                    sub_id=-1556988544,
                                    asset_type=b'51f376140d774f369ac67ed70a0ac868'
                                )
                            ]
                        ),
                    ]
                )
            ],
            override_assets=[
                asset_db_utils.DBSourceAsset(
                    source_file_name="multiple_mesh_multiple_material.fbx",
                    uuid=b"b5915fb874af5c8a866ccabbddb57595",
                    jobs=[
                        asset_db_utils.DBJob(
                            job_key="Scene compilation",
                            builder_guid=b"bd8bf65894854fe3830e8ec3a23c35f3",
                            status=4,
                            error_count=0,
                            warning_count=2,
                            products=[
                                asset_db_utils.DBProduct(
                                    product_name='twomeshtwomaterial/multiple_mesh_multiple_material.dbgsg',
                                    sub_id=896980093,
                                    asset_type=b'07f289d14dc74c4094b40a53bbcb9f0b'),
                                asset_db_utils.DBProduct(
                                    product_name='twomeshtwomaterial/multiple_mesh_multiple_material.dbgsg.xml',
                                    sub_id=-1556988544,
                                    asset_type=b'51f376140d774f369ac67ed70a0ac868'
                                )
                            ]
                        ),
                    ]
                )
            ]
        ),
        id="34389075",
        marks=pytest.mark.test_case_id("C34389075"),
    ),
]


@pytest.mark.usefixtures("asset_processor")
@pytest.mark.usefixtures("ap_setup_fixture")
@pytest.mark.usefixtures("local_resources")
@pytest.mark.parametrize("project", targetProjects)
@pytest.mark.assetpipeline
@pytest.mark.SUITE_sandbox
class TestsFBX_AllPlatforms(object):

    @pytest.mark.BAT
    @pytest.mark.SUITE_sandbox
    @pytest.mark.parametrize("blackbox_param", blackbox_fbx_tests)
    def test_FBXBlackboxTest_SourceFiles_Processed_ResultInExpectedProducts(self, workspace, ap_setup_fixture,
                                                                            asset_processor, project, blackbox_param):
        """

        Please see run_fbx_test(...) for details
        Test Steps:
        1. Determine if blackbox is set to none
        2. Run FBX Test

        """

        if blackbox_param == None:
            return
        self.run_fbx_test(workspace, ap_setup_fixture, asset_processor, project, blackbox_param)

    @pytest.mark.BAT
    @pytest.mark.SUITE_sandbox
    @pytest.mark.parametrize("blackbox_param", blackbox_fbx_special_tests)
    def test_FBXBlackboxTest_AssetInfoModified_AssetReprocessed_ResultInExpectedProducts(
            self, workspace, ap_setup_fixture, asset_processor, project, blackbox_param):
        """
        Please see run_fbx_test(...) for details

        Test Steps:
        1. Determine if blackbox is set to none
        2. Run FBX Test
        2. Re-run FBX test and validate the information in override assets
        """

        if blackbox_param == None:
            return
        self.run_fbx_test(workspace, ap_setup_fixture,
                          asset_processor, project, blackbox_param)

        # Run the test again and validate the information in the override assets
        self.run_fbx_test(workspace, ap_setup_fixture,
                          asset_processor, project, blackbox_param, True)

    def populateAssetInfo(self, workspace, project, assets):

        # Check that each given source asset resulted in the expected jobs and products.
        for expected_source in assets:
            for job in expected_source.jobs:
                job.platform = ASSET_PROCESSOR_PLATFORM_MAP[workspace.asset_processor_platform]
                for product in job.products:
                    product.product_name = job.platform + "/" \
                                           + product.product_name

    def compare_scene_debug_file(self, asset_processor, expected_file_path, actual_file_path):
        debug_graph_path = os.path.join(asset_processor.project_test_cache_folder(), actual_file_path)
        expected_debug_graph_path = os.path.join(asset_processor.project_test_source_folder(), "SceneDebug", expected_file_path)

        logger.info(f"Parsing scene graph: {debug_graph_path}")
        with open(debug_graph_path, "r") as scene_file:
            actual_lines = scene_file.readlines()

        logger.info(f"Parsing scene graph: {expected_debug_graph_path}")
        with open(expected_debug_graph_path, "r") as scene_file:
            expected_lines = scene_file.readlines()

        assert utils.compare_lists(actual_lines, expected_lines), "Scene mismatch"
    def run_fbx_test(self, workspace, ap_setup_fixture, asset_processor,
                     project, blackbox_params: BlackboxAssetTest, overrideAsset=False):
        """
        These tests work by having the test case ingest the test data and determine the run pattern.
        Tests will process scene settings files and will additionally do a verification against a provided debug file
        Additionally, if an override is passed, the output is checked against the override.

        Test Steps:
        1. Create temporary test environment
        2. Process Assets
        3. Determine what assets to validate based upon test data
        4. Validate assets were created in cache
        5. If debug file provided, verify scene files were generated correctly
        6. Verify that each given source asset resulted in the expected jobs and products
        """

        test_assets_folder = blackbox_params.override_asset_folder if overrideAsset else blackbox_params.asset_folder
        logger.info(f"{blackbox_params.test_name}: Processing assets in folder '"
                    f"{test_assets_folder}' and verifying they match expected output.")

        # Prepare test environment and process assets
        if not overrideAsset:
            asset_processor.prepare_test_environment(ap_setup_fixture['tests_dir'], test_assets_folder)
        else:
            asset_processor.prepare_test_environment(ap_setup_fixture['tests_dir'], test_assets_folder,
                                                     use_current_root=True, add_scan_folder=False,
                                                     existing_function_name=blackbox_params.asset_folder)
        asset_processor.batch_process(extra_params=["--debugOutput",
                                                    "--regset=\"/O3DE/SceneAPI/AssetImporter/SkipAtomOutput=true\""])

        logger.info(f"Validating assets.")
        assetsToValidate = blackbox_params.override_assets if overrideAsset else blackbox_params.assets

        expected_product_list = []
        for expected_source in assetsToValidate:
            for expected_job in expected_source.jobs:
                for expected_product in expected_job.products:
                    expected_product_list.append(expected_product.product_name)

        missing_assets, _ = utils.compare_assets_with_cache(expected_product_list,
                                                            asset_processor.project_test_cache_folder())

        assert not missing_assets, \
            f'The following assets were expected to be in, but not found in cache: {str(missing_assets)}'

        # Load the asset database.
        db_path = os.path.join(asset_processor.temp_asset_root(), "Cache",
                               "assetdb.sqlite")
        cache_root = os.path.dirname(os.path.join(asset_processor.temp_asset_root(), "Cache",
                                                  ASSET_PROCESSOR_PLATFORM_MAP[workspace.asset_processor_platform]))

        if blackbox_params.scene_debug_file:
            scene_debug_file = blackbox_params.override_scene_debug_file if overrideAsset \
                else blackbox_params.scene_debug_file

            self.compare_scene_debug_file(asset_processor, scene_debug_file, blackbox_params.scene_debug_file)

            # Run again for the .dbgsg.xml file
            self.compare_scene_debug_file(asset_processor,
                                          scene_debug_file + ".xml",
                                          blackbox_params.scene_debug_file + ".xml")

        # Check that each given source asset resulted in the expected jobs and products.
        self.populateAssetInfo(workspace, project, assetsToValidate)
        for expected_source in assetsToValidate:
            for job in expected_source.jobs:
                for product in job.products:
                    asset_db_utils.compare_expected_asset_to_actual_asset(db_path,
                                                                          expected_source,
                                                                          f"{blackbox_params.asset_folder}/"
                                                                          f"{expected_source.source_file_name}",
                                                                          cache_root)
