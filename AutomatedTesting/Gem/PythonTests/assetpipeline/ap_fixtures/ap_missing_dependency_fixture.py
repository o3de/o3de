"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Pytest fixture missing dependency scanner tests

"""

import logging
import os
import pytest
import re
import tempfile
from typing import Dict, List, Tuple, Any, Set

from ly_test_tools.environment.file_system import create_backup, restore_backup, unlock_file
from automatedtesting_shared import asset_database_utils as db_utils
from ly_test_tools.o3de.ap_log_parser import APLogParser

from . import ap_setup_fixture as ap_setup_fixture

logger = logging.getLogger(__name__)


@pytest.fixture
def ap_missing_dependency_fixture(request, workspace, ap_setup_fixture) -> Any:
    ap_setup = ap_setup_fixture

    class MissingDependencyHelper:
        def __init__(self):
            # Indexes for MissingProductDependencies rows in DB
            self.UUID_INDEX = 5
            self.SUB_ID_INDEX = 6
            self.STR_INDEX = 7

            # Constant tuple to represent a 'No missing dependencies' in a database entry
            self.DB_NO_MISSING_DEP = ("No missing dependencies found", "{00000000-0000-0000-0000-000000000000}:0")

            self.log_file = workspace.paths.ap_batch_log()
            self.ap_batch = os.path.join(workspace.paths.build_directory(), "AssetProcessorBatch")
            self.asset_db = workspace.paths.asset_db()
            self.backup_dir = os.path.join(tempfile.gettempdir(), "MissingDependencyBackup")

        def extract_missing_dependencies_from_output(self, log_output: List[str or List[str]]) -> Dict[
            str, List[Tuple[str, str]]]:
            """
            Extracts missing dependencies for each product file scanned from the console output.
            log_output: List[str or List[str]],
            Returns a dictionary, where:
                each key: product file with missing dependencies
                each value: a list of tuples where:
                    each tuple[0] - Missing dependency string
                    each tuple[1] - Missing dependency asset id
            """
            # Useful regex workers
            re_file_path = re.compile(r"Scanning for missing dependencies:\s*(.*)")
            re_content = re.compile(r'Missing dependency: String "(.*)" matches asset: (.*)\Z')

            result = {}  # The dictionary to return
            missing_dependencies = None  # Temp list to hold current_products missing dependency tuples
            current_product = None  # Current product in the log file

            for line in log_output:

                next_file = re_file_path.search(line, re.I)  # Attempt pattern search for next 'product'
                if next_file:
                    # Found next product in file
                    if current_product is not None:
                        # If already in a dependency file log section, save it before starting the next
                        result[current_product] = missing_dependencies

                    # Start a new product in this file
                    missing_dependencies = []
                    current_product = next_file.group(1)
                    assert current_product, "REGEX 'current_product' parsing error-- Did the log format change?"
                else:
                    # Not a new product yet in log:
                    # Try to parse dependency "String" and "missing asset"
                    content = re_content.search(line, re.I)
                    if content:
                        # Regex success, extract groups and add to [missing_dependencies]
                        missing_dependencies.append((content.group(1), content.group(2)))
            # End: for line in log_file

            # Hit the end of file, we might still have one more product to save
            if current_product is not None:
                # Append last dependency file log section
                result[current_product] = missing_dependencies

            return result

        def extract_missing_dependencies_from_log(self) -> Dict[str, List[Tuple[str, str]]]:
            """
            Extracts missing dependencies for each product file scanned from ONLY THE LAST AP Batch run.
            Returns a dictionary, where:
                each key: product file with missing dependencies
                each value: a list of tuples where:
                    each tuple[0] - Missing dependency string
                    each tuple[1] - Missing dependency asset id
            """
            return self.extract_missing_dependencies_from_output(APLogParser(self.log_file).get_lines(run=-1))

        def extract_missing_dependencies_from_database(self, product: str, check_platforms: List[str] = None) -> Dict[
            str, Set[Tuple[str, str]]]:
            """
            Extracts missing dependencies for the [product] from the database.
            Returns a dictionary, where:
                each key: product file with cache platform path. (i.e. pc, ios... etc.)
                each value: a set of tuples where:
                    each tuple[0] - Missing dependency string
                    each tuple[1] - Missing dependency asset id
            """
            platforms = check_platforms or db_utils.get_active_platforms_from_db(self.asset_db)
            logger.info(f"Searching for product {product} in active platforms: {platforms}")
            product_names = [  # Get product name in database (<platform_cache>/<product>)
                f"{str(platform)}/{product}".replace("\\", "/") for platform in platforms
            ]
            missing_dependencies = {}
            # Fetch missing dependencies for each platform for the target product
            for product_name in product_names:
                logger.info(f"Checking missing dependencies for '{product_name}'")

                missing_dependencies[product_name] = set()  # Set() to not allow duplicates

                product_id = db_utils.get_product_id(self.asset_db, product_name)
                # Assert exactly one match
                assert product_id, f"Expected to find exactly one DB match for {product_name}, " \
                                   f"instead found {len(product_id)}"

                db_dependencies = db_utils.get_missing_dependencies(self.asset_db, product_id)
                for db_dep in db_dependencies:
                    # Parse database entries to get ready for comparison #

                    # UUID stored as binary; convert to hex string
                    uuid = hex(int.from_bytes(db_dep[self.UUID_INDEX], byteorder="big", signed=False))

                    # Sub ID stored in DB as unsigned 32-bit int (may appear negative in python); make it positive
                    sub_id = int(db_dep[self.SUB_ID_INDEX]) & 0xFFFFFFFF

                    # File String stored as a string (no funky conversion needed)
                    file_str = db_dep[self.STR_INDEX]

                    # Asset-ID is in format: "{<uuid>}:<sub_id>" (sub_id as hex string: trim off first 2 chars: "0x")
                    asset_id = f"{{{self.uuid_format(uuid)}}}:{hex(sub_id)[2:]}"

                    # Only add to missing dependencies if this tuple is in fact a missing dependency.
                    # 'No missing dependencies' are represented as a specific "File String", "Asset-ID" pair
                    if file_str != self.DB_NO_MISSING_DEP[0] and asset_id != self.DB_NO_MISSING_DEP[1]:
                        logger.info(f"Found missing dependency '{file_str}' with asset ID '{asset_id}'")
                        missing_dependencies[product_name].add((file_str, asset_id))

            return missing_dependencies

        def uuid_format(self, hex_string: str) -> str:
            """
            Converts the UUID hex string into a common format.
            ex:  '0x785a05d2483e5b43a2b992acdae6e938}:[0' -> '785A05D2-483E-5B43-A2B9-92ACDAE6E938'
            """
            hex_string = hex_string.upper()
            # Remove unwanted text
            hex_string = hex_string.replace("-", "").replace("{", "").replace("}", "").replace("0X", "")
            hex_string = hex_string.split(":")[0]  # Anything after ':' is the sub id; remove
            if len(hex_string) < 32:
                # If the hex value didn't fill 32 characters, pad with leading zeroes
                hex_string = ("0" * (32 - len(hex_string))) + hex_string
            # fmt:off
            # Add hyphen separators
            return hex_string[0:8] + "-" + hex_string[8:12] + "-" + hex_string[12:16] + \
                   "-" + hex_string[16:20] + "-" + hex_string[20:]
            # fmt:on

        def validate_expected_dependencies(self, product: str, expected_dependencies: List[Tuple[str, str]],
                                           log_output: List[str or List[str]], platforms: List[str] = None) -> None:
            """
            Validates that the [product] has missing dependencies in both the AP Batch log and the asset database.
            :param product: The file to look for missing dependencies in.
            :param expected_dependencies: A list of tuples, where:
                    tuple[0] - the asset string for a missing dependency
                    tuple[1] - the asset id for a missing dependency
            ;param platforms: Platforms to validate in the DB.  Checks the db for all platforms processed if None.
            :return: None
            """
            logger.info(f"Searching output for expected dependencies for product {product}")
            sorted_expected = sorted(expected_dependencies)
            # Check dependencies found either in the log or console output
            for product_name, missing_deps in self.extract_missing_dependencies_from_output(log_output).items():
                if product in product_name:
                    sorted_missing = sorted(missing_deps)
                    # fmt:off
                    assert sorted_expected == sorted_missing, \
                        f"Missing dependencies for '{product_name}' did not match expected. Expected: " \
                        f"{sorted_expected}, Actual: {sorted_missing}"
                    # fmt:on

            # Check dependencies found in Database
            for product_name, missing_deps in self.extract_missing_dependencies_from_database(product,
                                                                                              platforms).items():
                if product.replace("\\", "/") in product_name:
                    sorted_missing = sorted(missing_deps)
                    # fmt:off
                    assert sorted_expected == sorted_missing, \
                        f"Product '{product_name}' expected missing dependencies: {sorted_expected}; " \
                        f"actual missing dependencies {sorted_missing}"
                    # fmt:on

        def __getitem__(self, item: str) -> object:
            """
            Get Item overload to use the object like a dictionary.
            Implemented so this fixture "looks and feels" like other AP fixtures that return dictionaries
            """
            return self.__dict__[str(item)]

    # End class MissingDependencyHelper

    md = MissingDependencyHelper()

    return md
