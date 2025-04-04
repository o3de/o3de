"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import binascii
from dataclasses import dataclass
import logging
import os
from pprint import pformat
import pytest
import re
import shutil
from typing import List
import warnings

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

# Helper: Gets a case correct version of the cache folder
def get_cache_folder(asset_processor):
    # Make sure the folder being checked is fully lowercase.
    # Leave the "c" in Cache uppercase.
    return re.sub("ache[/\\\\](.*)", lambda m: m.group().lower(), asset_processor.project_test_cache_folder())

@pytest.fixture
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
            scene_debug_file="onemeshonematerial.fbx.dbgsg",
            assets=[
                asset_db_utils.DBSourceAsset(
                    source_file_name="OneMeshOneMaterial.fbx",
                    uuid=b"8a9164adb84859be893e18aa819438e1",
                    jobs=[
                        asset_db_utils.DBJob(
                            job_key='Scene compilation',
                            builder_guid=b'bd8bf65894854fe3830e8ec3a23c35f3',
                            status=4,
                            error_count=0,
                            products=[
                                asset_db_utils.DBProduct(
                                    product_name='onemeshonematerial/onemeshonematerial.fbx.abdata.json',
                                    sub_id=4194304,
                                    asset_type=b'd0a5e84e98664ad7a6a14d28fe7871c5'),
                                asset_db_utils.DBProduct(
                                    product_name='onemeshonematerial/onemeshonematerial.fbx.assetinfo.dbg',
                                    sub_id=635465363,
                                    asset_type=b'48a78be7b3f244b88aa6f0607e9a75a5'),
                                asset_db_utils.DBProduct(
                                    product_name='onemeshonematerial/onemeshonematerial.fbx.dbgsg',
                                    sub_id=2078121323,
                                    asset_type=b'07f289d14dc74c4094b40a53bbcb9f0b'),
                                asset_db_utils.DBProduct(
                                    product_name='onemeshonematerial/onemeshonematerial.fbx.dbgsg.json',
                                    sub_id=1736583047,
                                    asset_type=b'4342b27e0e1449c3b3b9bcdb9a5fca23'),
                                asset_db_utils.DBProduct(
                                    product_name='onemeshonematerial/onemeshonematerial.fbx.dbgsg.xml',
                                    sub_id=-976624731, asset_type=b'51f376140d774f369ac67ed70a0ac868'),
                                asset_db_utils.DBProduct(
                                    product_name='onemeshonematerial/onemeshonematerial_fbx.procprefab',
                                    sub_id=-1819295853,
                                    asset_type=b'9b7c8459471e4eada3637990cc4065a9'),
                                asset_db_utils.DBProduct(
                                    product_name='onemeshonematerial/onemeshonematerial_fbx.procprefab.json',
                                    sub_id=-1860036023,
                                    asset_type=b'00000000000000000000000000000000')
                            ]
                        )
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
            test_name="SoftNamingLOD_RunAP_SuccessWithMatchingProducts",
            asset_folder="SoftNamingLOD",
            scene_debug_file="lodtest.fbx.dbgsg",
            assets=[
                asset_db_utils.DBSourceAsset(
                    source_file_name="lodtest.fbx",
                    uuid=b"44c8627fe2c25aae91fe3ff9547be3b9",
                    jobs=[
                        asset_db_utils.DBJob(
                            job_key='Scene compilation',
                            builder_guid=b'bd8bf65894854fe3830e8ec3a23c35f3',
                            status=4,
                            error_count=0,
                            products=[
                                asset_db_utils.DBProduct(
                                    product_name='softnaminglod/lodtest.fbx.abdata.json',
                                    sub_id=4194304,
                                    asset_type=b'd0a5e84e98664ad7a6a14d28fe7871c5'),
                                asset_db_utils.DBProduct(
                                    product_name='softnaminglod/lodtest.fbx.assetinfo.dbg',
                                    sub_id=-1252475526,
                                    asset_type=b'48a78be7b3f244b88aa6f0607e9a75a5'),
                                asset_db_utils.DBProduct(
                                    product_name='softnaminglod/lodtest.fbx.dbgsg',
                                    sub_id=1082129636,
                                    asset_type=b'07f289d14dc74c4094b40a53bbcb9f0b'),
                                asset_db_utils.DBProduct(
                                    product_name='softnaminglod/lodtest.fbx.dbgsg.json',
                                    sub_id=998015244,
                                    asset_type=b'4342b27e0e1449c3b3b9bcdb9a5fca23'),
                                asset_db_utils.DBProduct(
                                    product_name='softnaminglod/lodtest.fbx.dbgsg.xml',
                                    sub_id=1858673704,
                                    asset_type=b'51f376140d774f369ac67ed70a0ac868'),
                                asset_db_utils.DBProduct(
                                    product_name='softnaminglod/lodtest_fbx.procprefab',
                                    sub_id=838906682,
                                    asset_type=b'9b7c8459471e4eada3637990cc4065a9'),
                                asset_db_utils.DBProduct(
                                    product_name='softnaminglod/lodtest_fbx.procprefab.json',
                                    sub_id=-71923453,
                                    asset_type=b'00000000000000000000000000000000'
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
            test_name="SoftNamingPhysics_RunAP_SuccessWithMatchingProducts",
            asset_folder="SoftNamingPhysics",
            scene_debug_file="physicstest.fbx.dbgsg",
            assets=[
                asset_db_utils.DBSourceAsset(
                    source_file_name="physicstest.fbx",
                    uuid=b"df957b7918cf5b029806c73f630fa1c8",
                    jobs=[
                        asset_db_utils.DBJob(
                            job_key='Scene compilation',
                            builder_guid=b'bd8bf65894854fe3830e8ec3a23c35f3',
                            status=4,
                            error_count=0,
                            products=[
                                asset_db_utils.DBProduct(
                                    product_name='softnamingphysics/physicstest.fbx.abdata.json',
                                    sub_id=4194304,
                                    asset_type=b'd0a5e84e98664ad7a6a14d28fe7871c5'),
                                asset_db_utils.DBProduct(
                                    product_name='softnamingphysics/physicstest.fbx.assetinfo.dbg',
                                    sub_id=376675704,
                                    asset_type=b'48a78be7b3f244b88aa6f0607e9a75a5'),
                                asset_db_utils.DBProduct(
                                    product_name='softnamingphysics/physicstest.fbx.dbgsg',
                                    sub_id=530659840,
                                    asset_type=b'07f289d14dc74c4094b40a53bbcb9f0b'),
                                asset_db_utils.DBProduct(
                                    product_name='softnamingphysics/physicstest.fbx.dbgsg.json',
                                    sub_id=997442341,
                                    asset_type=b'4342b27e0e1449c3b3b9bcdb9a5fca23'),
                                asset_db_utils.DBProduct(
                                    product_name='softnamingphysics/physicstest.fbx.dbgsg.xml',
                                    sub_id=1618976652,
                                    asset_type=b'51f376140d774f369ac67ed70a0ac868'),
                                asset_db_utils.DBProduct(
                                    product_name='softnamingphysics/physicstest.fbx.pxmesh',
                                    sub_id=640975857,
                                    asset_type=b'7a2871b95eab4de0a901b0d2c6920ddb'),
                                asset_db_utils.DBProduct(
                                    product_name='softnamingphysics/physicstest_fbx.procprefab',
                                    sub_id=651014275,
                                    asset_type=b'9b7c8459471e4eada3637990cc4065a9'),
                                asset_db_utils.DBProduct(
                                    product_name='softnamingphysics/physicstest_fbx.procprefab.json',
                                    sub_id=1124966094,
                                    asset_type=b'00000000000000000000000000000000'
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
            scene_debug_file="multiple_mesh_one_material.fbx.dbgsg",
            assets=[
                asset_db_utils.DBSourceAsset(
                    source_file_name="multiple_mesh_one_material.fbx",
                    uuid=b"597618fd497659a1b197a015fe47aa95",
                    jobs=[
                        asset_db_utils.DBJob(
                            job_key='Scene compilation',
                            builder_guid=b'bd8bf65894854fe3830e8ec3a23c35f3',
                            status=4,
                            error_count=0,
                            products=[
                                asset_db_utils.DBProduct(
                                    product_name='twomeshonematerial/multiple_mesh_one_material.fbx.abdata.json',
                                    sub_id=4194304,
                                    asset_type=b'd0a5e84e98664ad7a6a14d28fe7871c5'),
                                asset_db_utils.DBProduct(
                                    product_name='twomeshonematerial/multiple_mesh_one_material.fbx.assetinfo.dbg',
                                    sub_id=-251254148,
                                    asset_type=b'48a78be7b3f244b88aa6f0607e9a75a5'),
                                asset_db_utils.DBProduct(
                                    product_name='twomeshonematerial/multiple_mesh_one_material.fbx.dbgsg',
                                    sub_id=-599090337,
                                    asset_type=b'07f289d14dc74c4094b40a53bbcb9f0b'),
                                asset_db_utils.DBProduct(
                                    product_name='twomeshonematerial/multiple_mesh_one_material.fbx.dbgsg.json',
                                    sub_id=-981232068,
                                    asset_type=b'4342b27e0e1449c3b3b9bcdb9a5fca23'),
                                asset_db_utils.DBProduct(
                                    product_name='twomeshonematerial/multiple_mesh_one_material.fbx.dbgsg.xml',
                                    sub_id=2098803780,
                                    asset_type=b'51f376140d774f369ac67ed70a0ac868'
                                )
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
            scene_debug_file="multiple_mesh_linked_materials.fbx.dbgsg",
            assets=[
                asset_db_utils.DBSourceAsset(
                    source_file_name="multiple_mesh_linked_materials.fbx",
                    uuid=b"25d8301c2eef5dc7bded310db8ea608d",
                    jobs=[
                        asset_db_utils.DBJob(
                            job_key='Scene compilation',
                            builder_guid=b'bd8bf65894854fe3830e8ec3a23c35f3',
                            status=4,
                            error_count=0,
                            products=[
                                asset_db_utils.DBProduct(
                                    product_name='twomeshlinkedmaterials/multiple_mesh_linked_materials.fbx.abdata.json',
                                    sub_id=4194304,
                                    asset_type=b'd0a5e84e98664ad7a6a14d28fe7871c5'),
                                asset_db_utils.DBProduct(
                                    product_name='twomeshlinkedmaterials/multiple_mesh_linked_materials.fbx.assetinfo.dbg',
                                    sub_id=1774684099,
                                    asset_type=b'48a78be7b3f244b88aa6f0607e9a75a5'),
                                asset_db_utils.DBProduct(
                                    product_name='twomeshlinkedmaterials/multiple_mesh_linked_materials.fbx.dbgsg',
                                    sub_id=399409097,
                                    asset_type=b'07f289d14dc74c4094b40a53bbcb9f0b'),
                                asset_db_utils.DBProduct(
                                    product_name='twomeshlinkedmaterials/multiple_mesh_linked_materials.fbx.dbgsg.json',
                                    sub_id=-977002611,
                                    asset_type=b'4342b27e0e1449c3b3b9bcdb9a5fca23'),
                                asset_db_utils.DBProduct(
                                    product_name='twomeshlinkedmaterials/multiple_mesh_linked_materials.fbx.dbgsg.xml',
                                    sub_id=-1473507434,
                                    asset_type=b'51f376140d774f369ac67ed70a0ac868'),
                                asset_db_utils.DBProduct(
                                    product_name='twomeshlinkedmaterials/multiple_mesh_linked_materials_fbx.procprefab',
                                    sub_id=-859152125,
                                    asset_type=b'9b7c8459471e4eada3637990cc4065a9'),
                                asset_db_utils.DBProduct(
                                    product_name='twomeshlinkedmaterials/multiple_mesh_linked_materials_fbx.procprefab.json',
                                    sub_id=-841089725,
                                    asset_type=b'00000000000000000000000000000000'
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
            scene_debug_file="single_mesh_multiple_materials.fbx.dbgsg",
            assets=[
                asset_db_utils.DBSourceAsset(
                    source_file_name="single_mesh_multiple_materials.fbx",
                    uuid=b"f08fd585dfa35881b4bf86637da5e858",
                    jobs=[
                        asset_db_utils.DBJob(
                            job_key='Scene compilation',
                            builder_guid=b'bd8bf65894854fe3830e8ec3a23c35f3',
                            status=4,
                            error_count=0,
                            products=[
                                asset_db_utils.DBProduct(
                                    product_name='onemeshmultiplematerials/single_mesh_multiple_materials.fbx.abdata.json',
                                    sub_id=4194304,
                                    asset_type=b'd0a5e84e98664ad7a6a14d28fe7871c5'),
                                asset_db_utils.DBProduct(
                                    product_name='onemeshmultiplematerials/single_mesh_multiple_materials.fbx.assetinfo.dbg',
                                    sub_id=-2146161009,
                                    asset_type=b'48a78be7b3f244b88aa6f0607e9a75a5'),
                                asset_db_utils.DBProduct(
                                    product_name='onemeshmultiplematerials/single_mesh_multiple_materials.fbx.dbgsg',
                                    sub_id=82607561,
                                    asset_type=b'07f289d14dc74c4094b40a53bbcb9f0b'),
                                asset_db_utils.DBProduct(
                                    product_name='onemeshmultiplematerials/single_mesh_multiple_materials.fbx.dbgsg.json',
                                    sub_id=72739255,
                                    asset_type=b'4342b27e0e1449c3b3b9bcdb9a5fca23'),
                                asset_db_utils.DBProduct(
                                    product_name='onemeshmultiplematerials/single_mesh_multiple_materials.fbx.dbgsg.xml',
                                    sub_id=-763826724,
                                    asset_type=b'51f376140d774f369ac67ed70a0ac868'),
                                asset_db_utils.DBProduct(
                                    product_name='onemeshmultiplematerials/single_mesh_multiple_materials_fbx.procprefab',
                                    sub_id=1496946180,
                                    asset_type=b'9b7c8459471e4eada3637990cc4065a9'),
                                asset_db_utils.DBProduct(
                                    product_name='onemeshmultiplematerials/single_mesh_multiple_materials_fbx.procprefab.json',
                                    sub_id=-537346917,
                                    asset_type=b'00000000000000000000000000000000'
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
            scene_debug_file="vertexcolor.fbx.dbgsg",
            assets=[
                asset_db_utils.DBSourceAsset(
                    source_file_name="VertexColor.fbx",
                    uuid=b"207e7e1540785a26b064e9be67361cdf",
                    jobs=[
                        asset_db_utils.DBJob(
                            job_key='Scene compilation',
                            builder_guid=b'bd8bf65894854fe3830e8ec3a23c35f3',
                            status=4,
                            error_count=0,
                            products=[
                                asset_db_utils.DBProduct(
                                    product_name='vertexcolor/vertexcolor.fbx.abdata.json',
                                    sub_id=4194304,
                                    asset_type=b'd0a5e84e98664ad7a6a14d28fe7871c5'),
                                asset_db_utils.DBProduct(
                                    product_name='vertexcolor/vertexcolor.fbx.assetinfo.dbg',
                                    sub_id=-684486226,
                                    asset_type=b'48a78be7b3f244b88aa6f0607e9a75a5'),
                                asset_db_utils.DBProduct(
                                    product_name='vertexcolor/vertexcolor.fbx.dbgsg',
                                    sub_id=-501404951,
                                    asset_type=b'07f289d14dc74c4094b40a53bbcb9f0b'),
                                asset_db_utils.DBProduct(
                                    product_name='vertexcolor/vertexcolor.fbx.dbgsg.json',
                                    sub_id=1580896704,
                                    asset_type=b'4342b27e0e1449c3b3b9bcdb9a5fca23'),
                                asset_db_utils.DBProduct(
                                    product_name='vertexcolor/vertexcolor.fbx.dbgsg.xml',
                                    sub_id=-2021002009,
                                    asset_type=b'51f376140d774f369ac67ed70a0ac868'),
                                asset_db_utils.DBProduct(
                                    product_name='vertexcolor/vertexcolor_fbx.procprefab',
                                    sub_id=604991654,
                                    asset_type=b'9b7c8459471e4eada3637990cc4065a9'),
                                asset_db_utils.DBProduct(
                                    product_name='vertexcolor/vertexcolor_fbx.procprefab.json',
                                    sub_id=-1669533680,
                                    asset_type=b'00000000000000000000000000000000'
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
            scene_debug_file="jack_idle_aim_zup.fbx.dbgsg",
            assets=[
                asset_db_utils.DBSourceAsset(
                    source_file_name="Jack_Idle_Aim_ZUp.fbx",
                    uuid=b"eda904ae0e145f8b973d57fc5809918b",
                    jobs=[
                        asset_db_utils.DBJob(
                            job_key='Scene compilation',
                            builder_guid=b'bd8bf65894854fe3830e8ec3a23c35f3',
                            status=4,
                            error_count=0,
                            products=[
                                asset_db_utils.DBProduct(
                                    product_name='motion/jack_idle_aim_zup.fbx.abdata.json',
                                    sub_id=4194304,
                                    asset_type=b'd0a5e84e98664ad7a6a14d28fe7871c5'),
                                asset_db_utils.DBProduct(
                                    product_name='motion/jack_idle_aim_zup.fbx.assetinfo.dbg',
                                    sub_id=-1090444673,
                                    asset_type=b'48a78be7b3f244b88aa6f0607e9a75a5'),
                                asset_db_utils.DBProduct(
                                    product_name='motion/jack_idle_aim_zup.fbx.dbgsg',
                                    sub_id=-1042666277,
                                    asset_type=b'07f289d14dc74c4094b40a53bbcb9f0b'),
                                asset_db_utils.DBProduct(
                                    product_name='motion/jack_idle_aim_zup.fbx.dbgsg.json',
                                    sub_id=-290748690,
                                    asset_type=b'4342b27e0e1449c3b3b9bcdb9a5fca23'),
                                asset_db_utils.DBProduct(
                                    product_name='motion/jack_idle_aim_zup.fbx.dbgsg.xml',
                                    sub_id=444631267,
                                    asset_type=b'51f376140d774f369ac67ed70a0ac868'),
                                asset_db_utils.DBProduct(
                                    product_name='motion/jack_idle_aim_zup.motion',
                                    sub_id=186392073,
                                    asset_type=b'00494b8e75784ba28b28272e90680787'
                                ),
                                asset_db_utils.DBProduct(
                                    product_name='motion/jack_idle_aim_zup_fbx.procprefab',
                                    sub_id=1049691217,
                                    asset_type=b'9b7c8459471e4eada3637990cc4065a9'
                                ),
                                asset_db_utils.DBProduct(
                                    product_name='motion/jack_idle_aim_zup_fbx.procprefab.json',
                                    sub_id=-980081481,
                                    asset_type=b'00000000000000000000000000000000'
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
            test_name="ShaderBall_RunAP_SuccessWithMatchingProducts",
            asset_folder="ShaderBall",
            scene_debug_file="shaderball.fbx.dbgsg",
            assets=[
                asset_db_utils.DBSourceAsset(
                    source_file_name="shaderball.fbx",
                    uuid=b"48181ba8038e5193997540fc8dffb06d",
                    jobs=[
                        asset_db_utils.DBJob(
                            job_key='Scene compilation',
                            builder_guid=b'bd8bf65894854fe3830e8ec3a23c35f3',
                            status=4,
                            error_count=0,
                            products=[
                                asset_db_utils.DBProduct(
                                    product_name='shaderball/shaderball.fbx.abdata.json',
                                    sub_id=4194304,
                                    asset_type=b'd0a5e84e98664ad7a6a14d28fe7871c5'),
                                asset_db_utils.DBProduct(
                                    product_name='shaderball/shaderball.fbx.assetinfo.dbg',
                                    sub_id=2366451,
                                    asset_type=b'48a78be7b3f244b88aa6f0607e9a75a5'),
                                asset_db_utils.DBProduct(
                                    product_name='shaderball/shaderball.fbx.dbgsg',
                                    sub_id=919030955,
                                    asset_type=b'07f289d14dc74c4094b40a53bbcb9f0b'),
                                asset_db_utils.DBProduct(
                                    product_name='shaderball/shaderball.fbx.dbgsg.json',
                                    sub_id=-1710127594,
                                    asset_type=b'4342b27e0e1449c3b3b9bcdb9a5fca23'),
                                asset_db_utils.DBProduct(
                                    product_name='shaderball/shaderball.fbx.dbgsg.xml',
                                    sub_id=-1137683506,
                                    asset_type=b'51f376140d774f369ac67ed70a0ac868'),
                                asset_db_utils.DBProduct(
                                    product_name='shaderball/shaderball_fbx.procprefab',
                                    sub_id=-1501914312,
                                    asset_type=b'9b7c8459471e4eada3637990cc4065a9'),
                                asset_db_utils.DBProduct(
                                    product_name='shaderball/shaderball_fbx.procprefab.json',
                                    sub_id=-1464074454,
                                    asset_type=b'00000000000000000000000000000000'
                                ),
                            ]
                        ),
                    ]
                )
            ]
        ),
    ),
    pytest.param(
        BlackboxAssetTest(
            test_name="cubewithline_RunAP_CubeOutputsWithoutLine",
            asset_folder="cubewithline",
            scene_debug_file="cubewithline.fbx.dbgsg",
            assets=[
                asset_db_utils.DBSourceAsset(
                    source_file_name="cubewithline.fbx",
                    uuid=b'1ee8fbf7c1f25b8399395a112e51906c',
                    jobs=[
                        asset_db_utils.DBJob(
                            job_key='Scene compilation',
                            builder_guid=b'bd8bf65894854fe3830e8ec3a23c35f3',
                            status=4,
                            error_count=0,
                            products=[
                                asset_db_utils.DBProduct(
                                    product_name='cubewithline/cubewithline.fbx.abdata.json',
                                    sub_id=4194304,
                                    asset_type=b'd0a5e84e98664ad7a6a14d28fe7871c5'),
                                asset_db_utils.DBProduct(
                                    product_name='cubewithline/cubewithline.fbx.assetinfo.dbg',
                                    sub_id=705480985,
                                    asset_type=b'48a78be7b3f244b88aa6f0607e9a75a5'),
                                asset_db_utils.DBProduct(
                                    product_name='cubewithline/cubewithline.fbx.dbgsg',
                                    sub_id=25987353,
                                    asset_type=b'07f289d14dc74c4094b40a53bbcb9f0b'),
                                asset_db_utils.DBProduct(
                                    product_name='cubewithline/cubewithline.fbx.dbgsg.json',
                                    sub_id=-1971624352,
                                    asset_type=b'4342b27e0e1449c3b3b9bcdb9a5fca23'),
                                asset_db_utils.DBProduct(
                                    product_name='cubewithline/cubewithline.fbx.dbgsg.xml',
                                    sub_id=975809434,
                                    asset_type=b'51f376140d774f369ac67ed70a0ac868'),
                                asset_db_utils.DBProduct(
                                    product_name='cubewithline/cubewithline_fbx.procprefab',
                                    sub_id=1800163200,
                                    asset_type=b'9b7c8459471e4eada3637990cc4065a9'),
                                asset_db_utils.DBProduct(
                                    product_name='cubewithline/cubewithline_fbx.procprefab.json',
                                    sub_id=2139660816,
                                    asset_type=b'00000000000000000000000000000000'
                                ),
                            ]
                        ),
                    ]
                )
            ]
        ),
    ),
    pytest.param(
        BlackboxAssetTest(
            test_name="MorphTargetOneMaterial_RunAP_SuccessWithMatchingProducts",
            asset_folder="MorphTargetOneMaterial",
            scene_debug_file="morphtargetonematerial.fbx.dbgsg",
            assets=[
                asset_db_utils.DBSourceAsset(
                    source_file_name="morphtargetonematerial.fbx",
                    uuid=b'6f2c17db3f4a5be8bcd2bece01c92f6d',
                    jobs=[
                        asset_db_utils.DBJob(
                            job_key='Scene compilation',
                            builder_guid=b'bd8bf65894854fe3830e8ec3a23c35f3',
                            status=4,
                            error_count=0,
                            products=[
                                asset_db_utils.DBProduct(
                                    product_name='morphtargetonematerial/morphtargetonematerial.actor',
                                    sub_id=-999339669,
                                    asset_type=b'f67cc648ea51464c9f5d4a9ce41a7f86'),
                                asset_db_utils.DBProduct(
                                    product_name='morphtargetonematerial/morphtargetonematerial.fbx.abdata.json',
                                    sub_id=4194304,
                                    asset_type=b'd0a5e84e98664ad7a6a14d28fe7871c5'),
                                asset_db_utils.DBProduct(
                                    product_name='morphtargetonematerial/morphtargetonematerial.fbx.assetinfo.dbg',
                                    sub_id=726384549,
                                    asset_type=b'48a78be7b3f244b88aa6f0607e9a75a5'),
                                asset_db_utils.DBProduct(
                                    product_name='morphtargetonematerial/morphtargetonematerial.fbx.dbgsg',
                                    sub_id=1620685182,
                                    asset_type=b'07f289d14dc74c4094b40a53bbcb9f0b'),
                                asset_db_utils.DBProduct(
                                    product_name='morphtargetonematerial/morphtargetonematerial.fbx.dbgsg.json',
                                    sub_id=1253137975,
                                    asset_type=b'4342b27e0e1449c3b3b9bcdb9a5fca23'),
                                asset_db_utils.DBProduct(
                                    product_name='morphtargetonematerial/morphtargetonematerial.fbx.dbgsg.xml',
                                    sub_id=-340185820,
                                    asset_type=b'51f376140d774f369ac67ed70a0ac868'),
                                asset_db_utils.DBProduct(
                                    product_name='morphtargetonematerial/morphtargetonematerial.motion',
                                    sub_id=692653652,
                                    asset_type=b'00494b8e75784ba28b28272e90680787'),
                            ]
                        ),
                    ]
                )
            ]
        ),
    ),
    pytest.param(
        BlackboxAssetTest(
            test_name="MorphTargetTwoMaterials_RunAP_SuccessWithMatchingProducts",
            asset_folder="MorphTargetTwoMaterials",
            scene_debug_file="morphtargettwomaterials.fbx.dbgsg",
            assets=[
                asset_db_utils.DBSourceAsset(
                    source_file_name="morphtargettwomaterials.fbx",
                    uuid=b'37c55eedd26658a4a6f7cbe2bec267d7',
                    jobs=[
                        asset_db_utils.DBJob(
                            job_key='Scene compilation',
                            builder_guid=b'bd8bf65894854fe3830e8ec3a23c35f3',
                            status=4,
                            error_count=0,
                            products=[
                                asset_db_utils.DBProduct(
                                    product_name='morphtargettwomaterials/morphtargettwomaterials.actor',
                                    sub_id=-557664045,
                                    asset_type=b'f67cc648ea51464c9f5d4a9ce41a7f86'),
                                asset_db_utils.DBProduct(
                                    product_name='morphtargettwomaterials/morphtargettwomaterials.fbx.abdata.json',
                                    sub_id=4194304,
                                    asset_type=b'd0a5e84e98664ad7a6a14d28fe7871c5'),
                                asset_db_utils.DBProduct(
                                    product_name='morphtargettwomaterials/morphtargettwomaterials.fbx.assetinfo.dbg',
                                    sub_id=-1807090167,
                                    asset_type=b'48a78be7b3f244b88aa6f0607e9a75a5'),
                                asset_db_utils.DBProduct(
                                    product_name='morphtargettwomaterials/morphtargettwomaterials.fbx.dbgsg',
                                    sub_id=-303201013,
                                    asset_type=b'07f289d14dc74c4094b40a53bbcb9f0b'),
                                asset_db_utils.DBProduct(
                                    product_name='morphtargettwomaterials/morphtargettwomaterials.fbx.dbgsg.json',
                                    sub_id=-1396790465,
                                    asset_type=b'4342b27e0e1449c3b3b9bcdb9a5fca23'),
                                asset_db_utils.DBProduct(
                                    product_name='morphtargettwomaterials/morphtargettwomaterials.fbx.dbgsg.xml',
                                    sub_id=-1157438659,
                                    asset_type=b'51f376140d774f369ac67ed70a0ac868'),
                                asset_db_utils.DBProduct(
                                    product_name='morphtargettwomaterials/morphtargettwomaterials.motion',
                                    sub_id=1527116269,
                                    asset_type=b'00494b8e75784ba28b28272e90680787'),
                            ]
                        ),
                    ]
                )
            ]
        ),
    ),

    pytest.param(
        BlackboxAssetTest(
            test_name="StingRayPBRAsset_PBRMaterialConvertion_AutoAssignOnProcessing",
            asset_folder="PBRMaterialConvertion",
            scene_debug_file="nagamaki1.fbx.dbgsg",
            assets=[
                asset_db_utils.DBSourceAsset(
                    source_file_name="nagamaki1.fbx",
                    uuid=b'9dbee85b454d53cf894b90a9ceb1430a',
                    jobs=[
                        asset_db_utils.DBJob(
                            job_key='Scene compilation',
                            builder_guid=b'bd8bf65894854fe3830e8ec3a23c35f3',
                            status=4,
                            error_count=0,
                            products=[
                                asset_db_utils.DBProduct(
                                    product_name='pbrmaterialconvertion/nagamaki1.fbx.abdata.json',
                                    sub_id=4194304,
                                    asset_type=b'd0a5e84e98664ad7a6a14d28fe7871c5'),
                                asset_db_utils.DBProduct(
                                    product_name='pbrmaterialconvertion/nagamaki1.fbx.assetinfo.dbg',
                                    sub_id=-799018249,
                                    asset_type=b'48a78be7b3f244b88aa6f0607e9a75a5'),
                                asset_db_utils.DBProduct(
                                    product_name='pbrmaterialconvertion/nagamaki1.fbx.dbgsg',
                                    sub_id=-1473851462,
                                    asset_type=b'07f289d14dc74c4094b40a53bbcb9f0b'),
                                asset_db_utils.DBProduct(
                                    product_name='pbrmaterialconvertion/nagamaki1.fbx.dbgsg.json',
                                    sub_id=-1739655040,
                                    asset_type=b'4342b27e0e1449c3b3b9bcdb9a5fca23'),
                                asset_db_utils.DBProduct(
                                    product_name='pbrmaterialconvertion/nagamaki1.fbx.dbgsg.xml',
                                    sub_id=-2094917174,
                                    asset_type=b'51f376140d774f369ac67ed70a0ac868'),
                                asset_db_utils.DBProduct(
                                    product_name='pbrmaterialconvertion/nagamaki1_fbx.procprefab',
                                    sub_id=-1637699071,
                                    asset_type=b'9b7c8459471e4eada3637990cc4065a9'),
                                asset_db_utils.DBProduct(
                                    product_name='pbrmaterialconvertion/nagamaki1_fbx.procprefab.json',
                                    sub_id=-602240166,
                                    asset_type=b'00000000000000000000000000000000'
                                )
                            ]
                        )
                    ]
                )
            ]
        )
    )
]

blackbox_fbx_special_tests = [
    pytest.param(
        BlackboxAssetTest(
            test_name="MultipleMeshMultipleMaterial_MultipleAssetInfo_RunAP_SuccessWithMatchingProducts",
            asset_folder="TwoMeshTwoMaterial",
            override_asset_folder="OverrideAssetInfoForTwoMeshTwoMaterial",
            scene_debug_file="multiple_mesh_multiple_material.fbx.dbgsg",
            override_scene_debug_file="multiple_mesh_multiple_material_override.fbx.dbgsg",
            assets=[
                asset_db_utils.DBSourceAsset(
                    source_file_name="multiple_mesh_multiple_material.fbx",
                    uuid=b"b5915fb874af5c8a866ccabbddb57595",
                    jobs=[
                        asset_db_utils.DBJob(
                            job_key='Scene compilation',
                            builder_guid=b'bd8bf65894854fe3830e8ec3a23c35f3',
                            status=4,
                            error_count=0,
                            products=[
                                asset_db_utils.DBProduct(
                                    product_name='twomeshtwomaterial/multiple_mesh_multiple_material.fbx.abdata.json',
                                    sub_id=4194304,
                                    asset_type=b'd0a5e84e98664ad7a6a14d28fe7871c5'),
                                asset_db_utils.DBProduct(
                                    product_name='twomeshtwomaterial/multiple_mesh_multiple_material.fbx.assetinfo.dbg',
                                    sub_id=-1020411616,
                                    asset_type=b'48a78be7b3f244b88aa6f0607e9a75a5'),
                                asset_db_utils.DBProduct(
                                    product_name='twomeshtwomaterial/multiple_mesh_multiple_material.fbx.dbgsg',
                                    sub_id=2097661127,
                                    asset_type=b'07f289d14dc74c4094b40a53bbcb9f0b'),
                                asset_db_utils.DBProduct(
                                    product_name='twomeshtwomaterial/multiple_mesh_multiple_material.fbx.dbgsg.json',
                                    sub_id=-1745484895,
                                    asset_type=b'4342b27e0e1449c3b3b9bcdb9a5fca23'),
                                asset_db_utils.DBProduct(
                                    product_name='twomeshtwomaterial/multiple_mesh_multiple_material.fbx.dbgsg.xml',
                                    sub_id=694850264,
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
                            job_key='Scene compilation',
                            builder_guid=b'bd8bf65894854fe3830e8ec3a23c35f3',
                            status=4,
                            error_count=0,
                            products=[
                                asset_db_utils.DBProduct(
                                    product_name='twomeshtwomaterial/multiple_mesh_multiple_material.fbx.abdata.json',
                                    sub_id=4194304,
                                    asset_type=b'd0a5e84e98664ad7a6a14d28fe7871c5'),
                                asset_db_utils.DBProduct(
                                    product_name='twomeshtwomaterial/multiple_mesh_multiple_material.fbx.assetinfo.dbg',
                                    sub_id=-1020411616,
                                    asset_type=b'48a78be7b3f244b88aa6f0607e9a75a5'),
                                asset_db_utils.DBProduct(
                                    product_name='twomeshtwomaterial/multiple_mesh_multiple_material.fbx.dbgsg',
                                    sub_id=2097661127,
                                    asset_type=b'07f289d14dc74c4094b40a53bbcb9f0b'),
                                asset_db_utils.DBProduct(
                                    product_name='twomeshtwomaterial/multiple_mesh_multiple_material.fbx.dbgsg.json',
                                    sub_id=-1745484895,
                                    asset_type=b'4342b27e0e1449c3b3b9bcdb9a5fca23'),
                                asset_db_utils.DBProduct(
                                    product_name='twomeshtwomaterial/multiple_mesh_multiple_material.fbx.dbgsg.xml',
                                    sub_id=694850264,
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

    @staticmethod
    def populate_asset_info(workspace, project, assets):

        # Check that each given source asset resulted in the expected jobs and products.
        for expected_source in assets:
            for job in expected_source.jobs:
                job.platform = ASSET_PROCESSOR_PLATFORM_MAP[workspace.asset_processor_platform]
                for product in job.products:
                    product.product_name = job.platform + "/" \
                                           + product.product_name


    @staticmethod
    def trim_floating_point_values_from_same_length_lists(diff_actual: List[str], diff_expected: List[str],
            actual_file_path:str, expected_file_path:str) -> (List[str], List[str]):
        # Linux and non-Linux platforms generate slightly different values for floating points.
        # Long term, it will be important to stabilize the output of product assets, because this difference
        # will cause problems: If an Android asset is generated from a Linux versus Windows machine, for example,
        # it will be different when it's not expected to be different.
        # In the short term, it's not something addressed yet, so instead this function will
        # truncate any floating point values to be short enough to be stable. It will then emit a warning, to help keep track of this issue.
        
        # Get the initial list lengths, so they can be compared to the list lengths later to see if any differences were
        # removed due to floating point value drift.
        initial_diff_actual_len = len(diff_actual)
        initial_diff_expected_len = len(diff_expected)

        # This function requires the two lists to be equal length.
        assert initial_diff_actual_len == initial_diff_expected_len, "Scene mismatch - different line counts"

        # Floating point values between Linux and Windows aren't consistent yet. For now, trim these values for comparison.
        # Store the trimmed values and compare the un-trimmed values separately, emitting warnings.
        # Trim decimals from the lists to be compared, if any where found, re-compare and generate new lists.
        DECIMAL_DIGITS_TO_PRESERVE = 3
        floating_point_regex = re.compile(f"(.*?-?[0-9]+\\.[0-9]{{{DECIMAL_DIGITS_TO_PRESERVE},{DECIMAL_DIGITS_TO_PRESERVE}}})[0-9]+(.*)")
        for index, diff_actual_line in enumerate(diff_actual):
            # Loop, because there may be multiple floats on the same line.
            while True:
                match_result = floating_point_regex.match(diff_actual[index])
                if not match_result:
                    break
                diff_actual[index] = f"{match_result.group(1)}{match_result.group(2)}"
            # diff_actual and diff_expected have the same line count, so they can both be checked here
            while True:
                match_result = floating_point_regex.match(diff_expected[index])
                if not match_result:
                    break
                diff_expected[index] = f"{match_result.group(1)}{match_result.group(2)}"
                
        # Re-run the diff now that floating point values have been truncated.
        diff_actual, diff_expected = utils.get_differences_between_lists(diff_actual, diff_expected)
        
        # If both lists are now empty, then the only differences between the two scene files were floating point drift.
        if (diff_actual == None and diff_expected == None) or (len(diff_actual) == 0 and len(diff_actual) == 0):
            warnings.warn(f"Floating point drift detected between {expected_file_path} and {actual_file_path}.")
            return diff_actual, diff_expected
            
        # Something has gone wrong if the lists are now somehow different lengths after the comparison.
        assert len(diff_actual) == len(diff_expected), "Scene mismatch - different line counts after truncation"

        # If some entries were removed from both lists but not all, then there was some floating point drift causing
        # differences to appear between the scene files. Provide a warning on that so it can be tracked, then
        # continue on to the next set of list comparisons.
        if len(diff_actual) != initial_diff_actual_len or len(diff_expected) != initial_diff_expected_len:
            warnings.warn(f"Floating point drift detected between {expected_file_path} and {actual_file_path}.")

        return diff_actual, diff_expected

        
    @staticmethod
    def scan_scene_debug_xml_file_for_issues(diff_actual: List[str], diff_expected: List[str],
            actual_hashes_to_skip: List[str], expected_hashes_to_skip: List[str]) -> (List[str], List[str]):
        # Given the differences between the newly generated XML file versus the last known good, and the lists of hashes that were
        # skipped in the non-XML debug scene graph comparison, check if the differences in the XML file are the same as the hashes
        # that were skipped in the non-XML file.
        # Hashes are generated differently on Linux than other platforms right now. Long term this is a problem, it will mean that
        # product assets generated on Linux are different than other platforms. Short term, this is a known issue. This automated
        # test handles this by emitting warnings when this occurs.

        # If the difference count doesn't match the non-XML file, then it's not just hash mis-matches in the XML file, and the test has failed.
        assert len(expected_hashes_to_skip) == len(diff_expected), "Scene mismatch"
        assert len(actual_hashes_to_skip) == len(diff_actual), "Scene mismatch"                

        # This test did a simple line by line comparison, and didn't actually load the XML data into a graph to compare.
        # Which means that the relevant info for this field to make it clear that it is a hash and not another number is not immediately available.
        # So instead, extract the number and compare it to the known list of hashes.
        # If this regex fails or the number isn't in the hash list, then it means this is a non-hash difference and should cause a test failure.
        # Otherwise, if it's just a hash difference, it can be a warning for now, while the information being hashed is not stable across platforms.
        xml_number_regex = re.compile('.*<Class name="AZ::u64" field="m_data" value="([0-9]*)" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"\\/>')

        for list_entry in diff_actual:
            match_result = xml_number_regex.match(list_entry)
            assert match_result, "Scene mismatch"
            data_value = match_result.group(1)
            # This value doesn't match the list of known hash differences, so mark this test as failed.
            assert (data_value in actual_hashes_to_skip), "Scene mismatch"
                
        for list_entry in diff_expected:
            match_result = xml_number_regex.match(list_entry)
            assert match_result, "Scene mismatch"
            data_value = match_result.group(1)
            # This value doesn't match the list of known hash differences, so mark this test as failed.
            assert (data_value in expected_hashes_to_skip), "Scene mismatch"
        return expected_hashes_to_skip, actual_hashes_to_skip


    @staticmethod
    def scan_scene_debug_scene_graph_file_differences_for_issues(diff_actual: List[str], diff_expected: List[str],
            actual_file_path:str, expected_file_path:str) -> (List[str], List[str]):
        # Given the set of differences between two debug scene graph files, check for any known issues and emit warnings.
        # For unknown issues, fail the test. This primarily checks for hashes that are different.
        # Right now, hash generation is sometimes different on Linux from other platforms, and the test assets were generated on Windows,
        # so the hashes may be different when run on Linux. Also, it's been a pain point to need to re-generate debug scene graphs
        # when small changes occur in the scenes. This layer of data changing hasn't been causing issues yet, and is caught by other
        # automated tests focused on the specific set of data. This automated test is to verify that the basic structure of the scene
        # is the same with each run.
        diff_actual_hashes_removed = []
        diff_expected_hashes_removed = []

        hash_regex = re.compile("(.*Hash: )([0-9]*)")

        actual_hashes = []
        expected_hashes = []

        for list_entry in diff_actual:
            match_result = hash_regex.match(list_entry)
            assert match_result, "Scene mismatch"
            diff_actual_hashes_removed.append(match_result.group(1))
            actual_hashes.append(match_result.group(2))
                
        for list_entry in diff_expected:
            match_result = hash_regex.match(list_entry)
            assert match_result, "Scene mismatch"
            diff_expected_hashes_removed.append(match_result.group(1))
            expected_hashes.append(match_result.group(2))

        hashes_removed_diffs_identical = utils.compare_lists(diff_actual_hashes_removed, diff_expected_hashes_removed)

        # If, after removing all of the hash values, the lists are now identical, emit a warning.
        if hashes_removed_diffs_identical == True:
            warnings.warn(f"Hash values no longer match for debug scene graph between files {expected_file_path} and {actual_file_path}")

        return expected_hashes, actual_hashes


    @staticmethod
    def compare_scene_debug_file(asset_processor, expected_file_path: str, actual_file_path: str,
            expected_hashes_to_skip: List[str] = None, actual_hashes_to_skip: List[str] = None):
        # Given the paths to the debug scene graph generated by re-processing the test scene file and the path to the
        # last known good debug scene graph for that file, load both debug scene graphs into memory and scan them for differences.
        # Warns on known issues, and fails on unknown issues.
        debug_graph_path = os.path.join(get_cache_folder(asset_processor), actual_file_path)
        expected_debug_graph_path = os.path.join(asset_processor.project_test_source_folder(), "SceneDebug", expected_file_path)

        logger.info(f"Parsing scene graph: {debug_graph_path}")
        with open(debug_graph_path, "r") as scene_file:
            actual_lines = scene_file.readlines()

        logger.info(f"Parsing scene graph: {expected_debug_graph_path}")
        with open(expected_debug_graph_path, "r") as scene_file:
            expected_lines = scene_file.readlines()
        diff_actual, diff_expected = utils.get_differences_between_lists(actual_lines, expected_lines)
        if diff_actual == None and diff_expected == None:
            return None, None

        # There are some differences that are currently considered warnings.
        # Long term these should become errors, but right now product assets on Linux and non-Linux
        # are showing differences in hashes.
        # Linux and non-Linux platforms are also generating different floating point values.
        diff_actual, diff_expected = TestsFBX_AllPlatforms.trim_floating_point_values_from_same_length_lists(diff_actual, diff_expected, actual_file_path, expected_file_path)

        # If this is the XML debug file, then it will be difficult to verify if a line is a hash line or another integer.
        # However, because XML files are always compared after standard dbgsg files, the hashes from that initial comparison can be used here to check.
        is_xml_dbgsg = os.path.splitext(expected_file_path)[-1].lower() == ".xml"        

        if is_xml_dbgsg:
            return TestsFBX_AllPlatforms.scan_scene_debug_xml_file_for_issues(diff_actual, diff_expected, actual_hashes_to_skip, expected_hashes_to_skip)
        else:
            return TestsFBX_AllPlatforms.scan_scene_debug_scene_graph_file_differences_for_issues(diff_actual, diff_expected, actual_file_path, expected_file_path)


    # Helper to run Asset Processor with debug output enabled and Atom output disabled
    @staticmethod
    def run_ap_debug_skip_atom_output(asset_processor):
        result, output = asset_processor.batch_process(capture_output=True, extra_params=["--debugOutput",
                                                             "--regset=\"/O3DE/SceneAPI/AssetImporter/SkipAtomOutput=true\""])
        # If the test fails, it's helpful to have the output from asset processor in the logs, to track the failure down.
        logger.info(f"Asset Processor Output: {pformat(output)}")
        assert result, "Asset Processor Failed"

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
        self.run_ap_debug_skip_atom_output(asset_processor)

        logger.info(f"Validating assets.")
        assetsToValidate = blackbox_params.override_assets if overrideAsset else blackbox_params.assets

        expected_product_list = []
        for expected_source in assetsToValidate:
            for expected_job in expected_source.jobs:
                for expected_product in expected_job.products:
                    expected_product_list.append(expected_product.product_name)

        cache_folder = get_cache_folder(asset_processor)
        missing_assets, _ = utils.compare_assets_with_cache(expected_product_list,
                                                            cache_folder)

        # If the test is going to fail, print information to help track down the cause of failure.
        if missing_assets:
            logger.info(f"The following assets were missing from cache: {pformat(missing_assets)}")
            in_cache = os.listdir(cache_folder)
            logger.info(f"The cache {cache_folder} contains this content: {pformat(in_cache)}")

        assert not missing_assets, \
            f'The following assets were expected to be in, but not found in cache: {str(missing_assets)}'

        # Load the asset database.
        db_path = os.path.join(asset_processor.temp_asset_root(), workspace.project, "Cache",
                               "assetdb.sqlite")
        cache_root = os.path.dirname(os.path.join(asset_processor.temp_asset_root(), workspace.project, "Cache",
                                                  ASSET_PROCESSOR_PLATFORM_MAP[workspace.asset_processor_platform]))

        if blackbox_params.scene_debug_file:
            scene_debug_file = blackbox_params.override_scene_debug_file if overrideAsset \
                else blackbox_params.scene_debug_file
            expected_hashes_to_skip, actual_hashes_to_skip = self.compare_scene_debug_file(asset_processor, scene_debug_file, blackbox_params.scene_debug_file)

            # Run again for the .dbgsg.xml file
            self.compare_scene_debug_file(asset_processor,
                                          scene_debug_file + ".xml",
                                          blackbox_params.scene_debug_file + ".xml",
                                          expected_hashes_to_skip = expected_hashes_to_skip,
                                          actual_hashes_to_skip = actual_hashes_to_skip)

        # Check that each given source asset resulted in the expected jobs and products.
        self.populate_asset_info(workspace, project, assetsToValidate)
        for expected_source in assetsToValidate:
            for job in expected_source.jobs:
                for product in job.products:
                    asset_db_utils.compare_expected_asset_to_actual_asset(db_path,
                                                                          expected_source,
                                                                          f"{blackbox_params.asset_folder}/"
                                                                          f"{expected_source.source_file_name}",
                                                                          cache_root)

    def test_FBX_ModifiedFBXFile_ConsistentProductOutput_OutputSucceeds(self, workspace, ap_setup_fixture,
                                                                            asset_processor):
        """
        Test verifies changing an FBX file with a custom .assetinfo file produces an expected output
        The .assetinfo file will remain unchanged.

        Test Steps:
            1. Create test environment with an FBX file and a custom .assetinfo file
            2. Launch Asset Processor
            3. Validate that Asset Processor generates the expected output
            4. Modify the FBX file
            5. Run asset processor
            6. Validate that Asset Processor generates the expected output
        """
        # Copying test assets to project folder
        TEST_FOLDER_NAME = "ModifiedFBXFile_ConsistentProductOutput"

        asset_processor.prepare_test_environment(ap_setup_fixture["tests_dir"], TEST_FOLDER_NAME)
        # Run AP against the FBX file and the .assetinfo file
        self.run_ap_debug_skip_atom_output(asset_processor)

        # Set path to expected dbgsg output, copied from test folder
        scene_debug_expected = os.path.join(asset_processor.project_test_source_folder(), "SceneDebug", "modifiedfbxfile.fbx.dbgsg")
        assert os.path.exists(scene_debug_expected), "Expected scene file missing in SceneDebug/modifiedfbxfile.fbx.dbgsg - Check test assets"

        # Set path to actual dbgsg output, obtained when running AP
        scene_debug_actual = os.path.join(
            asset_processor.temp_project_cache(asset_platform=ASSET_PROCESSOR_PLATFORM_MAP[workspace.asset_processor_platform]),
            TEST_FOLDER_NAME.lower(),
            "modifiedfbxfile.fbx.dbgsg")

        assert os.path.exists(scene_debug_actual)

        # Compare the dbgsg files to ensure expected outputs
        expected_hashes_to_skip, actual_hashes_to_skip = self.compare_scene_debug_file(asset_processor, scene_debug_expected, scene_debug_actual)

        # Run again for the .dbgsg.xml files
        self.compare_scene_debug_file(asset_processor, scene_debug_expected + ".xml", scene_debug_actual + ".xml",
            expected_hashes_to_skip=expected_hashes_to_skip, actual_hashes_to_skip=actual_hashes_to_skip)

        # Remove the files to be replaced from the source test asset folder
        filestoremove = [
            os.path.join(asset_processor.project_test_source_folder(),"ModifiedFBXFile.fbx"),
            scene_debug_expected,
            scene_debug_expected + ".xml"
        ]
        for file in filestoremove:
            os.remove(file)
            assert not os.path.exists(file), f"File failed to be removed: {file}"

        # Add the replacement FBX and expected dbgsg files into the test project
        source = os.path.join(ap_setup_fixture["tests_dir"], "assets",
                              "Override_ModifiedFBXFile_ConsistentProductOutput")
        destination = asset_processor.project_test_source_folder()
        shutil.copytree(source, destination, dirs_exist_ok=True)
        assert os.path.exists(scene_debug_expected), \
            "Expected scene file missing in SceneDebug/modifiedfbxfile.fbx.dbgsg - Check test assets"

        # Run AP again to regenerate the .dbgsg files
        self.run_ap_debug_skip_atom_output(asset_processor)

        # Compare the new .dbgsg files with their expected outputs
        expected_hashes_to_skip, actual_hashes_to_skip = self.compare_scene_debug_file(asset_processor, scene_debug_expected, scene_debug_actual)

        # Run again for the .dbgsg.xml file
        self.compare_scene_debug_file(asset_processor, scene_debug_expected + ".xml", scene_debug_actual + ".xml",
            expected_hashes_to_skip=expected_hashes_to_skip, actual_hashes_to_skip=actual_hashes_to_skip)

    def test_FBX_MixedCaseFileExtension_OutputSucceeds(self, workspace, ap_setup_fixture, asset_processor):
        """
        Test verifies FBX file with any combination of mixed casing in its file extension produces the
        expected output.

        Test Steps:
            1. Create test environment with an FBX file
            2. Change the .fbx file extension casing
            2. Launch Asset Processor
            3. Validate that Asset Processor generates the expected output
            4. Modify the FBX file's extension to a different casing
            5. Run asset processor
            6. Validate that Asset Processor generates the expected output
        """

        extensionlist = [
            "OneMeshOneMaterial.Fbx",
            "OneMeshOneMaterial.fBX",
            "OneMeshOneMaterial.FBX",
        ]

        expected_debug_list  = [
            "onemeshonematerial_capf.fbx.dbgsg",
            "onemeshonematerial_capbx.fbx.dbgsg",
            "onemeshonematerial_capfbx.fbx.dbgsg"
        ]

        original_extension = "OneMeshOneMaterial.fbx"

        for (extension, expected_debug_name) in zip(extensionlist, expected_debug_list):
            asset_processor.prepare_test_environment(ap_setup_fixture["tests_dir"], "OneMeshOneMaterial")
            rename_src = os.path.join(asset_processor.project_test_source_folder(), original_extension)
            rename_dst = os.path.join(asset_processor.project_test_source_folder(), extension)
            os.rename(rename_src, rename_dst)
            assert os.path.exists(rename_dst), "Expected test file missing"

            # Run AP while generating debug output and skipping atom output
            self.run_ap_debug_skip_atom_output(asset_processor)

            expectedassets = [
                'onemeshonematerial/onemeshonematerial.fbx.abdata.json',
                'onemeshonematerial/onemeshonematerial.fbx.assetinfo.dbg',
                'onemeshonematerial/onemeshonematerial.fbx.dbgsg',
                'onemeshonematerial/onemeshonematerial.fbx.dbgsg.xml',
                'onemeshonematerial/onemeshonematerial_fbx.procprefab',
                'onemeshonematerial/onemeshonematerial_fbx.procprefab.json'
                ]

            missing_assets, _ = utils.compare_assets_with_cache(expectedassets,
                                                                get_cache_folder(asset_processor))

            assert not missing_assets, \
                f'The following assets were expected to be in when processing {extension}, but not found in cache: ' \
                f'{str(missing_assets)}'

            scene_debug_expected = os.path.join(asset_processor.project_test_source_folder(), "SceneDebug",
                                                expected_debug_name)
            assert os.path.exists(scene_debug_expected), \
                f'Expected scene file missing in {scene_debug_expected} - Check test assets'

            # Set path to actual dbgsg output, obtained when running AP
            scene_debug_actual = os.path.join(asset_processor.temp_project_cache(
                asset_platform=ASSET_PROCESSOR_PLATFORM_MAP[workspace.asset_processor_platform]),
                                              "onemeshonematerial", "onemeshonematerial.fbx.dbgsg")
            assert os.path.exists(scene_debug_actual), f"Scene debug output missing after running AP on {extension}."

            expected_hashes_to_skip, actual_hashes_to_skip = self.compare_scene_debug_file(asset_processor, scene_debug_expected, scene_debug_actual)

            # Run again for the .dbgsg.xml file
            self.compare_scene_debug_file(asset_processor,
                                          scene_debug_expected + ".xml",
                                          scene_debug_actual + ".xml",
                                          expected_hashes_to_skip = expected_hashes_to_skip,
                                          actual_hashes_to_skip = actual_hashes_to_skip)
