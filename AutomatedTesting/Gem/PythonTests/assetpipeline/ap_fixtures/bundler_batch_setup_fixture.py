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

from assetpipeline.ap_fixtures.asset_processor_fixture import asset_processor
from assetpipeline.ap_fixtures.timeout_option_fixture import timeout_option_fixture as timeout
from assetpipeline.ap_fixtures.ap_config_backup_fixture import ap_config_backup_fixture as config_backup
from assetpipeline.ap_fixtures.ap_setup_fixture import ap_setup_fixture

import ly_test_tools.environment.file_system as fs
import ly_test_tools.o3de.pipeline_utils as utils
from ly_test_tools.o3de.asset_processor import ASSET_PROCESSOR_PLATFORM_MAP

logger = logging.getLogger(__name__)


@pytest.fixture
@pytest.mark.usefixtures("config_backup")
@pytest.mark.usefixtures("asset_processor")
@pytest.mark.usefixtures("ap_setup_fixture")
def bundler_batch_setup_fixture(request, workspace, ap_setup_fixture, asset_processor) -> Any:

    def get_all_platforms() -> list[str]:
        """Helper: This function generates a list of all platforms to be built for testing Asset Bundler."""
        ALL_PLATFORMS = []
        for _, value in ASSET_PROCESSOR_PLATFORM_MAP.items():
            ALL_PLATFORMS.append(value)
        return ALL_PLATFORMS

    def platforms_to_build() -> list | list[str]:
        """Helper: Gets the platforms to build for bundle tests."""
        platforms = request.config.getoption("--bundle_platforms")
        if platforms is not None:
            # Remove whitespace and split at commas if platforms provided
            platforms = [platform.strip() for platform in platforms.split(",")]
        else:
            # No commandline argument provided, default to mac and pc
            platforms = ["pc", "mac", "linux"]
        return platforms

    def setup_temp_workspace() -> None:
        """Helper: Sets up the temp workspace for asset bundling tests."""
        asset_processor.create_temp_asset_root()

    def setup_temp_dir() -> Any:
        """Helper: Sets up a temp directory for use in bundling tests that can't use the temp workspace."""
        tempDir = os.path.join(TF.gettempdir(), "AssetBundlerTempDirectory")
        if os.path.exists(tempDir):
            fs.delete([tempDir], True, True)
        if not os.path.exists(tempDir):
            os.mkdir(tempDir)
        return tempDir

    class WorkSpaceBundlerBatchFixture:
        """
        Houses useful variables and functions for running Asset Bundler Batch Tests
        """
        def __init__(self):
            self.bin_dir = workspace.paths.build_directory()
            self.bundler_batch = os.path.join(self.bin_dir, "AssetBundlerBatch")
            self.seed_list_file_name = "testSeedListFile.seed"
            self.asset_info_file_name = "assetFileInfo.assetlist"
            self.bundle_file_name = "bundle.pak"
            self.bundle_settings_file_name = "bundleSettingsFile.bundlesettings"
            self.workspace = workspace
            self.platforms = platforms_to_build()
            self.platforms_as_string = ",".join(self.platforms)

            # Useful sizes
            self.max_bundle_size_in_mib = 35
            self.number_of_bytes_in_mib = 1024 * 1024

            # Checks whether or not the fixture was parametrized to use the temp workspace. Defaults to True.
            self.use_temp_workspace = request.param if hasattr(request, "param") else True

            if self.use_temp_workspace:
                setup_temp_workspace()
                self.project_path = os.path.join(asset_processor.temp_asset_root(), workspace.project)
                self.cache = asset_processor.temp_project_cache_path()
                self.seed_list_file = os.path.join(self.project_path, self.seed_list_file_name)
                self.asset_info_file_request = os.path.join(self.project_path, self.asset_info_file_name)
                self.bundle_settings_file_request = os.path.join(self.project_path, self.bundle_settings_file_name)
                self.bundle_file = os.path.join(self.project_path, self.bundle_file_name)
                self.asset_info_file_result = os.path.join(
                    self.project_path, self.platform_file_name(self.asset_info_file_name,
                                                               workspace.asset_processor_platform)
                )
                self.bundle_settings_file_result = os.path.join(
                    self.project_path, self.platform_file_name(self.bundle_settings_file_name,
                                                               workspace.asset_processor_platform)
                )

            else:
                self.project_path = workspace.paths.project()
                self.cache = workspace.paths.project_cache()
                self.test_dir = setup_temp_dir()
                self.seed_list_file = os.path.join(self.test_dir, self.seed_list_file_name)
                self.asset_info_file_request = os.path.join(self.test_dir, self.asset_info_file_name)
                self.bundle_settings_file_request = os.path.join(self.test_dir, self.bundle_settings_file_name)
                self.bundle_file = os.path.join(self.test_dir, self.bundle_file_name)
                self.asset_info_file_result = os.path.join(
                    self.test_dir, self.platform_file_name(self.asset_info_file_name,
                                                           workspace.asset_processor_platform)
                )
                self.bundle_settings_file_result = os.path.join(
                    self.test_dir, self.platform_file_name(self.bundle_settings_file_name,
                                                           workspace.asset_processor_platform)
                )

        @staticmethod
        def call_asset_bundler(arg_list):
            """
            Prints out the call to asset bundler, so if an error occurs, the commands that were run can be repeated.
            """
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

        def call_bundlerbatch(self, **kwargs: Dict[str, str]) -> tuple[bool, str] | tuple[bool, Any]:
            """Helper function for calling assetbundlerbatch with no sub-command"""
            cmd = [self.bundler_batch]
            return self.call_asset_bundler(self._append_arguments(cmd, kwargs))

        def call_seeds(self, **kwargs: Dict[str, str]) -> tuple[bool, str] | tuple[bool, Any]:
            """Helper function for calling assetbundlerbatch with 'seeds' sub-command"""

            cmd = [self.bundler_batch, "seeds"]
            return self.call_asset_bundler(self._append_arguments(cmd, kwargs))

        def call_assetLists(self, **kwargs: Dict) -> tuple[bool, str] | tuple[bool, Any]:
            """Helper function for calling assetbundlerbatch with 'assetLists' sub-command"""

            cmd = [self.bundler_batch, "assetLists"]
            return self.call_asset_bundler(self._append_arguments(cmd, kwargs))

        def call_comparisonRules(self, **kwargs: Dict) -> tuple[bool, str] | tuple[bool, Any]:
            """Helper function for calling assetbundlerbatch with 'comparisonRules' sub-command"""

            cmd = [self.bundler_batch, "comparisonRules"]
            return self.call_asset_bundler(self._append_arguments(cmd, kwargs))

        def call_compare(self, **kwargs: Dict) -> tuple[bool, str] | tuple[bool, Any]:
            """Helper function for calling assetbundlerbatch with 'compare' sub-command"""

            cmd = [self.bundler_batch, "compare"]
            return self.call_asset_bundler(self._append_arguments(cmd, kwargs))

        def call_bundleSettings(self, **kwargs: Dict) -> tuple[bool, str] | tuple[bool, Any]:
            """Helper function for calling assetbundlerbatch with 'bundleSettings' sub-command"""

            cmd = [self.bundler_batch, "bundleSettings"]
            return self.call_asset_bundler(self._append_arguments(cmd, kwargs))

        def call_bundles(self, **kwargs: Dict) -> tuple[bool, str] | tuple[bool, Any]:
            """Helper function for calling assetbundlerbatch with 'bundles' sub-command"""

            cmd = [self.bundler_batch, "bundles"]
            return self.call_asset_bundler(self._append_arguments(cmd, kwargs))

        def call_bundleSeed(self, **kwargs: Dict) -> tuple[bool, str] | tuple[bool, Any]:
            """Helper function for calling assetbundlerbatch with 'bundleSeed' sub-command"""

            cmd = [self.bundler_batch, "bundleSeed"]
            return self.call_asset_bundler(self._append_arguments(cmd, kwargs))

        def _append_arguments(self, cmd: List[str], kwargs: Dict, append_defaults: bool = True) -> List[str]:
            """Appends and returns all keyword arguments to the list of string [cmd]"""
            for key, value in kwargs.items():
                if not value:
                    cmd.append(f"--{key}")

                else:
                    if type(value) != list:
                        cmd.append(f"--{key}={value}")
                    else:
                        for item in value:
                            cmd.append(f"--{key}={item}")

            if append_defaults:
                cmd.append(f"--project-path={self.project_path}")
            return cmd

        @staticmethod
        def get_seed_relative_paths(seed_file: str) -> str:
            """Iterates all asset relative paths in the [seed_file]."""
            assert seed_file.endswith(".seed"), f"file {seed_file} is not a seed file"
            # Get value from all XML nodes who are grandchildren of all Class tags and have
            # a field attr. equal to "pathHint"
            for node in ET.parse(seed_file).getroot().findall(r"./Class/Class/*[@field='pathHint']"):
                yield node.attrib["value"]

        @staticmethod
        def get_seed_relative_paths_for_platform(seed_file: str, platform_flags: int) -> list[str]:
            """Iterates all asset relative paths in the [seed_file] which match the platform flags"""
            assert seed_file.endswith(".seed"), f"file {seed_file} is not a seed file"
            # Get value from all XML nodes who are grandchildren of all Class tags and have
            # a field attr. equal to "pathHint"
            seedFileListContents = []

            data = ET.parse(seed_file)
            root = data.getroot()
            seedFileRootNode = root.find("Class")
            for seedFileInfoNode in seedFileRootNode.findall("*"):
                if (int(seedFileInfoNode.find('./Class[@field="platformFlags"]').attrib["value"]) & platform_flags):
                    pathHint = seedFileInfoNode.find('./Class[@field="pathHint"]').attrib["value"]
                    seedFileListContents.append(pathHint)
            return seedFileListContents

        @staticmethod
        def get_asset_relative_paths(asset_list_file: str) -> str:
            """Iterates all asset relative paths in the [asset_list_file]."""
            assert asset_list_file.endswith(".assetlist"), f"file {asset_list_file} is not an assetlist file"
            # Get value from all XML nodes who are great-grandchildren of all Class tags and have
            # a field attr. equal to "assetRelativePath"

            for node in (ET.parse(asset_list_file).getroot().findall(
                    r"./Class/Class/Class/*[@field='assetRelativePath']")):
                yield node.attrib["value"]

        @staticmethod
        def get_dependent_bundle_names(manifest_file: str) -> str:
            """Iterates all dependent bundle names in the [manifest_file]"""
            assert manifest_file.endswith(".xml"), f"File {manifest_file} does not have an XML extension"
            # Get value from all XML nodes whose parent field attr. is "DependentBundleNames" and whose tag is Class
            for node in ET.parse(manifest_file).getroot().findall(r".//*[@field='DependentBundleNames']/Class"):
                yield node.attrib["value"]

        @staticmethod
        def platform_file_name(file_name: str, platform: str) -> str:
            """Converts the standard [file_name] to a platform specific file name"""
            split = file_name.split(".", 1)
            platform_name = platform if platform in ASSET_PROCESSOR_PLATFORM_MAP.values() else ASSET_PROCESSOR_PLATFORM_MAP.get(platform)
            if not platform_name:
                logger.warning(f"platform {platform} not recognized. File name could not be generated")
                return file_name
            return f'{split[0]}_{platform_name}.{split[1]}'

        @staticmethod
        def extract_file_content(bundle_file: str, file_name_to_extract: str) -> bytes:
            """Extract the contents of a single file from a bundle as a ByteString."""

            with zipfile.ZipFile(bundle_file) as bundle_zip:
                with bundle_zip.open(file_name_to_extract, "r") as extracted_file:
                    return extracted_file.read()

        @staticmethod
        def get_crc_of_files_in_archive(archive_name: str) -> Dict[str, int]:
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

        @staticmethod
        def get_platform_flag(platform_name: str) -> int:
            """ Helper to fetch the platform flag from a provided platform name. """
            platform_flags = {
                "pc": 1,
                "android": 2,
                "ios": 4,
                "mac": 8,
                "server": 128
            }
            if platform_name in platform_flags:
                return platform_flags.get(platform_name)
            raise ValueError(f"{platform_name} not found within expected platform flags. "
                             f"Expected: {platform_flags.keys()}")

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

            expected_size = self.max_bundle_size_in_mib * self.number_of_bytes_in_mib
            # extract all files from the bundles
            for bundle in dependent_bundle_name:
                file_info = os.stat(bundle)
                # Verify that the size of all bundles is less than the max size specified
                assert file_info.st_size <= expected_size, \
                    f"file_info.st_size {file_info.st_size} for bundle {bundle} was expected to be smaller than {expected_size}"
                with zipfile.ZipFile(bundle) as bundle_zip:
                    bundle_zip.extractall(extract_dir)

            ignore_list = ["assetCatalog.bundle", "manifest.xml", "DeltaCatalog.xml"]
            assets_in_disk = utils.get_relative_file_paths(extract_dir, ignore_list=ignore_list)

            # Ensure that all assets were present in the bundles
            assert sorted(assets_from_file) == sorted(assets_in_disk)

        @staticmethod
        def _get_platform_flags() -> Dict[str, int]:
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
            if self.use_temp_workspace:
                fs.delete([asset_processor.temp_asset_root()], True, True)
            else:
                fs.delete([self.test_dir], True, True)

        def __getitem__(self, item: str) -> str:
            """Get Item overload to use the object like a dictionary.
            Implemented so this fixture "looks and feels" like other AP fixtures that return dictionaries"""
            return self.__dict__[str(item)]

    # End class BundlerBatchFixture

    bundler_batch_fixture = WorkSpaceBundlerBatchFixture()
    request.addfinalizer(bundler_batch_fixture.teardown)

    bundler_batch_fixture.platform_values = bundler_batch_fixture._get_platform_flags()

    return bundler_batch_fixture  # Return the fixture
