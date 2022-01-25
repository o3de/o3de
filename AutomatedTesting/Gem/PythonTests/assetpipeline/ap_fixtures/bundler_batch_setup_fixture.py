"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Fixture for helping Asset Bundler Batch tests
"""

import os
import pytest
from xml.etree import ElementTree as ET
import tempfile as TF
import re
import subprocess
import logging
import zipfile
from typing import Any, Dict, List

from . import asset_processor_fixture as asset_processor
from . import timeout_option_fixture as timeout
from . import ap_config_backup_fixture as config_backup

import ly_test_tools.environment.file_system as fs
import ly_test_tools.o3de.pipeline_utils as utils
from ly_test_tools.o3de.asset_processor import ASSET_PROCESSOR_PLATFORM_MAP

logger = logging.getLogger(__name__)


@pytest.fixture
@pytest.mark.usefixtures("config_backup")
def bundler_batch_setup_fixture(request, workspace, asset_processor, timeout) -> Any:

    ALL_PLATFORMS = []
    for _, value in ASSET_PROCESSOR_PLATFORM_MAP.items():
        ALL_PLATFORMS.append(value)

    # Set up temporary directory to use for tests
    tempDir = os.path.join(TF.gettempdir(), "AssetBundlerTempDirectory")
    if os.path.exists(tempDir):
        fs.delete([tempDir], True, True)
    if not os.path.exists(tempDir):
        os.mkdir(tempDir)

    # Get platforms to build for bundle tests
    platforms = request.config.getoption("--bundle_platforms")
    if platforms is not None:
        # Remove whitespace and split at commas if platforms provided
        platforms = [platform.strip() for platform in platforms.split(",")]
    else:
        # No commandline argument provided, default to mac and pc
        platforms = ["pc", "mac"]

    class BundlerBatchFixture:
        """
        Houses useful variables and functions for running Asset Bundler Batch Tests
        """

        def __init__(self):
            self.bin_dir = workspace.paths.build_directory()
            self.bundler_batch = os.path.join(self.bin_dir, "AssetBundlerBatch")
            self.test_dir = tempDir
            self.asset_alias = workspace.paths.platform_cache()
            self.tools_dir = os.path.join(workspace.paths.engine_root(), "Tools")
            self.seed_list_file_name = "testSeedListFile.seed"
            self.seed_list_file = os.path.join(self.test_dir, self.seed_list_file_name)
            self.asset_info_file_name = "assetFileInfo.assetlist"
            self.asset_info_file_request = os.path.join(self.test_dir, self.asset_info_file_name)
            self.bundle_settings_file_request = os.path.join(self.test_dir, "bundleSettingsFile.bundlesettings")
            self.bundle_file_name = "bundle.pak"
            self.bundle_file = os.path.join(self.test_dir, self.bundle_file_name)
            self.asset_info_file_result = os.path.join(
                self.test_dir, self.platform_file_name(self.asset_info_file_name,
                                                       workspace.asset_processor_platform)
            )
            self.bundle_settings_file_result = os.path.join(
                self.test_dir, self.platform_file_name("bundleSettingsFile.bundlesettings",
                                                       workspace.asset_processor_platform)
            )
            # Useful sizes
            self.max_bundle_size_in_mib = 5
            self.number_of_bytes_in_mib = 1024 * 1024
            self.workspace = workspace
            self.platforms = platforms
            self.platforms_as_string = ",".join(platforms)

        # *** Bundler-batch-calling helpers ***

        def call_asset_bundler(self, arg_list):                 
            # Print out the call to asset bundler, so if an error occurs, the commands that were run can be repeated.
            # Split out the list to something that can be easily copy and pasted. Printing a list by default will put it in,
            # [comma, separated, braces], this join will instead print each arg in the list with only a space between them.
            logger.info(f"{' '.join(f'arg_list{x}' for x in arg_list)}")
            try:
                output = subprocess.check_output(arg_list).decode()
                return True, output
            except subprocess.CalledProcessError as e:
                output = e.output.decode('utf-8')
                logger.error(f"AssetBundlerBatch called with args {arg_list} returned error {e} with output {output}")
                return False, output
            except FileNotFoundError as e:
                logger.error(f"File Not Found - Failed to call AssetBundlerBatch with args {arg_list} with error {e}")
                raise e

        def call_bundlerbatch(self, **kwargs: Dict[str, str]):
            """Helper function for calling assetbundlerbatch with no sub-command"""
            cmd = [self.bundler_batch]
            return self.call_asset_bundler(self._append_arguments(cmd, kwargs))

        def call_seeds(self, **kwargs: Dict[str, str]) -> None:
            """Helper function for calling assetbundlerbatch with 'seeds' sub-command"""

            cmd = [self.bundler_batch, "seeds"]
            return self.call_asset_bundler(self._append_arguments(cmd, kwargs))

        def call_assetLists(self, **kwargs: Dict) -> None:
            """Helper function for calling assetbundlerbatch with 'assetLists' sub-command"""

            cmd = [self.bundler_batch, "assetLists"]
            return self.call_asset_bundler(self._append_arguments(cmd, kwargs))

        def call_comparisonRules(self, **kwargs: Dict) -> None:
            """Helper function for calling assetbundlerbatch with 'comparisonRules' sub-command"""

            cmd = [self.bundler_batch, "comparisonRules"]
            return self.call_asset_bundler(self._append_arguments(cmd, kwargs))

        def call_compare(self, **kwargs: Dict) -> None:
            """Helper function for calling assetbundlerbatch with 'compare' sub-command"""

            cmd = [self.bundler_batch, "compare"]
            return self.call_asset_bundler(self._append_arguments(cmd, kwargs))

        def call_bundleSettings(self, **kwargs: Dict) -> None:
            """Helper function for calling assetbundlerbatch with 'bundleSettings' sub-command"""

            cmd = [self.bundler_batch, "bundleSettings"]
            return self.call_asset_bundler(self._append_arguments(cmd, kwargs))

        def call_bundles(self, **kwargs: Dict) -> None:
            """Helper function for calling assetbundlerbatch with 'bundles' sub-command"""

            cmd = [self.bundler_batch, "bundles"]
            return self.call_asset_bundler(self._append_arguments(cmd, kwargs))

        def call_bundleSeed(self, **kwargs: Dict) -> None:
            """Helper function for calling assetbundlerbatch with 'bundleSeed' sub-command"""

            cmd = [self.bundler_batch, "bundleSeed"]
            return self.call_asset_bundler(self._append_arguments(cmd, kwargs))

        def _append_arguments(self, cmd: List[str], kwargs: Dict, append_defaults: bool = True) -> List[str]:
            """Appends and returns all keyword arguments to the list of string [cmd]"""
            for key, value in kwargs.items():
                if value:
                    cmd.append(f"--{key}={value}")
                else:
                    cmd.append(f"--{key}")
            if append_defaults:
                cmd.append(f"--project-path={workspace.paths.project()}")
            return cmd

        # ******

        def get_seed_relative_paths(self, seed_file: str) -> str:
            """Iterates all asset relative paths in the [seed_file]."""
            assert seed_file.endswith(".seed"), f"file {seed_file} is not a seed file"
            # Get value from all XML nodes who are grandchildren of all Class tags and have
            # a field attr. equal to "pathHint"
            for node in ET.parse(seed_file).getroot().findall(r"./Class/Class/*[@field='pathHint']"):
                yield node.attrib["value"]

        def get_seed_relative_paths_for_platform(self, seed_file: str, platform_flags : int) -> str:
            """Iterates all asset relative paths in the [seed_file] which match the platform flags"""
            assert seed_file.endswith(".seed"), f"file {seed_file} is not a seed file"
            # Get value from all XML nodes who are grandchildren of all Class tags and have
            # a field attr. equal to "pathHint"
            seedFileListContents = []

            data = ET.parse(seed_file)
            root = data.getroot()
            seedFileRootNode = root.find("Class")
            for seedFileInfoNode in seedFileRootNode.findall("*"):
                if (int(seedFileInfoNode.find('./Class[@field="platformFlags"]').attrib["value"]) & platform_flags ):
                    pathHint = seedFileInfoNode.find('./Class[@field="pathHint"]').attrib["value"]
                    seedFileListContents.append(pathHint)
            return seedFileListContents

        def get_asset_relative_paths(self, asset_list_file: str) -> str:
            """Iterates all asset relative paths in the [asset_list_file]."""
            assert asset_list_file.endswith(".assetlist"), f"file {asset_list_file} is not an assetlist file"
            # Get value from all XML nodes who are great-grandchildren of all Class tags and have
            # a field attr. equal to "assetRelativePath"
            # fmt:off
            for node in (ET.parse(asset_list_file).getroot().findall(
                    r"./Class/Class/Class/*[@field='assetRelativePath']")):
                yield node.attrib["value"]
            # fmt:on

        def get_dependent_bundle_names(self, manifest_file: str) -> str:
            """Iterates all dependent bundle names in the [manifest_file]"""
            assert manifest_file.endswith(".xml"), f"File {manifest_file} does not have an XML extension"
            # Get value from all XML nodes whose parent field attr. is "DependentBundleNames" and whose tag is Class
            for node in ET.parse(manifest_file).getroot().findall(r".//*[@field='DependentBundleNames']/Class"):
                yield node.attrib["value"]

        def platform_file_name(self, file_name: str, platform: str) -> str:
            """Converts the standard [file_name] to a platform specific file name"""
            split = file_name.split(".", 1)
            platform_name = ASSET_PROCESSOR_PLATFORM_MAP.get(platform)
            if not platform_name:
                logger.warning(f"platform {platform} not recognized. File name could not be generated")
                return file_name
            return f'{split[0]}_{platform_name}.{split[1]}'

        def extract_file_content(self, bundle_file: str, file_name_to_extract: str) -> str:
            """Extract the contents of a single file from a bundle"""

            with zipfile.ZipFile(bundle_file) as bundle_zip:
                with bundle_zip.open(file_name_to_extract, "r") as extracted_file:
                    return extracted_file.read()

        def get_crc_of_files_in_archive(self, archive_name: str) -> Dict[str, int]:
            """
            Extracts the CRC-32 'checksum' for all files in the archive as dictionary.
            The returned dictionary will have:
                key - filename
                value - the crc checksum for that file
            """
            file_crcs = {}
            zf = zipfile.ZipFile(archive_name)
            for info in zf.infolist():
                file_crcs[info.filename] = info.CRC
            return file_crcs

        def get_platform_flag(self, platform_name: str) -> int:
            if (platform_name == "pc"):
                return 1
            elif (platform_name == "android"):
                return 2
            elif (platform_name == "ios"):
                return 4
            elif (platform_name == "mac"):
                return 8
            elif (platform_name == "server"):
                return 128

        def extract_and_check(self, extract_dir: str, bundle_file: str) -> None:
            """
            Helper function to extract the manifest from a bundle
            and validate against the actual files in the bundle
            """
            # Ensure that the parent bundle was created
            assert os.path.isfile(bundle_file), f"{bundle_file} was not created by the 'bundles' call"

            os.mkdir(extract_dir)

            with zipfile.ZipFile(bundle_file) as bundle_zip:
                bundle_zip.extract("manifest.xml", path=extract_dir)

            # Populate the bundle file paths
            dependent_bundle_name = [bundle_file]

            manifest_file_path = os.path.join(extract_dir, "manifest.xml")
            for bundle_name in self.get_dependent_bundle_names(manifest_file_path):
                dependent_bundle_name.append(os.path.join(self.test_dir, bundle_name))

            # relative asset paths
            assets_from_file = []
            for rel_path in self.get_asset_relative_paths(self.asset_info_file_result):
                assets_from_file.append(os.path.normpath(rel_path))

            # extract all files from the bundles
            for bundle in dependent_bundle_name:
                file_info = os.stat(bundle)
                # Verify that the size of all bundles is less than the max size specified
                assert file_info.st_size <= (self.max_bundle_size_in_mib * self.number_of_bytes_in_mib)
                with zipfile.ZipFile(bundle) as bundle_zip:
                    bundle_zip.extractall(extract_dir)

            ignore_list = ["assetCatalog.bundle", "manifest.xml", "DeltaCatalog.xml"]
            assets_in_disk = utils.get_relative_file_paths(extract_dir, ignore_list=ignore_list)

            # Ensure that all assets were present in the bundles
            assert sorted(assets_from_file) == sorted(assets_in_disk)

        def _get_platform_flags(self) -> Dict[str, int]:
            """
            Extract platform numeric values from the file that declares them (PlatformDefaults.h):
            Platform Flags are defined in the C header file, where ORDER MATTERS.
            Dynamically parse them from the file's enum and calculate their integer value at run-time.
            Note: the platform's are bit flags, so their values are powers of 2: 1 << position_in_file
            """
            platform_declaration_file = os.path.join(
                workspace.paths.engine_root(),
                "Code",
                "Framework",
                "AzCore",
                "AzCore",
                "PlatformId",
                "PlatformDefaults.h",
            )

            platform_values = {}  # Dictionary to store platform flags. {<platform>: int}
            counter = 1  # Store current platform flag value
            get_platform = re.compile(r"^\s+(\w+),")
            start_gathering = False
            with open(platform_declaration_file, "r") as platform_file:
                for line in platform_file.readlines():
                    if start_gathering:
                        if "NumPlatforms" in line:
                            break  # NumPlatforms is the last
                    if start_gathering:
                        result = get_platform.match(line)  # Try the regex
                        if result:
                            platform_values[result.group(1).replace("_ID", "").lower()] = counter
                            counter = counter << 1
                    elif "(Invalid, -1)" in line:  # The line right before the first platform
                        start_gathering = True
            return platform_values

        def teardown(self) -> None:
            """Destroys the temporary directory used in this test"""
            fs.delete([self.test_dir], True, True)

        def __getitem__(self, item: str) -> str:
            """Get Item overload to use the object like a dictionary.
            Implemented so this fixture "looks and feels" like other AP fixtures that return dictionaries"""
            return self.__dict__[str(item)]

    # End class BundlerBatchFixture

    bundler_batch_fixture = BundlerBatchFixture()
    request.addfinalizer(bundler_batch_fixture.teardown)

    bundler_batch_fixture.platform_values = bundler_batch_fixture._get_platform_flags()

    # Enable platforms accepted from command line
    platforms_list = [platform for platform in platforms if platform in ALL_PLATFORMS]

    # Run a full scan to ENSURE that both caches (pc and osx) are COMPLETELY POPULATED
    # Needed for asset bundling
    # fmt:off
    assert asset_processor.batch_process(fastscan=True, timeout=timeout * len(platforms), platforms=platforms_list), \
        "AP Batch failed to process in bundler_batch_fixture"
    # fmt:on

    return bundler_batch_fixture  # Return the fixture
