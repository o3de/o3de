"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

General Asset Bundler Batch Tests
"""

# Import builtin libraries
import pytest
import logging
import os
import subprocess
import re
from typing import List, Optional, Dict
import time

# Import LyTestTools
import ly_test_tools.builtin.helpers as helpers
import ly_test_tools.environment.file_system as fs
import ly_test_tools.environment.waiter as waiter

from ..ap_fixtures.ap_setup_fixture import ap_setup_fixture as ap_setup_fixture
from ..ap_fixtures.asset_processor_fixture import asset_processor
from ..ap_fixtures.timeout_option_fixture import timeout_option_fixture as timeout

# fmt:off
from ..ap_fixtures.bundler_batch_setup_fixture \
    import bundler_batch_setup_fixture as bundler_batch_helper
# fmt:on
from ..ap_fixtures.ap_config_backup_fixture import ap_config_backup_fixture as config_backup

# Import LyShared
import ly_test_tools.o3de.pipeline_utils as utils
from ly_test_tools.o3de.asset_processor import ASSET_PROCESSOR_PLATFORM_MAP

win_and_mac_platforms = [ASSET_PROCESSOR_PLATFORM_MAP['windows'],
                         ASSET_PROCESSOR_PLATFORM_MAP['mac']]

# Just some platforms for filename computation (doesn't matter which)
platforms = {}
for key, value in ASSET_PROCESSOR_PLATFORM_MAP.items():
    platforms[value] = key

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


@pytest.mark.usefixtures("local_resources")
@pytest.mark.parametrize("project", targetProjects)
@pytest.mark.assetpipeline
@pytest.mark.SUITE_periodic
class TestsAssetBundlerBatch(object):
    """
    Asset Bundler Batch Tests for all platforms
    """

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C16877174")
    @pytest.mark.test_case_id("C16877175")
    @pytest.mark.test_case_id("C16877178")
    @pytest.mark.test_case_id("C16877177")
    def test_RunHelpCmd_ZeroExitCode(self, workspace, bundler_batch_helper):
        """
        Simple calls to all AssetBundlerBatch --help to make sure a non-zero exit codes are returned.

        Test will call each Asset Bundler Batch sub-command with help and will error on a non-0 exit code
        """
        bundler_batch_helper.call_bundlerbatch(help="")
        bundler_batch_helper.call_seeds(help="")
        bundler_batch_helper.call_assetLists(help="")
        bundler_batch_helper.call_comparisonRules(help="")
        bundler_batch_helper.call_compare(help="")
        bundler_batch_helper.call_bundleSettings(help="")
        bundler_batch_helper.call_bundles(help="")
        bundler_batch_helper.call_bundleSeed(help="")


    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C16877175")
    def test_GenerateDebugInfo_DoesNotEffectOutputFile(self, workspace, bundler_batch_helper):
        """
        Validates destructive overwriting for asset lists and
        that generating debug information does not affect asset list creation

        1. Create an asset list from seed_list
        2. Validate asset list was created
        3. Read and store contents of asset list into memory
        4. Attempt to create a new asset list in without using --allowOverwrites
        5. Verify that Asset Bundler returns false
        6. Verify that file contents of the originally created asset list did not change from what was stored in memory
        7. Attempt to create a new asset list without debug while allowing overwrites
        8. Verify that file contents of the originally created asset list changed from what was stored in memory
        """
        helper = bundler_batch_helper
        seed_list = os.path.join(workspace.paths.engine_root(), "Assets", "Engine", "SeedAssetList.seed")  # Engine seed list
        asset = r"levels\testdependencieslevel\testdependencieslevel.spawnable"

        # Create Asset list
        helper.call_assetLists(
            seedListFile=seed_list,
            assetListFile=helper['asset_info_file_request'],
        )

        # Validate file was created
        assert os.path.isfile(helper["asset_info_file_result"])

        # Read asset list contents to compare before and after destructive overwrite
        with open(helper["asset_info_file_result"], "r") as asset_list_file:
            file_contents = asset_list_file.read()

        # Make sure destructive overwrite will fail without --allowOverwrites
        # Try overwriting the existing file without --allowOverwrites (should fail)
        result, _ = helper.call_assetLists(seedListFile=seed_list, addSeed=asset,
                                           assetListFile=helper["asset_info_file_request"])
        assert result is False, "Destructive overwrite without override DID NOT fail"

        # Make sure file contents DID NOT change
        # fmt:off
        with open(helper["asset_info_file_result"], "r") as asset_list_file:
            assert file_contents == asset_list_file.read(), \
                "File was changed even though the Destructive overwrite failed without override."
        # fmt:on

        # Create the asset list file without generating debug info (and overwriting existing file)
        helper.call_assetLists(
            addSeed=asset,
            assetListFile=helper["asset_info_file_request"],
            allowOverwrites=""
        )

        # Make sure file contents DID change
        # fmt:off
        with open(helper["asset_info_file_result"], "r") as asset_list_file:
            assert file_contents != asset_list_file.read(), \
                "File was NOT changed even though the Destructive overwrite was allowed."
        # fmt:on

        # Get list of all files (relative paths) in generated asset list (no debug file created)
        assets_generated_without_debug_info = []
        for rel_path in helper.get_asset_relative_paths(helper["asset_info_file_result"]):
            assets_generated_without_debug_info.append(rel_path)

        # Create the asset list file while also generating debug info
        helper.call_assetLists(
            addSeed=asset,
            allowOverwrites="",
            generateDebugFile="",
            assetListFile=helper["asset_info_file_request"]
        )
        assert os.path.isfile(helper["asset_info_file_result"])

        # Get list of all files (relative paths) in generated asset list (debug file created)
        assets_generated_with_debug_info = []
        for rel_path in helper.get_asset_relative_paths(helper["asset_info_file_result"]):
            assets_generated_with_debug_info.append(rel_path)

        # Compare assets from asset lists
        assert sorted(assets_generated_without_debug_info) == sorted(assets_generated_with_debug_info)

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C16877175")
    @pytest.mark.test_case_id("C16877177")
    def test_BundlesAndBundleSettings_EquivalentOutput(self, workspace, bundler_batch_helper):
        """
        Validates bundle creation both through the 'bundles' and 'bundlesettings'
        subcommands.

        Test Steps:
        1. Create an asset list
        2. Create a bundle with the asset list and without a bundle settings file
        3. Create a bundle with the asset list and a bundle settings file
        4. Validate calling bundle doesn't perform destructive overwrite without --allowOverwrites
        5. Calling bundle again with --alowOverwrites performs destructive overwrite
        6. Validate contents of original bundle and overwritten bundle
        """
        helper = bundler_batch_helper
        seed_list = os.path.join(workspace.paths.engine_root(), "Assets", "Engine", "SeedAssetList.seed")  # Engine seed list
        asset = r"levels\testdependencieslevel\testdependencieslevel.spawnable"

        # Useful bundle locations / names (2 for comparing contents)
        # fmt:off
        platform_bundle_file_1 = os.path.join(
            helper["test_dir"],
            helper.platform_file_name(helper["bundle_file_name"], workspace.asset_processor_platform))

        second_bundle_file_request = os.path.join(helper["test_dir"], "bundle_2.pak")
        platform_bundle_file_2 = os.path.join(
            helper["test_dir"], helper.platform_file_name("bundle_2.pak", workspace.asset_processor_platform))
        # fmt:on

        # Extraction directories
        extract_dir_1 = os.path.join(helper["test_dir"], "ExtractDir1")
        extract_dir_2 = os.path.join(helper["test_dir"], "ExtractDir2")

        # Create asset list to test bundles on
        helper.call_assetLists(
            addSeed=asset,
            seedListFile=seed_list,
            assetListFile=helper["asset_info_file_request"],
        )

        # Make a bundle using 'bundles' command
        helper.call_bundles(
            assetListFile=helper["asset_info_file_result"],
            outputBundlePath=helper["bundle_file"],
            maxSize=helper["max_bundle_size_in_mib"],
        )

        # Make a bundle setting using the 'bundleSettings' command
        helper.call_bundleSettings(
            bundleSettingsFile=helper["bundle_settings_file_request"],
            assetListFile=helper["asset_info_file_result"],
            outputBundlePath=second_bundle_file_request,
            maxSize=helper["max_bundle_size_in_mib"],
        )

        # Recreate bundle via bundle settings this time
        helper.call_bundles(bundleSettingsFile=helper["bundle_settings_file_result"])


        # Make sure destructive overwrite fails without --allowOverwrites
        result, _ = helper.call_bundles(bundleSettingsFile=helper["bundle_settings_file_result"])

        assert result is False, "bundles call DID NOT fail when overwriting without --allowOverrides"

        # Run again to check overriding (this time it should work)
        helper.call_bundles(
            bundleSettingsFile=helper["bundle_settings_file_result"],
            allowOverwrites="",
        )

        # Validate results
        helper.extract_and_check(extract_dir_1, platform_bundle_file_1)
        helper.extract_and_check(extract_dir_2, platform_bundle_file_2)

        # Get files from extracted directories
        extracted_files_1 = utils.get_relative_file_paths(extract_dir_1)
        extracted_files_2 = utils.get_relative_file_paths(extract_dir_2)

        assert sorted(extracted_files_1) == sorted(extracted_files_2), "The two extracted bundles do not match"

        # Use CRC checksums to make sure archives' files match
        crc_1 = helper.get_crc_of_files_in_archive(platform_bundle_file_1)
        crc_2 = helper.get_crc_of_files_in_archive(platform_bundle_file_2)
        del crc_1["manifest.xml"]  # Remove manifest files from comparisons
        del crc_2["manifest.xml"]  # it is expected that they may differ
        assert crc_1 == crc_2, "Extracted files from the two different bundles did not match"

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C16877175")
    @pytest.mark.test_case_id("C16877177")
    def test_CreateMultiPlatformBundles_ValidContents(self, workspace, bundler_batch_helper):
        """
        Creates bundles using the same asset list and compares that they are created equally. Also
        validates that platform bundles exclude/include an expected file. (excluded for WIN, included for MAC)

        Test Steps:
        1. Create an asset list
        2. Create bundles for both PC & Mac
        3. Validate that bundles were created
        4. Verify that expected missing file is not in windows bundle
        5. Verify that expected file is in the mac bundle
        6. Create duplicate bundles with allowOverwrites
        7. Verify that files were generated
        8. Verify original bundle checksums are equal to new bundle checksums
        """
        helper = bundler_batch_helper
        # fmt:off
        assert "pc" in helper["platforms"] and "mac" in helper["platforms"], \
            "This test requires both PC and MAC platforms to be enabled. " \
            "Please rerun with commandline option: '--bundle_platforms=pc,mac'"
        # fmt:on

        # Engine seed list
        seed_list = os.path.join(workspace.paths.engine_root(), "Assets", "Engine", "SeedAssetList.seed")

        # Useful bundle / asset list locations
        bundle_dir = os.path.dirname(helper["bundle_file"])
        bundle_files = {}
        duplicate_bundle_files = {}
        for platform in helper["platforms"]:
            bundle_files[platform] = os.path.join(
                bundle_dir,
                helper.platform_file_name("bundle.pak", platforms[platform])
            )
            duplicate_bundle_files[platform] = os.path.join(
                bundle_dir,
                helper.platform_file_name("duplicateBundle.pak", platforms[platform])
            )

        duplicate_asset_info_file_request = os.path.join(helper["test_dir"], "duplicateAssetFileInfo.assetlist")
        duplicate_bundle_file = os.path.join(helper["test_dir"], "duplicateBundle.pak")

        # Create an asset list to work with
        helper.call_assetLists(
            seedListFile=seed_list,
            assetListFile=helper["asset_info_file_request"],
            platform=helper["platforms_as_string"],
        )

        # Create bundles for both pc and mac
        helper.call_bundles(
            assetListFile=helper["asset_info_file_request"],
            outputBundlePath=helper["bundle_file"],
            platform=helper["platforms_as_string"],
        )

        # Ensure that the bundles were created
        for bundle_file in bundle_files.values():
            assert os.path.isfile(bundle_file)

        # This asset is created both on mac and windows platform
        file_to_check = b"engineassets/textures/Cubemap/default_level_cubemap.tif"  # [use byte str because file is in binary]

        # Extract the delta catalog file from pc archive. {file_to_check} SHOULD NOT be present for PC
        file_contents = helper.extract_file_content(bundle_files["pc"], "DeltaCatalog.xml")
        # fmt:off
        assert file_contents.find(file_to_check), \
            f"{file_to_check} was found in DeltaCatalog.xml in pc bundle file {bundle_files['pc']}"
        # fmt:on

        # Extract the delta catalog file from mac archive. {file_to_check} SHOULD be present for MAC
        file_contents = helper.extract_file_content(bundle_files["mac"], "DeltaCatalog.xml")
        # fmt:off
        assert file_contents.find(file_to_check), \
            f"{file_to_check} was not found in DeltaCatalog.xml in darwin bundle file {bundle_files['mac']}"
        # fmt:on

        # Gather checksums for first set of bundles
        check_sums_before = {}
        for platform in helper["platforms"]:
            check_sums_before[platform] = helper.get_crc_of_files_in_archive(bundle_files[platform])

        # Create duplicate asset list
        helper.call_assetLists(
            seedListFile=seed_list,
            assetListFile=duplicate_asset_info_file_request,
            platform=helper["platforms_as_string"],
            allowOverwrites="",
        )

        # Create duplicate bundles
        helper.call_bundles(
            assetListFile=f"{helper['asset_info_file_request']},{duplicate_asset_info_file_request}",
            outputBundlePath=f"{helper['bundle_file']},{duplicate_bundle_file}",
            platform=helper["platforms_as_string"],
            allowOverwrites="",
        )

        # Make sure all files were created as expected
        for platform in helper["platforms"]:
            assert os.path.isfile(bundle_files[platform]), f"File: {bundle_files[platform]} was not created"
            # fmt:off
            assert os.path.isfile(duplicate_bundle_files[platform]), \
                f"File: {duplicate_bundle_files[platform]} was not created"
            # fmt:on

        # Get original bundles' contents again
        check_sums_after = {}
        check_sums_duplicates = {}
        for platform in helper["platforms"]:
            check_sums_after[platform] = helper.get_crc_of_files_in_archive(bundle_files[platform])
            check_sums_duplicates[platform] = helper.get_crc_of_files_in_archive(duplicate_bundle_files[platform])

        # Make sure original bundles' contents did not change during operation
        for platform in helper["platforms"]:
            # fmt:off
            assert check_sums_before[platform] == check_sums_after[platform], \
                f"Before and after check sum for {platform} did not match"
            assert check_sums_before[platform] == check_sums_duplicates[platform], \
                f"Before and duplicated check sum for {platform} did not match"
            # fmt:on

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C16877174")
    def test_AddAndRemoveSeedPlatform_Success(self, workspace, bundler_batch_helper):
        """
        Validates that the 'seeds' subcommand can add and remove seeds and seed platforms properly.
        Also checks that destructive overwrites require the --allowOverwrites flag

        Test Steps:

        1. Create a PC Seed List from a test asset
        2. Validate that seed list was generated with proper platform flag
        3. Add Mac & PC as platforms to the seed list
        4. Verify that seed has both Mac & PC platform flags
        5. Remove Mac as a platform from the seed list
        6. Verify that seed only has PC as a platform flag
        7. Attempt to add a platform without using the --platform argument
        8. Verify that asset bundler returns False and file contents did not change
        9. Add Mac platform via --addPlatformToSeeds
        10. Validate that seed has both Mac & PC platform flags
        11. Attempt to remove platform without specifying a platform
        12. Validate that seed has both Mac & PC platform flags
        13. Validate that seed list contents did not change
        14. Remove seed
        15. Validate that seed was removed from the seed list
        """
        helper = bundler_batch_helper

        # Make sure asset list file does not exist entering the test
        if os.path.exists(helper["asset_info_file_request"]):
            fs.delete([helper["asset_info_file_request"]], True, True)

        test_asset = r"fonts/open_sans/license.txt"
        re_pattern = re.compile(r"""field="platformFlags" value="(\d+)" """)
        all_lines = ""

        def check_seed_platform(seed_file: str, asset: str, expected_platform_flag: int) -> str:
            """Helper function to check a seed's platform flag. Returns the contents of the seed_file"""
            with open(seed_file, "r") as seed_list_file:
                lines = seed_list_file.read()
            assert asset in lines, f"{asset} was not added to asset list file {seed_file}"
            re_match = re_pattern.search(lines)
            assert re_match, f"PlatformFlags were not found in seed file {seed_file}"
            platform_flag = int(re_match.group(1))
            # fmt:off
            assert platform_flag == expected_platform_flag, \
                f"Expected platform flag to be {expected_platform_flag}. Actual {platform_flag}"
            # fmt:on
            return lines

        # End check_seed_platform()

        # Create seed list file
        helper.call_seeds(
            seedListFile=helper["seed_list_file"],
            addSeed=test_asset,
            platform="pc",
        )

        # Validate file exists and has proper platform flag
        assert os.path.exists(helper["seed_list_file"]), f"seed list file was not created at {helper['seed_list_file']}"
        check_seed_platform(helper["seed_list_file"], test_asset, helper["platform_values"]["pc"])

        # Add Mac and pc as platform for seed
        helper.call_seeds(
            seedListFile=helper["seed_list_file"],
            addSeed=test_asset,
            platform="pc,mac",
        )

        # Validate both mac and pc are activated for seed
        # fmt:off
        check_seed_platform(helper["seed_list_file"], test_asset,
                            helper["platform_values"]["pc"] + helper["platform_values"]["mac"])
        # fmt:on

        # Remove MAC platform
        helper.call_seeds(
            seedListFile=helper["seed_list_file"],
            removePlatformFromSeeds="",
            platform="mac",
        )
        # Validate only pc platform for seed. Save file contents to variable
        all_lines = check_seed_platform(helper["seed_list_file"], test_asset, helper["platform_values"]["pc"])

        result, _ = helper.call_seeds(seedListFile=helper["seed_list_file"], addPlatformToSeeds="", )

        assert result is False, "Calling --addPlatformToSeeds did not fail when not specifying the --platform argument"

        # Make sure the file contents did not change
        # fmt:off
        with open(helper["seed_list_file"], "r") as asset_list_file:
            assert all_lines == asset_list_file.read(), \
                "Calling --addPlatformToSeeds without --platform failed but changed the seed file"
        # fmt:on

        # Add MAC platform via --addPlatformToSeeds
        helper.call_seeds(
            seedListFile=helper["seed_list_file"],
            addPlatformToSeeds="",
            platform="mac",
        )
        # Validate Mac platform was added back on. Save file contents
        # fmt:off
        all_lines = check_seed_platform(helper["seed_list_file"], test_asset,
                                        helper["platform_values"]["pc"] + helper["platform_values"]["mac"])
        # fmt:on

        # Try to remove platform without specifying a platform to remove (should fail)
        result, _ = helper.call_seeds(seedListFile=helper["seed_list_file"], removePlatformFromSeeds="", )

        assert result is False, "Calling --removePlatformFromSeeds did not fail when not specifying the --platform argument"

        # Make sure file contents did not change
        # fmt:off
        with open(helper["seed_list_file"], "r") as asset_list_file:
            assert all_lines == asset_list_file.read(), \
                "Calling --removePlatformFromSeeds without --platform failed but changed the seed file"
        # fmt:on

        # Remove the seed
        helper.call_seeds(
            seedListFile=helper["seed_list_file"],
            removeSeed=test_asset,
            platform="pc,mac",
        )

        # Validate seed was removed from file
        # fmt:off
        with open(helper["seed_list_file"], "r") as seed_list_file:
            assert test_asset not in seed_list_file.read(), \
                f"Seed was not removed from asset list file {helper['seed_list_file']}"
        # fmt:on

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.SUITE_sandbox
    @pytest.mark.test_case_id("C16877174")
    @pytest.mark.test_case_id("C16877175")
    @pytest.mark.test_case_id("C16877178")
    # fmt:off
    def test_ComparisonOperations_Success(self, workspace, bundler_batch_helper, ap_setup_fixture,
                                                        asset_processor, timeout):
        # fmt:on
        """
        Tests asset list comparison, both by file and by comparison type. Uses a set
        of controlled test assets to compare resulting output asset lists

        1. Create comparison rules files
        2. Create seed files for different sets of test assets
        3. Create assetlist files for seed files
        4. Validate assetlists were created properly
        5. Compare using comparison rules files and just command line arguments
        """
        helper = bundler_batch_helper
        env = ap_setup_fixture

        # fmt:off
        assert "pc" in helper["platforms"] and "mac" in helper["platforms"], \
            "This test requires both PC and MAC platforms to be enabled. " \
            "Please rerun with commandline option: '--bundle_platforms=pc,mac'"
        # fmt:on

        # Test assets arranged in common lists: six (0-5) .txt files and .dat files
        even_txt = ["txtfile_0.txt", "txtfile_2.txt", "txtfile_4.txt"]
        odd_txt = ["txtfile_1.txt", "txtfile_3.txt", "txtfile_5.txt"]
        even_dat = ["datfile_0.dat", "datfile_2.dat", "datfile_4.dat"]
        odd_dat = ["datfile_1.dat", "datfile_3.dat", "datfile_5.dat"]
        even_assets = even_txt + even_dat
        odd_assets = odd_txt + odd_dat
        all_txt = even_txt + odd_txt
        all_dat = even_dat + odd_dat
        all_assets = even_assets + odd_assets

        # Maps a test asset to platform(s)
        file_platforms = {
            "txtfile_0.txt": "pc",
            "txtfile_1.txt": "pc",
            "txtfile_2.txt": "pc,mac",
            "txtfile_3.txt": "pc,mac",
            "txtfile_4.txt": "mac",
            "txtfile_5.txt": "mac",
            "datfile_0.dat": "pc",
            "datfile_1.dat": "pc",
            "datfile_2.dat": "pc,mac",
            "datfile_3.dat": "pc,mac",
            "datfile_4.dat": "mac",
            "datfile_5.dat": "mac",
        }

        # Comparison rules files and their associated 'comparisonType' flags
        comparison_files = [
            ("delta.rules", "0"),
            ("union.rules", "1"),
            ("intersection.rules", "2"),
            ("complement.rules", "3"),
            ("pattern.rules", "4"),
            ("combined.rules", "2,0"),
        ]

        # Get our test assets ready and processed
        utils.prepare_test_assets(env["tests_dir"], "C16877178", env["project_test_assets_dir"])
        asset_processor.batch_process(timeout=timeout, fastscan=False, platforms="pc,mac")

        # *** Some helper functions *** #

        def create_seed_file(asset_names: List[str], seed_file_name: str) -> None:
            """Adds the [asset_names] to the seed file [seed_file_name] with their specific platform"""
            for asset_file_name in asset_names:
                helper.call_seeds(
                    seedListFile=os.path.join(helper["test_dir"], seed_file_name),
                    addSeed=os.path.join(env["test_asset_dir_name"], asset_file_name),
                    platform=file_platforms[asset_file_name],
                )

        def create_asset_list_file(seed_file_name: str, asset_list_file_name: str) -> None:
            """Simple wrapper for calling the Bundler Batch for a [seed_file_name] and [asset_list_file_name]"""
            helper.call_assetLists(
                assetListFile=os.path.join(helper["test_dir"], asset_list_file_name),
                seedListFile=os.path.join(helper["test_dir"], seed_file_name),
                platform="pc,mac",
            )

        def get_platform_assets(asset_name_list: List[str]) -> Dict[str, List[str]]:
            """Separates each asset in [asset_name_list] into their platforms"""
            win_assets = []
            mac_assets = []
            for asset_name in asset_name_list:
                if "pc" in file_platforms[asset_name]:
                    win_assets.append(asset_name)
                if "mac" in file_platforms[asset_name]:
                    mac_assets.append(asset_name)
            return {"win": win_assets, "mac": mac_assets}

        def validate_asset_list(asset_list_file: str, asset_list: List[str]) -> None:
            """Validates that the [asset_list_file] contains exactly the assets in [asset_list]"""
            assets_to_find = list(asset_list)  # Make copy of list. We will be removing elements as we go
            for rel_path in helper.get_asset_relative_paths(os.path.join(helper["test_dir"], asset_list_file)):
                asset = os.path.split(rel_path)[1]  # Get just the asset's file name
                try:
                    assets_to_find.remove(asset)  # Attempt to remove
                except ValueError:
                    # Item not found in list? Unexpected asset

                    assert False, (
                        f"Unexpected asset found. Asset List: {asset_list_file}; "
                        f"Unexpected Asset {asset}; Expected Assets: {asset_list}"
                    )

            # If assets_to_find is empty, we found all expected assets
            assert (
                    len(assets_to_find) == 0
            ), f"Expected asset(s) {assets_to_find} not found in asset list: {asset_list_file}"

        def validate_request_file(request_file: str, asset_names: List[str]) -> None:
            """Validates both mac and pc platform results for an assetlist request file"""

            # Get platform result file names
            win_asset_list_file = helper.platform_file_name(request_file, platforms["pc"])
            mac_asset_list_file = helper.platform_file_name(request_file, platforms["mac"])

            # Get expected platforms for each asset in asset_names
            platform_files = get_platform_assets(asset_names)

            # Validate each platform
            validate_asset_list(win_asset_list_file, platform_files["win"])
            validate_asset_list(mac_asset_list_file, platform_files["mac"])

        def compare_and_check(
                rule: str,  # The rule to use for comparison
                first_asset_list: str or List[str],  # The parameter(s) for '--firstAssetList'
                second_asset_list: str or List[str],  # The parameter(s) for '--secondAssetList'
                expected_asset_list: str or List[str],  # A list of expected asset to be in the output assetlist
                output_file: str or List[str] = "output.assetlist",  # The parameter for '--output'
                use_file: Optional[bool] = False,  # Bool for whether to compare using the .rules file
                pattern_type: Optional[str] = None,  # Parameter for '--filePatternType' (pattern comparison only)
                pattern: Optional[str] = None,  # Parameter for '--filePattern' (pattern comparison only)
        ) -> None:
            """
            Based on parameters, creates an asset bundler batch command for executing a compare,
            then validates the resulting file. Runs command for both platforms (Win and Mac)
            """

            def asset_lists_to_string(asset_lists: str or List[str]) -> object:
                """Converts a list of asset list files into a string parameter to use for Bundler CLI"""
                if asset_lists is None:
                    return None
                if type(asset_lists) == str:
                    asset_lists = [asset_lists]
                out = ""
                for asset_list in asset_lists:
                    if asset_list.startswith("$"):
                        # If it's a command line variable, don't worry about file platform
                        out += asset_list + ","
                    else:
                        # Get absolute file path
                        out += os.path.join(helper["test_dir"], asset_list) + ","
                return out[:-1]  # Trim off extra comma

            # End asset_lists_to_string()

            # Initialize common command arguments
            # We do not specify a platform in the file names, the AssetBundlerBatch will handle that automatically
            first_input_arg = asset_lists_to_string(first_asset_list)  # --firstAssetList
            second_input_arg = asset_lists_to_string(second_asset_list)  # --secondAssetList
            output_arg = asset_lists_to_string(output_file)  # --output

            def generate_compare_command(platform_arg: str, project_name : str) -> object:
                """Creates a string containing a full Compare command. This string can be executed as-is."""
                cmd = [helper["bundler_batch"], "compare", f"--firstassetFile={first_input_arg}", f"--output={output_arg}"]
                if platform_arg is not None:
                    cmd.append(f"--platform={platform_arg}")
                if second_input_arg is not None:
                    cmd.append(f"--secondAssetFile={second_input_arg}")
                if use_file:
                    file_name = os.path.join(helper["test_dir"], rule + ".rules")
                    cmd.append("--comparisonRulesFile=" + file_name)
                else:
                    comp_type = [i for r, i in comparison_files if r.startswith(rule)][0]  # Get comparisonType flag
                    cmd.append(f"--comparisonType={comp_type}")
                    if comp_type == "4":
                        # Extra arguments for pattern comparison
                        cmd.extend([f"--filePatternType={pattern_type}", f"--filePattern={pattern}"])
                if workspace.project:
                    cmd.append(f'--project-path={workspace.paths.project()}')
                return cmd
            # End generate_compare_command()

            def verify_asset_list_contents(expected: str, output_asset_list: str) -> None:
                # Compare relative paths from inside 'expected' and 'output_asset_list' (last output file from cmd)
                expected_paths = []
                actual_paths = []
                for rel_path in helper.get_asset_relative_paths(expected):
                    expected_paths.append(rel_path)
                for rel_path in helper.get_asset_relative_paths(output_asset_list):
                    actual_paths.append(rel_path)
                # fmt:off
                assert sorted(expected_paths) == sorted(actual_paths), \
                    "Asset list comparison did not yield expected results"
                # fmt:on
            # End verify_asset_list_contents()

            def run_compare_command_and_verify(platform_arg: str, expect_pc_output: bool, expect_mac_output: bool) -> None:

                # Expected asset list to equal result of comparison
                expected_pc_asset_list = None
                expected_mac_asset_list = None

                # Last output file. Use this for comparison to 'expected'
                output_pc_asset_list = None
                output_mac_asset_list = None

                # Add the platform to the file name to match what the Bundler will create
                last_output_arg = output_arg.split(",")[-1]
                if expect_pc_output:
                    platform = platforms["pc"]
                    expected_pc_asset_list = os.path.join(helper["test_dir"], helper.platform_file_name(expected_asset_list, platform))
                    output_pc_asset_list = helper.platform_file_name(last_output_arg, platform)

                if expect_mac_output:
                    platform = platforms["mac"]
                    expected_mac_asset_list = os.path.join(helper["test_dir"], helper.platform_file_name(expected_asset_list, platform))
                    output_mac_asset_list = helper.platform_file_name(last_output_arg, platform)

                # Build execution command
                cmd = generate_compare_command(platform_arg, workspace.paths.project())

                # Execute command
                subprocess.check_call(cmd)

                # Verify and clean up
                if expect_pc_output:
                    verify_asset_list_contents(expected_pc_asset_list, output_pc_asset_list)
                    fs.delete([output_pc_asset_list], True, True)

                if expect_mac_output:
                    verify_asset_list_contents(expected_mac_asset_list, output_mac_asset_list)
                    fs.delete([output_mac_asset_list], True, True)
            # End run_compare_command_and_verify()

            # Generate command, run and validate for each platform
            run_compare_command_and_verify("pc", True, False)
            run_compare_command_and_verify("mac", False, True)
            run_compare_command_and_verify("pc,mac", True, True)
            #run_compare_command_and_verify(None, True, True)

        # End compare_and_check()

        # *** Start test execution code *** #

        # Create comparison (.rules) files
        for args in comparison_files:
            rule_file = os.path.join(helper["test_dir"], args[0])
            logger.info(f"Creating rule file: {rule_file}")
            cmd = [
                helper["bundler_batch"],
                "comparisonRules",
                f"--comparisonRulesFile={rule_file}",
                f"--comparisonType={args[1]}",
                r"--addComparison",
                f"--project-path={workspace.paths.project()}",
            ]
            if args[1] == "4":
                # If pattern comparison, append a few extra arguments
                cmd.extend(["--filePatternType=0", "--filePattern=*.dat"])

            subprocess.check_call(cmd)
            assert os.path.exists(rule_file), f"Rule file {args[0]} was not created at location: {rule_file}"

        # Create seed files for different sets of test assets (something to compare against)
        create_seed_file(even_txt, "even_txt.seed")
        create_seed_file(even_dat, "even_dat.seed")
        create_seed_file(even_assets, "even_assets.seed")
        create_seed_file(odd_txt, "odd_txt.seed")
        create_seed_file(odd_dat, "odd_dat.seed")
        create_seed_file(odd_assets, "odd_assets.seed")
        create_seed_file(all_txt, "all_txt.seed")
        create_seed_file(all_dat, "all_dat.seed")
        create_seed_file(all_assets, "all_assets.seed")

        # Create assetlist files for seed files
        create_asset_list_file("even_txt.seed", "even_txt.assetlist")
        create_asset_list_file("even_dat.seed", "even_dat.assetlist")
        create_asset_list_file("even_assets.seed", "even_assets.assetlist")
        create_asset_list_file("odd_txt.seed", "odd_txt.assetlist")
        create_asset_list_file("odd_dat.seed", "odd_dat.assetlist")
        create_asset_list_file("odd_assets.seed", "odd_assets.assetlist")
        create_asset_list_file("all_txt.seed", "all_txt.assetlist")
        create_asset_list_file("all_dat.seed", "all_dat.assetlist")
        create_asset_list_file("all_assets.seed", "all_assets.assetlist")

        # Make sure the assetlists were created properly (including platform validation)
        validate_request_file("even_txt.assetlist", even_txt)
        validate_request_file("even_dat.assetlist", even_dat)
        validate_request_file("even_assets.assetlist", even_assets)
        validate_request_file("odd_txt.assetlist", odd_txt)
        validate_request_file("odd_dat.assetlist", odd_dat)
        validate_request_file("odd_assets.assetlist", odd_assets)
        validate_request_file("all_txt.assetlist", all_txt)
        validate_request_file("all_dat.assetlist", all_dat)
        validate_request_file("all_assets.assetlist", all_assets)

        # Compare using comparison rules files and just command line arguments
        for use_file in [True, False]:
            compare_and_check(
                "delta", "even_assets.assetlist", "all_assets.assetlist", "odd_assets.assetlist", use_file=use_file
            )
            compare_and_check(
                "union", "even_assets.assetlist", "odd_assets.assetlist", "all_assets.assetlist", use_file=use_file
            )
            compare_and_check(
                "intersection",
                "even_assets.assetlist",
                "all_assets.assetlist",
                "even_assets.assetlist",
                use_file=use_file,
            )
            compare_and_check(
                "complement", "all_txt.assetlist", "all_assets.assetlist", "all_dat.assetlist", use_file=use_file
            )
            compare_and_check(
                "pattern",
                "all_assets.assetlist",
                None,
                "all_dat.assetlist",
                use_file=use_file,
                pattern_type="0",
                pattern="*.dat",
            )
            # Special parameters for 'combined' comparisons
            compare_and_check(
                rule="combined",
                first_asset_list=["all_dat.assetlist", "$first"],
                second_asset_list=["even_assets.assetlist", "even_assets.assetlist"],
                expected_asset_list="even_txt.assetlist",
                output_file=["$first", "output.assetlist"],
                use_file=use_file,
            )

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C16877174")
    @pytest.mark.test_case_id("C16877175")
    def test_AssetListCreation_OutputMatchesResult(self, workspace, bundler_batch_helper):
        """
        Tests that assetlists are created equivalent to the output while being created, and
        makes sure overwriting an existing file without the --allowOverwrites fails

        Test Steps:
        1. Check that Asset List creation requires PC platform flag
        2. Create a PC Asset List using asset info file and default seed lists using --print
        3. Validate all assets output are present in the asset list
        4. Create a seed file
        5. Attempt to overwrite Asset List without using --allowOverwrites
        6. Validate that command returned an error and file contents did not change
        7. Specifying platform but not "add" or "remove" should fail
        8. Verify file Has changed
        """
        helper = bundler_batch_helper

        # fmt:off
        assert "pc" in helper["platforms"], \
            "This test requires the PC platform to be enabled. " \
            "Please rerun with commandline option: '--bundle_platforms=pc'"
        # fmt:on

        # Assetlist result file (pc platform)
        al_file_path = os.path.join(
            helper["test_dir"],
            helper.platform_file_name(helper["asset_info_file_name"], platforms["pc"])
        )
        seed_files_pattern = re.compile("Loading Seed List file ( ([^)]*) )")

        # Create an asset file
        output = subprocess.check_output(
            [
                helper["bundler_batch"],
                "assetLists",
                f"--assetListFile={helper['asset_info_file_request']}",
                "--addDefaultSeedListFiles",
                "--platform=pc",
                "--print",
                f"--project-path={workspace.paths.project()}"
            ],
            universal_newlines=True,
        )

        seed_files = seed_files_pattern.findall(output)

        # Validate all assets output are present in the resulting asset file
        for seed_file in seed_files:
            for rel_path in helper.get_seed_relative_paths_for_platform(seed_file, helper.get_platform_flag("pc")):
                assert rel_path in output, f"{rel_path} was not found in output from Asset Bundle Batch"

        # Create a seed file
        helper.call_seeds(
            seedListFile=helper["seed_list_file"],
            addSeed=r"levels\testdependencieslevel\testdependencieslevel.spawnable",
            platform="pc",
        )

        # Get file contents before trying a failed overwrite attempt
        with open(al_file_path, "r") as asset_file:
            file_contents = asset_file.read()

        result, _ = helper.call_assetLists(
            assetListFile=helper["asset_info_file_request"],
            seedListFile=helper["seed_list_file"],
            platform="pc",
            print="",
        )
        assert result is False, "Overwriting without override did not throw an error"

        # Validate file did not change when overwrite failed
        with open(al_file_path, "r") as asset_file:
            assert file_contents == asset_file.read(), "The failed overwrite changed the file {al_file_path}"

        # Specifying platform but not "add" or "remove" should fail
        result, _ = helper.call_assetLists(
            assetListFile=helper["asset_info_file_request"],
            allowOverwrites="",
            seedListFile=helper["seed_list_file"],
            platform="pc",
        )
        assert result, "Overwriting with override threw an error"

        # Make sure the file is now changed
        with open(al_file_path, "r") as asset_file:
            assert file_contents != asset_file.read(), f"The overwrite did not change the file {al_file_path}"

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.test_case_id("C16877175")
    @pytest.mark.test_case_id("C16877177")
    # fmt:off
    def test_AP_BundleProcessing_BundleProcessedAtRuntime(self, workspace, bundler_batch_helper,
                                                                        asset_processor, request):
        # fmt:on
        """
        Test to make sure the AP GUI will process a newly created bundle file

        Test Steps:
        1. Make asset list file (used for bundle creation)
        2. Start Asset Processor GUI
        3. Make bundle in <project_folder>/Bundles
        4. Validate file was created in Bundles folder
        5. Make sure bundle now exists in cache
        """
        # Set up helpers and variables
        helper = bundler_batch_helper

        # Make sure file gets deleted on teardown
        request.addfinalizer(lambda: fs.delete([bundle_result_path], True, False))

        bundles_folder = os.path.join(workspace.paths.project(), "Bundles")
        level_pak = r"levels\testdependencieslevel\testdependencieslevel.spawnable"
        bundle_request_path = os.path.join(bundles_folder, "bundle.pak")
        bundle_result_path = os.path.join(bundles_folder,
                                          helper.platform_file_name("bundle.pak", workspace.asset_processor_platform))

        bundle_cache_path = os.path.join(workspace.paths.platform_cache(),
                                         "Bundles",
                                         helper.platform_file_name("bundle.pak", workspace.asset_processor_platform))

        # Create target 'Bundles' folder if DNE
        if not os.path.exists(bundles_folder):
            os.mkdir(bundles_folder)
        # Delete target bundle file if it already exists
        if os.path.exists(bundle_result_path):
            fs.delete([bundle_result_path], True, False)

        # Make asset list file (used for bundle creation)
        helper.call_assetLists(
            addSeed=level_pak,
            assetListFile=helper["asset_info_file_request"],
        )

        # Run Asset Processor GUI
        result, _ = asset_processor.gui_process()
        assert result, "AP GUI failed"

        time.sleep(5)

        # Make bundle in <project_folder>/Bundles
        helper.call_bundles(
            assetListFile=helper["asset_info_file_result"],
            outputBundlePath=bundle_request_path,
            maxSize="2048",
        )

        # Ensure file was created
        assert os.path.exists(bundle_result_path), f"Bundle was not created at location: {bundle_result_path}"

        timeout = 10
        waiter.wait_for(lambda: os.path.exists(bundle_cache_path), timeout=timeout)
        # Make sure bundle now exists in cache
        assert os.path.exists(bundle_cache_path), f"{bundle_cache_path} not found"

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    # fmt:off
    def test_FilesMarkedSkip_FilesAreSkipped(self, workspace, bundler_batch_helper):
        """
        Test Steps:
        1. Create an asset list with a file marked as skip
        2. Verify file was created
        3. Verify that only the expected assets are present in the created asset list
        """
        expected_assets = sorted([
            "ui/canvases/lyshineexamples/animation/multiplesequences.uicanvas",
            "ui/textures/prefab/button_disabled.tif.streamingimage",
            "ui/textures/prefab/tooltip_sliced.tif.streamingimage",
            "ui/textures/prefab/button_normal.tif.streamingimage"
        ])
        # Printing these lists out can save a step in debugging if this test fails on Jenkins.
        logger.info(f"expected_assets: {expected_assets}")

        skip_assets = sorted([
            "ui/scripts/lyshineexamples/animation/multiplesequences.luac",
            "ui/scripts/lyshineexamples/unloadthiscanvasbutton.luac",
            "fonts/vera.fontfamily",
            "fonts/vera-italic.font",
            "fonts/vera.font",
            "fonts/vera-bold.font",
            "fonts/vera-bold-italic.font",
            "fonts/vera-italic.ttf",
            "fonts/vera.ttf",
            "fonts/vera-bold.ttf",
            "fonts/vera-bold-italic.ttf"
        ])
        logger.info(f"skip_assets: {skip_assets}")

        expected_and_skip_assets = sorted(expected_assets + skip_assets)
        # Printing both together to make it quick to compare the results in the logs for a test failure on Jenkins
        logger.info(f"expected_and_skip_assets: {expected_and_skip_assets}")

        # First, generate an asset info file without skipping, to get a list that can be used as a baseline to verify
        # the files were actually skipped, and not just missing.
        bundler_batch_helper.call_assetLists(
            assetListFile=bundler_batch_helper['asset_info_file_request'],
            addSeed="ui/canvases/lyshineexamples/animation/multiplesequences.uicanvas"
        )
        assert os.path.isfile(bundler_batch_helper["asset_info_file_result"])
        assets_in_no_skip_list = []
        for rel_path in bundler_batch_helper.get_asset_relative_paths(bundler_batch_helper["asset_info_file_result"]):
            assets_in_no_skip_list.append(rel_path)
        assets_in_no_skip_list = sorted(assets_in_no_skip_list)
        logger.info(f"assets_in_no_skip_list: {assets_in_no_skip_list}")
        assert assets_in_no_skip_list == expected_and_skip_assets

        # Now generate an asset info file using the skip command, and verify the skip files are not in the list.
        bundler_batch_helper.call_assetLists(
            assetListFile=bundler_batch_helper['asset_info_file_request'],
            addSeed="ui/canvases/lyshineexamples/animation/multiplesequences.uicanvas",
            allowOverwrites="",
            skip=','.join(skip_assets)
        )

        assert os.path.isfile(bundler_batch_helper["asset_info_file_result"])
        assets_in_list = []
        for rel_path in bundler_batch_helper.get_asset_relative_paths(bundler_batch_helper["asset_info_file_result"]):
            assets_in_list.append(rel_path)
        assets_in_list = sorted(assets_in_list)
        logger.info(f"assets_in_list: {assets_in_list}")
        assert assets_in_list == expected_assets


    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    # fmt:off
    def test_AssetListSkipOneOfTwoParents_SharedDependencyIsIncluded(self, workspace,
                                                                                   bundler_batch_helper):
        """
        Test Steps:
        1. Create Asset List with multiple parent assets that are skipped
        2. Verify that Asset List was created
        3. Verify that only the expected assets are present in the asset list
        """
        expected_assets = [
            "objects/cube_lod0.azlod",
            "objects/cube_lod0_index.azbuffer",
            "resourcepools/defaultvertexbufferpool.pool"
        ]
        # Test Asset Structure:
        #          Grandparent
        #      /        |        \
        #   ParentA  ParentB  ParentC
        #      \        |        /
        #          CommonChild

        # Even if we exclude all parents besides "ParentA", we should still have "CommonChild" since it is a product dependency of "ParentA"
        bundler_batch_helper.call_assetLists(
            assetListFile=bundler_batch_helper['asset_info_file_request'],
            addSeed="objects/cube_lod0.azlod",
            skip="objects/cube_lod0_position0.azbuffer, objects/cube_lod0_tangent0.azbuffer,"
                 "objects/cube_lod0_normal0.azbuffer, objects/cube_lod0_bitangent0.azbuffer,"
                 "objects/cube_lod0_uv0.azbuffer"
        )
        assert os.path.isfile(bundler_batch_helper["asset_info_file_result"])
        assets_in_list = []
        for rel_path in bundler_batch_helper.get_asset_relative_paths(bundler_batch_helper["asset_info_file_result"]):
            assets_in_list.append(rel_path)
        assert sorted(assets_in_list) == sorted(expected_assets)

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    # fmt:off
    def test_AssetLists_SkipRoot_ExcludesAll(self, workspace, bundler_batch_helper):
        """
        Negative scenario test that skips the same file being used as the parent seed.

        Test Steps:
        1. Create an asset list that skips the root asset
        2. Verify that asset list was not generated
        """

        result, _ = bundler_batch_helper.call_assetLists(
            assetListFile=bundler_batch_helper['asset_info_file_request'],
            addSeed="libs/particles/milestone2particles.xml",
            skip="libs/particles/milestone2particles.xml"
        )
        if not result:
            assert not os.path.isfile(bundler_batch_helper["asset_info_file_result"])
            return
        # If an error was not thrown, this test should fail
        assert False

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    # fmt:off
    def test_AssetLists_SkipUniversalWildcard_ExcludesAll(self, workspace, bundler_batch_helper):
        """
        Negative scenario test that uses the all wildcard when generating an asset list.

        Test Steps:
        1. Create an Asset List while using the universal all wildcard "*"
        2. Verify that asset list was not generated
        """

        result, _ = bundler_batch_helper.call_assetLists(
            assetListFile=bundler_batch_helper['asset_info_file_request'],
            addSeed="libs/particles/milestone2particles.xml",
            skip="*"
        )
        if not result:
            assert not os.path.isfile(bundler_batch_helper["asset_info_file_result"])
            return
        # If an error was not thrown, this test should fail
        assert False

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    # fmt:off
    def test_Bundles_InvalidAssetList_ReportsError(self, workspace, bundler_batch_helper):
        """
        Verifies that when the bundles command is used, with an invalid asset list, the command fails and prints a useful error message.
        """
        try:
            # Not invoking via bundler_batch_helper because this test specifically wants an error to occur, and bundler_batch_helper handles errors.
            output = subprocess.check_output([bundler_batch_helper.bundler_batch,
                "bundles",
                "--assetListFile=this_file_does_not_exist.assetlist",
                f"--outputBundlePath={bundler_batch_helper['bundle_file']}"]).decode()
        except subprocess.CalledProcessError as e:
            output = e.output.decode('utf-8')
            expected_error_regex = "Cannot load Asset List file \\( (.*)this_file_does_not_exist\\.assetlist \\): File does not exist."

            if re.search(expected_error_regex, output) is None:
                logger.error(f"AssetBundlerBatch called with args returned error {e} with output {output}, missing expected error")
                assert False
            return
        logger.error("AssetBundlerBatch was expected to fail when given an invalid asset list to bundle, but did not.")
        assert False
