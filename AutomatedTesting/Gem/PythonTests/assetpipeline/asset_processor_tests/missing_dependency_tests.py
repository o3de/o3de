"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

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
        missing_dep_helper.asset_db = os.path.join(asset_processor.temp_asset_root(), self._workspace.project, "Cache",
                                                   "assetdb.sqlite")
        self._asset_processor.add_source_folder_assets(f"{self._workspace.project}\\Prefabs")
        self._asset_processor.add_source_folder_assets(f"{self._workspace.project}\\Materials")
        self._asset_processor.add_source_folder_assets(f"{self._workspace.project}\\textures")
        self._asset_processor.add_source_folder_assets(f"{self._workspace.project}\\UI")
        self._asset_processor.add_source_folder_assets(f"{self._workspace.project}\\libs\\particles")

    def do_missing_dependency_test(self, source_product, expected_dependencies,
                                   dsp_param,
                                   platforms=None, max_iterations=0):
        """
        Test Steps:
        1. Determine what platforms to run against
        2. Process assets for that platform
        3. Determine the missing dependency params to set
        4. Set the max iteration param
        5. Run missing dependency scanner against target platforms and search params based on test data
        6. Validate missing dependencies against test data
        """

        platforms = platforms or ASSET_PROCESSOR_PLATFORM_MAP[self._workspace.asset_processor_platform]
        if not isinstance(platforms, list):
            platforms = [platforms]

        self._asset_processor.batch_process(platforms=platforms)

        """Run a single test"""
        for asset_platform in platforms:
            db_product_path = db_utils.get_db_product_path(self._workspace, source_product, asset_platform)
            db_product = db_utils.get_product_id(self._missing_dep_helper.asset_db, db_product_path)
            if db_product:
                db_utils.clear_missing_dependencies(self._missing_dep_helper.asset_db, db_product)
        expected_product = source_product.lower()

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
        """
        Tests that a valid UUID referenced in a file will report any missing dependencies

        Test Steps:
        1. Set the expected product
        2. Set the expected missing dependencies
        3. Execute test
        """

        # Relative path to the txt file with missing dependencies
        expected_product = f"testassets\\validuuidsnotdependency.txt"
        # Expected missing dependencies
        expected_dependencies = [
            #            String                                      Asset                     #
            # InvalidAssetIdNoReport.txt
            ('E68A85B0-131D-5A82-B2D5-BC58EE4062AE', '{E68A85B0-131D-5A82-B2D5-BC58EE4062AE}:0'),
            # InvalidRelativePathsNoReport.txt
            ('B3EF12DD306C520EB0A8A6B0D031A195', '{B3EF12DD-306C-520E-B0A8-A6B0D031A195}:0'),
            # SelfReferenceUUID.txt
            ('33bcee02F3225688ABEE534F6058593F', '{33BCEE02-F322-5688-ABEE-534F6058593F}:0'),
            # SelfReferencePath.txt
            ('DD587FBE-16C8-5B98-AE3C-A9F8750B2692', '{DD587FBE-16C8-5B98-AE3C-A9F8750B2692}:0'),
            # InvalidUUIDNoReport.txt
            ('837412DF-D05F-576D-81AA-ACF360463749', '{837412DF-D05F-576D-81AA-ACF360463749}:0'),
            # MaxIteration31Deep.txt
            ('3F642A0FDC825696A70A1DA5709744DF', '{3F642A0F-DC82-5696-A70A-1DA5709744DF}:0'),
            # OnlyMatchesCorrectLengthUUIDs.txt
            ('2545AD8B-1B9B-5F93-859D-D8DC1DC2B480', '{2545AD8B-1B9B-5F93-859D-D8DC1DC2B480}:0'),
            # WildcardScanTest1.txt
            ('1CB10C43F3245B93A294C602ADEF95F9:[0', '{1CB10C43-F324-5B93-A294-C602ADEF95F9}:0'),
            # RelativeProductPathsNotDependencies.txt
            ('B772953CA08A5D209491530E87D11504:[0', '{B772953C-A08A-5D20-9491-530E87D11504}:0'),
            # WildcardScanTest2.txt
            ('D92C4661C8985E19BD3597CB2318CFA6', '{D92C4661-C898-5E19-BD35-97CB2318CFA6}:0'),
        ]
        self.do_missing_dependency_test(expected_product, expected_dependencies,
                                        "%ValidUUIDsNotDependency.txt")

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C17226567")
    def test_WindowsAndMac_InvalidUUIDsNotDependencies_NoReportedMessage(self):
        """
        Tests that invalid UUIDs do not count as missing dependencies

        Test Steps:
        1. Set the expected product
        2. Set the expected missing dependencies
        3. Execute test
        """
        # Relative path to the txt file with invalid UUIDs
        expected_product = f"testassets\\invaliduuidnoreport.txt"
        expected_dependencies = []  # No expected missing dependencies

        self.do_missing_dependency_test(expected_product, expected_dependencies,
                                        "%InvalidUUIDNoReport.txt")

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C17226567")
    def test_WindowsAndMac_ValidAssetIdsNotDependencies_ReportsMissingDependency(self):
        """
        Tests that valid asset IDs but not dependencies, show missing dependencies

        Test Steps:
        1. Set the expected product
        2. Set the expected missing dependencies
        3. Execute test
        """

        # Relative path to the txt file with valid asset ids but not dependencies
        expected_product = f"testassets\\validassetidnotdependency.txt"

        # Expected missing dependencies
        expected_dependencies = [
            #            String                                                  Asset                     #
            # _dev_Red.tif
            ('2ef92b8D044E5C278E2BB1AC0374A4E7:1000', '{2EF92B8D-044E-5C27-8E2B-B1AC0374A4E7}:3e8'),
            # _dev_Purple.tif
            ('A2482826-053D-5634-A27B-084B1326AAE5}:[1002', '{A2482826-053D-5634-A27B-084B1326AAE5}:3ea'),
            # _dev_White.tif
            ('D83B36F1-61A6-5001-B191-4D0CE282E236}-1002', '{D83B36F1-61A6-5001-B191-4D0CE282E236}:3ea'),
        ]

        self.do_missing_dependency_test(expected_product, expected_dependencies,
                                        "%ValidAssetIdNotDependency.txt")

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C17226567")
    def test_WindowsAndMac_InvalidAssetsIDNotDependencies_NoReportedMessage(self):
        """
        Tests that invalid asset IDs do not count as missing dependencies

        Test Steps:
        1. Set the expected product
        2. Set the expected missing dependencies
        3. Execute test
        """

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
        """
        Tests that valid source paths can translate to missing dependencies

        Test Steps:
        1. Set the expected product
        2. Set the expected missing dependencies
        3. Execute test
        """

        # Relative path to the txt file with missing dependencies as source paths
        expected_product = f"testassets\\relativesourcepathsnotdependencies.txt"

        # Expected missing dependencies
        expected_dependencies = [
            #            String                               Asset                     #
            ('TestAssets\\WildcardScanTest1.txt', '{1CB10C43-F324-5B93-A294-C602ADEF95F9}:0'),
            ('TESTASSETS/ReportONEmISSINGdEPENDENCY.tXT', '{BE5E2373-245E-59E4-B4C6-7370EEAA2EFD}:0'),
            ('textures/_dev_Purple.tif', '{A2482826-053D-5634-A27B-084B1326AAE5}:3e8'),
            ('textures/_dev_Purple.tif', '{A2482826-053D-5634-A27B-084B1326AAE5}:3ea'),
            ('TestAssets/InvalidAssetIdNoReport.txt', '{E68A85B0-131D-5A82-B2D5-BC58EE4062AE}:0'),
            ('TestAssets/RelativeProductPathsNotDependencies.txt', '{B772953C-A08A-5D20-9491-530E87D11504}:0'),
        ]

        self.do_missing_dependency_test(expected_product, expected_dependencies,
                                        "%RelativeSourcePathsNotDependencies.txt")

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C17226567")
    def test_WindowsAndMac_InvalidARelativePathsNotDependencies_NoReportedMessage(self):
        """
        Tests that invalid relative paths do not resolve to missing dependencies

        Test Steps:
        1. Set the expected product
        2. Set the expected missing dependencies
        3. Execute test
        """

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
        """
        Tests that valid product paths can resolve to missing dependencies

        Test Steps:
        1. Set the expected product
        2. Set the expected missing dependencies
        3. Execute test
        """
        # Relative path to the txt file with missing dependencies as product paths
        expected_product = f"testassets\\relativeproductpathsnotdependencies.txt"
        expected_dependencies = [
            #            String                                 Asset                     #
            ('textures/_dev_purple.tif.streamingimage', '{A2482826-053D-5634-A27B-084B1326AAE5}:3e8'),
            ('textures\\_dev_stucco.tif.streamingimage', '{70114D85-D712-5AEB-A816-8FE3A37087AF}:3e8'),
            ('textures\\\\_dev_tan.tif.streamingimage', '{8F2BCEF5-C8CE-5B80-8103-8C1D694D012C}:3e8'),
            ('TEXTURES/_DEV_WHITE.tif.streamingimage', '{D83B36F1-61A6-5001-B191-4D0CE282E236}:3e8'),
            ('textures/_dev_yellow_light.tif.1002.imagemipchain', '{6C40868F-3FC1-5115-96EA-DD0A9E33DEE4}:3ea'),
            ('textures/_dev_woodland.tif.1002.imagemipchain', '{F3DD193C-5845-569C-A974-AA338B30CF86}:3ea'),
            ('textures/_dev_woodland.tif.streamingimage', '{F3DD193C-5845-569C-A974-AA338B30CF86}:3e8'),
            ('textures/_dev_yellow_light.tif.streamingimage', '{6C40868F-3FC1-5115-96EA-DD0A9E33DEE4}:3e8'),
            ('textures/_dev_yellow_med.tif.1002.imagemipchain', '{BB4DFF57-52BD-525B-9628-68232E31802C}:3ea'),
            ('textures/lights/flare01.tif.streamingimage', '{D8E49CC4-C743-5F31-A1EC-4AA89163B8F5}:3e8'),
            # SelfReferenceUUID.txt
            ('33BCEE02-F322-5688-ABEE-534F6058593F', '{33BCEE02-F322-5688-ABEE-534F6058593F}:0'),
            ('textures/test_texture_sequence/test_texture_sequence000.png.streamingimage', '{6CC90BEE-0A9F-57A8-9013-7C1D643C0E8E}:3e8'),
            # _dev_red.tif.streamingimage
            ('2ef92b8D044E5C278E2BB1AC0374A4E7:1002', '{2EF92B8D-044E-5C27-8E2B-B1AC0374A4E7}:3ea'),
            # SelfReferenceAssetID.txt
            ('785A05D2-483E-5B43-A2B9-92ACDAE6E938', '{785A05D2-483E-5B43-A2B9-92ACDAE6E938}:0'),
            ('textures/test_texture_sequence/test_texture_sequence001.png.streamingimage', '{8A8A37DD-01B9-5D70-92E4-925E2C0FE826}:3e8'),
            # _dev_purple.tif.1002.imagemipchain
            ('A2482826-053D-5634-A27B-084B1326AAE5}:[1002', '{A2482826-053D-5634-A27B-084B1326AAE5}:3ea'),
            ('textures/_dev_purple_glass.tif.1002.imagemipchain', '{2FCDD831-77D1-5BE1-A4C8-CA47E4F89F19}:3ea'),
        ]

        self.do_missing_dependency_test(expected_product, expected_dependencies,
                                        "%RelativeProductPathsNotDependencies.txt")

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C17226567")
    def test_WindowsAndMac_WildcardScan_FindsAllExpectedFiles(self):
        """
        Tests that the wildcard scanning will pick up multiple files

        Test Steps:
        1. Set the expected product
        2. Set the expected missing dependencies
        3. Execute test
        """

        helper = self._missing_dep_helper

        # Relative paths to the txt file with no missing dependencies
        expected_product_1 = f"testassets\\wildcardscantest1.txt"
        expected_product_2 = f"testassets\\wildcardscantest2.txt"
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
        This file uses entity names with UUIDs and relative paths to resemble references.
        For these references that are valid, all but one have available, matching dependencies. This test is
        primarily meant to verify that the missing dependency reporter checks the product dependency table before
        emitting missing dependencies.

        Test Steps:
        1. Set the expected product
        2. Set the expected missing dependencies
        3. Execute test
        """
        # Relative path to target test file
        expected_product = f"testassets\\reportonemissingdependency.txt"

        # The only expected missing dependency
        expected_dependencies = [('6BDE282B49C957F7B0714B26579BCA9A', '{6BDE282B-49C9-57F7-B071-4B26579BCA9A}:0'),]

        self.do_missing_dependency_test(expected_product, expected_dependencies,
                                        "%reportonemissingdependency.txt")

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C17226567")
    def test_WindowsAndMac_ReferencesSelfPath_NoReportedMessage(self):
        """
        Tests that a file that references itself via relative path does not report itself as a missing dependency

        Test Steps:
        1. Set the expected product
        2. Set the expected missing dependencies
        3. Execute test
        """
        # Relative path to file that references itself via relative path
        expected_product = f"testassets\\selfreferencepath.txt"
        expected_dependencies = []

        self.do_missing_dependency_test(expected_product, expected_dependencies,
                                        "%SelfReferencePath.txt")

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C17226567")
    def test_WindowsAndMac_ReferencesSelfUUID_NoReportedMessage(self):
        """
        Tests that a file that references itself via its UUID does not report itself as a missing dependency

        Test Steps:
        1. Set the expected product
        2. Set the expected missing dependencies
        3. Execute test
        """

        # Relative path to file that references itself via its UUID
        expected_product = f"testassets\\selfreferenceuuid.txt"
        expected_dependencies = []

        self.do_missing_dependency_test(expected_product, expected_dependencies,
                                        "%SelfReferenceUUID.txt")

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C17226567")
    def test_WindowsAndMac_ReferencesSelfAssetID_NoReportedMessage(self):
        """
        Tests that a file that references itself via its Asset ID does not report itself as a missing dependency

        Test Steps:
        1. Set the expected product
        2. Set the expected missing dependencies
        3. Execute test
        """

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

        Test Steps:
        1. Set the expected product
        2. Set the expected missing dependencies
        3. Execute test
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

        Test Steps:
        1. Set the expected product
        2. Set the expected missing dependencies
        3. Execute test
        """

        # Relative path to file that has a missing dependency at 31 iterations deep
        expected_product = f"testassets\\maxiteration31deep.txt"

        # Expected missing dependency hiding 31 dependencies deep
        expected_dependencies = [
            #            String                                        Asset                     #
            ("6BDE282B-49C9-57F7-B071-4B26579BCA9A", "{6BDE282B-49C9-57F7-B071-4B26579BCA9A}:0")
        ]

        self.do_missing_dependency_test(expected_product, expected_dependencies,
                                        "%MaxIteration31Deep.txt", max_iterations=31)

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C17226567")
    # fmt:off
    def test_WindowsAndMac_PotentialMatchesLongerThanUUIDString_OnlyReportsCorrectLengthUUIDs(self):
        # fmt:on
        """
        Tests that dependency references that are longer than expected are ignored

        Test Steps:
        1. Set the expected product
        2. Set the expected missing dependencies
        3. Execute test
        """

        # Relative path to text file with varying length UUID references
        expected_product = f"testassets\\onlymatchescorrectlengthuuids.txt"
        # Expected dependencies with valid lengths from file
        expected_dependencies = [
            # String                              Asset ID
            ('D92C4661C8985E19BD3597CB2318CFA6', '{D92C4661-C898-5E19-BD35-97CB2318CFA6}:0'),
            ('837412DFD05F576D81AAACF360463749', '{837412DF-D05F-576D-81AA-ACF360463749}:0'),
            ('785A05D2483E5B43A2B992ACDAE6E938', '{785A05D2-483E-5B43-A2B9-92ACDAE6E938}:0'),
            ('3F642A0FDC825696A70A1DA5709744DF', '{3F642A0F-DC82-5696-A70A-1DA5709744DF}:0'),
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
        """
        Tests the Missing Dependency Scanner can scan gradimage files

        Test Steps:
        1. Create temporary testing environment
        2. Run the move dependency scanner against the gradimage
        2. Validate that the expected product files and and expected depdencies match
        """

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
