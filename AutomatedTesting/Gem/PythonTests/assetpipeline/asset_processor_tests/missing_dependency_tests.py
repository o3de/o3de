"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Automated scripts for testing the AssetProcessorBatch's missing dependency scanner feature.

"""

import pytest
import subprocess
import logging
import os
from typing import List, Tuple

from ..ap_fixtures.asset_processor_fixture import asset_processor
from ..ap_fixtures.ap_setup_fixture import ap_setup_fixture
from ly_test_tools.o3de.asset_processor import ASSET_PROCESSOR_PLATFORM_MAP
from ly_test_tools.o3de import asset_processor as asset_processor_utils

# fmt:off
from ..ap_fixtures.ap_missing_dependency_fixture \
    import ap_missing_dependency_fixture as missing_dep_helper
# fmt:on

# Import LyTestTools
import ly_test_tools.builtin.helpers as helpers

# Import LyShared
import ly_test_tools.o3de.pipeline_utils as utils
from automatedtesting_shared import asset_database_utils as db_utils

# Use the following logging pattern to hook all test logging together:
logger = logging.getLogger(__name__)
# Configuring the logging is done in ly_test_tools at the following location:
# ~/dev/Tools/LyTestTools/ly_test_tools/log/py_logging_util.py

# Helper: variables we will use for parameter values in the test:
targetProjects = ["AutomatedTesting"]


@pytest.fixture
def local_resources(request, workspace, ap_setup_fixture):
    # Test-level asset folder. Directory contains a subfolder for each test (i.e. C1234567)
    ap_setup_fixture["tests_dir"] = os.path.dirname(os.path.realpath(__file__))


@pytest.mark.parametrize("project", targetProjects)
@pytest.mark.assetpipeline
@pytest.mark.usefixtures("asset_processor")
@pytest.mark.usefixtures("workspace")
@pytest.mark.SUITE_periodic
class TestsMissingDependencies_WindowsAndMac(object):

    @pytest.fixture(autouse=True)
    def setup_teardown(self, asset_processor, missing_dep_helper, workspace):
        self._workspace = workspace
        self._asset_processor = asset_processor
        self._missing_dep_helper = missing_dep_helper
        self._asset_processor.create_temp_asset_root()
        self._asset_processor.add_source_folder_assets(f"AutomatedTesting\\TestAssets")
        missing_dep_helper.asset_db = os.path.join(asset_processor.temp_asset_root(), "Cache",
                                                   "assetdb.sqlite")
        self._asset_processor.add_source_folder_assets(f"{self._workspace.project}\\Slices")
        self._asset_processor.add_source_folder_assets(f"{self._workspace.project}\\Materials")
        self._asset_processor.add_source_folder_assets(f"{self._workspace.project}\\textures")
        self._asset_processor.add_source_folder_assets(f"{self._workspace.project}\\UI")
        self._asset_processor.add_source_folder_assets(f"{self._workspace.project}\\libs\\particles")

    def do_missing_dependency_test(self, source_product, expected_dependencies,
                                   dsp_param,
                                   platforms=None, max_iterations=0):

        platforms = platforms or ASSET_PROCESSOR_PLATFORM_MAP[self._workspace.asset_processor_platform]
        if not isinstance(platforms, list):
            platforms = [platforms]

        self._asset_processor.batch_process(platforms=platforms)

        """Run a single test"""
        for asset_platform in platforms:
            db_product = db_utils.get_product_id_from_relative(self._workspace, source_product, asset_platform)
            if db_product:
                db_utils.clear_missing_dependencies(self._missing_dep_helper.asset_db, db_product)
        expected_product = os.path.join(self._workspace.project, source_product).lower()

        dependency_search_params = [f"--dsp={dsp_param}", "--zeroAnalysisMode"]
        if max_iterations:
            dependency_search_params.append(f"--dependencyScanMaxIteration={max_iterations}")

        logger.info(f"Running apbatch dependency search for {platforms} dependency search {dependency_search_params}")
        _, ap_batch_output = self._asset_processor.batch_process(capture_output=True,
                                                              extra_params=dependency_search_params)
        self._missing_dep_helper.validate_expected_dependencies(expected_product, expected_dependencies, ap_batch_output,
                                                          platforms)

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C17226567")
    def test_WindowsAndMac_ValidUUIDNotDependency_ReportsMissingDependency(self):
        """Tests that a valid UUID referenced in a file will report any missing dependencies"""

        # Relative path to the txt file with missing dependencies
        expected_product = f"testassets\\validuuidsnotdependency.txt"
        self._asset_processor.add_source_folder_assets(f"{self._workspace.project}\\Objects\\LumberTank")
        self._asset_processor.add_source_folder_assets(f"{self._workspace.project}\\Objects\\Characters\\Jack")
        # Expected missing dependencies
        expected_dependencies = [
            #            String                                      Asset                     #
            ("06E9D6633C875400A532BCB2C0CA19D6", "{06E9D663-3C87-5400-A532-BCB2C0CA19D6}:0"),
            ("1CB10C43F3245B93A294C602ADEF95F9:[0", "{1CB10C43-F324-5B93-A294-C602ADEF95F9}:0"),
            ("58BE9DA51F1753B98CEEEEB10E63454D", "{58BE9DA5-1F17-53B9-8CEE-EEB10E63454D}:914f19b7"),
            ("6BDE282B49C957F7B0714B26579BCA9A", "{6BDE282B-49C9-57F7-B071-4B26579BCA9A}:0"),
            ("747D31D71E62553592226173C49CF97E", "{747D31D7-1E62-5535-9222-6173C49CF97E}:1"),
            ("747D31D71E62553592226173C49CF97E", "{747D31D7-1E62-5535-9222-6173C49CF97E}:2"),
            ("9886E132-572D-5746-9377-E629AB6C1981", "{9886E132-572D-5746-9377-E629AB6C1981}:0"),
            ("33bcee02F3225688ABEE534F6058593F", "{33BCEE02-F322-5688-ABEE-534F6058593F}:0"),
            ("B92667DC-9F5B-5D72-A29D-99219DD9B691", "{B92667DC-9F5B-5D72-A29D-99219DD9B691}:0"),
            ("D92C4661C8985E19BD3597CB2318CFA6:[0", "{D92C4661-C898-5E19-BD35-97CB2318CFA6}:0"),
            ("7364AB2B092F5B0B80601BBC6E53087C", "{7364AB2B-092F-5B0B-8060-1BBC6E53087C}:0"),
        ]
        self.do_missing_dependency_test(expected_product, expected_dependencies,
                                        "%ValidUUIDsNotDependency.txt")

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C17226567")
    def test_WindowsAndMac_InvalidUUIDsNotDependencies_NoReportedMessage(self):
        """Tests that invalid UUIDs do not count as missing dependencies"""
        # Relative path to the txt file with invalid UUIDs
        expected_product = f"testassets\\invaliduuidnoreport.txt"
        expected_dependencies = []  # No expected missing dependencies

        self.do_missing_dependency_test(expected_product, expected_dependencies,
                                        "%InvalidUUIDNoReport.txt")

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C17226567")
    def test_WindowsAndMac_ValidAssetIdsNotDependencies_ReportsMissingDependency(self):
        """Tests that valid asset IDs but not dependencies, show missing dependencies"""

        # Relative path to the txt file with valid asset ids but not dependencies
        expected_product = f"testassets\\validassetidnotdependency.txt"

        # Expected missing dependencies
        expected_dependencies = [
            #            String                                                  Asset                     #
            ("2ef92b8D044E5C278E2BB1AC0374A4E7:131072", "{2EF92B8D-044E-5C27-8E2B-B1AC0374A4E7}:20000"),
            ("A2482826-053D-5634-A27B-084B1326AAE5}:[196608", "{A2482826-053D-5634-A27B-084B1326AAE5}:30000"),
            ("D83B36F1-61A6-5001-B191-4D0CE282E236}-327680", "{D83B36F1-61A6-5001-B191-4D0CE282E236}:50000"),
        ]

        self.do_missing_dependency_test(expected_product, expected_dependencies,
                                        "%ValidAssetIdNotDependency.txt")

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C17226567")
    def test_WindowsAndMac_InvalidAssetsIDNotDependencies_NoReportedMessage(self):
        """Tests that invalid asset IDs do not count as missing dependencies"""

        # Relative path to the txt file with invalid asset IDs
        expected_product = f"testassets\\invalidassetidnoreport.txt"
        expected_dependencies = []

        self.do_missing_dependency_test(expected_product, expected_dependencies,
                                        "%InvalidAssetIdNoReport.txt")

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C17226567")
    # fmt:off
    def test_WindowsAndMac_ValidSourcePathsNotDependencies_ReportsMissingDependencies(self):
        # fmt:on
        """Tests that valid source paths can translate to missing dependencies"""

        # Relative path to the txt file with missing dependencies as source paths
        expected_product = f"testassets\\relativesourcepathsnotdependencies.txt"

        # Expected missing dependencies
        expected_dependencies = [
            #            String                               Asset                     #
            ("Config/Editor.xml", "{06E9D663-3C87-5400-A532-BCB2C0CA19D6}:0"),
            (r"TestAssets\WildcardScanTest1.txt", "{1CB10C43-F324-5B93-A294-C602ADEF95F9}:0"),
            ("TestAssets/RelativeProductPathsNotDependencies.txt", "{B772953C-A08A-5D20-9491-530E87D11504}:0"),
            ("textures/_dev_Purple.tif", "{A2482826-053D-5634-A27B-084B1326AAE5}:0"),
            ("textures/_dev_Purple.tif", "{A2482826-053D-5634-A27B-084B1326AAE5}:10000"),
            ("textures/_dev_Purple.tif", "{A2482826-053D-5634-A27B-084B1326AAE5}:20000"),
            ("textures/_dev_Purple.tif", "{A2482826-053D-5634-A27B-084B1326AAE5}:30000"),
            ("textures/_dev_Purple.tif", "{A2482826-053D-5634-A27B-084B1326AAE5}:40000"),
            ("textures/_dev_Purple.tif", "{A2482826-053D-5634-A27B-084B1326AAE5}:50000"),
            ("Config/gAME.XML", "{B92667DC-9F5B-5D72-A29D-99219DD9B691}:0"),
        ]

        self.do_missing_dependency_test(expected_product, expected_dependencies,
                                        "%RelativeSourcePathsNotDependencies.txt")

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C17226567")
    def test_WindowsAndMac_InvalidARelativePathsNotDependencies_NoReportedMessage(self):
        """Tests that invalid relative paths do not resolve to missing dependencies"""

        # Relative path to the txt file with invalid relative paths
        expected_product = f"testassets\\invalidrelativepathsnoreport.txt"
        expected_dependencies = []

        self.do_missing_dependency_test(expected_product, expected_dependencies,
                                        "%InvalidRelativePathsNoReport.txt")

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C17226567")
    # fmt:off
    def test_WindowsAndMac_ValidProductPathsNotDependencies_ReportsMissingDependencies(self):
        # fmt:on
        """Tests that valid product paths can resolve to missing dependencies"""

        self._asset_processor.add_source_folder_assets(f"Gems\\LyShineExamples\\Assets\\UI\\Fonts\\LyShineExamples")
        self._asset_processor.add_scan_folder(f"Gems\\LyShineExamples\\Assets")
        # Relative path to the txt file with missing dependencies as product paths
        expected_product = f"testassets\\relativeproductpathsnotdependencies.txt"
        expected_dependencies = [
            #            String                                 Asset                     #
            ("materials/floor_tile.mtl", "{0EFF5E4A-F544-5D87-8696-6DDFA62D6063}:0"),
            ("materials/am_grass1.mtl", "{1151F14D-38A6-5579-888A-BE3139882E68}:0"),
            ("2ef92b8D044E5C278E2BB1AC0374A4E7:131072", "{2EF92B8D-044E-5C27-8E2B-B1AC0374A4E7}:20000"),
            ("ui/milestone2menu.uicanvas", "{445D9AF3-6CA5-5281-82A9-5C570BCD1DB8}:0"),
            ("ui/fonts/lyshineexamples/vera.ttf", "{74F5C29E-4749-5EE8-AEC6-A1C540600CE7}:0"),
            ("materials/am_rockground.mtl", "{A1DA3D05-A020-5BB5-A608-C4812B7BD733}:0"),
            ("textures/_dev_yellow_light.dds.2", "{6C40868F-3FC1-5115-96EA-DD0A9E33DEE4}:20000"),
            (r"automatedtesting\textures\_dev_stucco.dds", "{70114D85-D712-5AEB-A816-8FE3A37087AF}:0"),
            ("textures/milestone2/ama_grey_02.dds", "{3EE80AAD-EB9C-56BD-9E9C-65410578998C}:0"),
            (r"textures\\_dev_tan.dds", "{8F2BCEF5-C8CE-5B80-8103-8C1D694D012C}:0"),
            ("textures/_dev_purple.dds", "{A2482826-053D-5634-A27B-084B1326AAE5}:0"),
            ("TEXTURES/_DEV_WHITE.dds", "{D83B36F1-61A6-5001-B191-4D0CE282E236}:0"),
            ("textures/_dev_woodland.dds", "{F3DD193C-5845-569C-A974-AA338B30CF86}:0"),
            ("A2482826-053D-5634-A27B-084B1326AAE5}:[196608", "{A2482826-053D-5634-A27B-084B1326AAE5}:30000"),
            ("B92667DC-9F5B-5D72-A29D-99219DD9B691", "{B92667DC-9F5B-5D72-A29D-99219DD9B691}:0"),
            ("CEAA362B4E505BCEB827CB92EF40A50E", "{CEAA362B-4E50-5BCE-B827-CB92EF40A50E}:1"),
            ("CEAA362B4E505BCEB827CB92EF40A50E", "{CEAA362B-4E50-5BCE-B827-CB92EF40A50E}:2"),
            ("ui/fonts/lyshineexamples/veramono.ttf", "{BAD7FDC5-7BA6-5490-95AA-89078E2FA876}:0"),
        ]

        self.do_missing_dependency_test(expected_product, expected_dependencies,
                                        "%RelativeProductPathsNotDependencies.txt")

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C17226567")
    def test_WindowsAndMac_WildcardScan_FindsAllExpectedFiles(self):
        """Tests that the wildcard scanning will pick up multiple files"""

        helper = self._missing_dep_helper

        # Relative paths to the txt file with no missing dependencies
        expected_product_1 = f"{self._workspace.project}\\testassets\\wildcardscantest1.txt"
        expected_product_2 = f"{self._workspace.project}\\testassets\\wildcardscantest2.txt"
        expected_dependencies = []  # Neither file has expected missing dependencies

        # Run missing dependency scanner and validate results for both files
        cache_platform = ASSET_PROCESSOR_PLATFORM_MAP[self._workspace.asset_processor_platform]
        _, ap_batch_output = self._asset_processor.batch_process(capture_output=True,
                                                           extra_params=["/dsp=%WildcardScanTest%.txt"])

        helper.validate_expected_dependencies(expected_product_1, expected_dependencies, ap_batch_output,
                                              [cache_platform])
        helper.validate_expected_dependencies(expected_product_2, expected_dependencies, ap_batch_output,
                                              [cache_platform])

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C17226567")
    # fmt:off
    def test_WindowsAndMac_AssetReportsAllButOneReference_ReportsOneMissingDependency(self):
        # fmt:on
        """
        Tests that a file with many dependencies but only one missing has only the one missing dependency reported.
        This slice file uses entity names with UUIDs and relative paths to resemble references.
        For these references that are valid, all but one have available, matching dependencies. This test is
        primarily meant to verify that the missing dependency reporter checks the product dependency table before
        emitting missing dependencies.
        """
        # Relative path to target test file
        expected_product = f"testassets\\dependencyscannerasset.dynamicslice"

        # The only expected missing dependency
        expected_dependencies = [("Config/Game.xml", "{B92667DC-9F5B-5D72-A29D-99219DD9B691}:0")]

        self.do_missing_dependency_test(expected_product, expected_dependencies,
                                        "%DependencyScannerAsset%.dynamicslice")

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C17226567")
    def test_WindowsAndMac_ReferencesSelfPath_NoReportedMessage(self):
        """Tests that a file that references itself via relative path does not report itself as a missing dependency"""
        # Relative path to file that references itself via relative path
        expected_product = f"testassets\\selfreferencepath.txt"
        expected_dependencies = []

        self.do_missing_dependency_test(expected_product, expected_dependencies,
                                        "%SelfReferencePath.txt")

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C17226567")
    def test_WindowsAndMac_ReferencesSelfUUID_NoReportedMessage(self):
        """Tests that a file that references itself via its UUID does not report itself as a missing dependency"""

        # Relative path to file that references itself via its UUID
        expected_product = f"testassets\\selfreferenceuuid.txt"
        expected_dependencies = []

        self.do_missing_dependency_test(expected_product, expected_dependencies,
                                        "%SelfReferenceUUID.txt")

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C17226567")
    def test_WindowsAndMac_ReferencesSelfAssetID_NoReportedMessage(self):
        """Tests that a file that references itself via its Asset ID does not report itself as a missing dependency"""

        # Relative path to file that references itself via its Asset ID
        expected_product = f"testassets\\selfreferenceassetid.txt"
        expected_dependencies = []

        self.do_missing_dependency_test(expected_product, expected_dependencies,
                                        "%SelfReferenceAssetID.txt")

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C17226567")
    def test_WindowsAndMac_FileExceedsScanLimit_NoMissingDependenciesFound(self):
        """
        Tests that the scan limit fails to find a missing dependency that is out of reach.
        The max iteration count is set to just under where a valid missing dependency is on a line in the file,
        so this will not report any missing dependencies.
        """

        # Relative path to file that has a missing dependency at 31 iterations deep
        expected_product = f"testassets\\maxiteration31deep.txt"
        expected_dependencies = []

        self.do_missing_dependency_test(expected_product, expected_dependencies,
                                        "%MaxIteration31Deep.txt", max_iterations=30)

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C17226567")
    def test_WindowsAndMac_ScanLimitNotExceeded_ReportsOneMissingDependency(self):
        """
        Tests that the scan limit succeeds in finding a missing dependency that is barely in reach.
        In the previous test, the scanner was set to stop recursion just before a missing dependency was found.
        This test runs with the recursion limit set deep enough to actually find the missing dependency.
        """
        # Relative path to file that has a missing dependency at 31 iterations deep
        expected_product = f"testassets\\maxiteration31deep.txt"

        # Expected missing dependency hiding 31 dependencies deep
        expected_dependencies = [
            #            String                                        Asset                     #
            ("B92667DC-9F5B-5D72-A29D-99219DD9B691", "{B92667DC-9F5B-5D72-A29D-99219DD9B691}:0")
        ]

        self.do_missing_dependency_test(expected_product, expected_dependencies,
                                        "%MaxIteration31Deep.txt", max_iterations=31)

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C17226567")
    # fmt:off
    def test_WindowsAndMac_PotentialMatchesLongerThanUUIDString_OnlyReportsCorrectLengthUUIDs(self):
        # fmt:on
        """Tests that dependency references that are longer than expected are ignored"""

        # Relative path to text file with varying length UUID references
        expected_product = f"testassets\\onlymatchescorrectlengthuuids.txt"
        self._asset_processor.add_source_folder_assets(f"{self._workspace.project}\\Objects\\Characters\\Jack")
        # Expected dependencies with valid lengths from file
        expected_dependencies = [
            #            String                                           Asset                   #
            ("D92C4661C8985E19BD3597CB2318CFA6", "{D92C4661-C898-5E19-BD35-97CB2318CFA6}:0"),
            ("58BE9DA51F1753B98CEEEEB10E63454D", "{58BE9DA5-1F17-53B9-8CEE-EEB10E63454D}:914f19b7"),
            ("747D31D71E62553592226173C49CF97E", "{747D31D7-1E62-5535-9222-6173C49CF97E}:1"),
            ("747D31D71E62553592226173C49CF97E", "{747D31D7-1E62-5535-9222-6173C49CF97E}:2"),
            ("1CB10C43-F324-5B93-A294-C602ADEF95F9", "{1CB10C43-F324-5B93-A294-C602ADEF95F9}:0"),
        ]

        self.do_missing_dependency_test( expected_product, expected_dependencies,
                                        "%OnlyMatchesCorrectLengthUUIDs.txt")

    @pytest.mark.assetpipeline
    @pytest.mark.usefixtures("ap_setup_fixture")
    @pytest.mark.usefixtures("local_resources")
    @pytest.mark.test_case_id("C24255732")
    def test_WindowsAndMac_MissingDependencyScanner_GradImageSuccess(
            self, ap_setup_fixture
    ):
        """Tests the Missing Dependency Scanner can scan gradimage files"""

        env = ap_setup_fixture
        helper = self._missing_dep_helper
        filename = "DependencyScannerAssetGradImage"

        # Move the necessary files over to the project and confirm it was moved successfully
        test_assets_folder, cache_folder = self._asset_processor.prepare_test_environment(env["tests_dir"], "C24255732",
                                                                                          use_current_root=True)

        # fmt:off
        assert os.path.exists(os.path.join(test_assets_folder, f"{filename}.gradimage")), \
            f"{filename} does not exist in {test_assets_folder} folder."
        # fmt:on

        # Construct asset processor batch command

        _, ap_batch_output = self._asset_processor.batch_process(capture_output=True,
                                                           extra_params=["/dsp=%{filename}%.gradimage"])

        def validate_expected_dependencies(product: str, expected_dependencies: List[Tuple[str, str]],
                                           log_output: List[str or List[str]]) -> None:
            # Check dependencies found in the log
            for product_name, missing_deps in helper.extract_missing_dependencies_from_output(log_output).items():
                if product in product_name:
                    # fmt:off
                    assert sorted(missing_deps) == sorted(expected_dependencies), \
                        f"Missing dependencies did not match expected. Expected: " \
                        f"{expected_dependencies}, Actual: {missing_deps}"
                    # fmt:on

        expected_product = os.path.join(test_assets_folder, f"{filename}.gradimage")
        expected_dependencies = []
        validate_expected_dependencies(expected_product, expected_dependencies, ap_batch_output)
