"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from itertools import chain
import logging
from os import path, PathLike, walk
from typing import List, Pattern, Tuple

from ly_test_tools._internal.managers.workspace import AbstractWorkspaceManager
from ly_test_tools.o3de.asset_processor import ASSET_PROCESSOR_PLATFORM_MAP

from assetpipeline.assetpipeline_utils.assetpipeline_constants import FLOATING_POINT_REGEX

from automatedtesting_shared.asset_database_utils import DBJob, DBSourceAsset

logger = logging.getLogger(__name__)


def trim_floating_point_values(list_to_trim_floats: List[str],
                               regex_pattern: Pattern[str]) -> List[str]:
    """
    Helper function that store the trimmed and un-trimmed values of floats in .dbgsg files generated while running
    scene tests.
    Truncates everything before, between, and after the two matched regex groups that will be combined.
    :param list_to_trim_floats: The list of lines with floats that will be trimmed by this function.
    :param regex_pattern: The pattern by which to search and trim the floats.
    """

    for index, diff_actual_line in enumerate(list_to_trim_floats):
        # Loop, because there may be multiple floats on the same line.
        while match_result := regex_pattern.match(list_to_trim_floats[index]):
            list_to_trim_floats[index] = f"{match_result.group(1)}{match_result.group(2)}"

    return list_to_trim_floats


def stabilize_floating_point_values_from_same_length_lists(diff_expected: List[str],
                                                           diff_actual: List[str],
                                                           expected_file_path: PathLike[str],
                                                           actual_file_path: PathLike[str]) -> (List[str], List[str]):
    """
    This function will truncate any floating point values to be short enough to be stable.

    o3de/o3de#13713: We need to stabilize the output of product assets, so this will emit a warning.

    :param diff_expected: A list containing the expected debug output of the source asset under test.
    :param diff_actual: A list containing the debug output (dbgsg file) of the processed source asset under test.
    :param expected_file_path: Path to the last known good dbgsg file containing the debug output to compare against.
    :param actual_file_path: Path to the dbgsg file containing the debug output of the processed asset under test.
    """

    # Save the initial values to help detect floating point drift after truncation.
    initial_diff_expected = tuple(diff_expected)
    initial_diff_actual = tuple(diff_actual)

    def _detect_floating_point_drift(first: List[str] | Tuple[str],
                                     second: List[str] | Tuple[str],
                                     file_path: PathLike[str]) -> None:
        """
        Helper function that compares the total number of characters in each list.
        Emits a logger warning if the numbers are different, as it would imply floating point drift was detected and
        floats were truncated.
        """
        if sum(len(i) for i in first) != sum(len(i) for i in second):
            logger.warning(f"Floating point drift detected in {file_path}.")

    # Trim the floating point values in each list, then emit a warning if any float drift is detected.
    diff_expected = trim_floating_point_values(diff_expected, FLOATING_POINT_REGEX)
    _detect_floating_point_drift(initial_diff_expected, diff_expected, expected_file_path)

    diff_actual = trim_floating_point_values(diff_actual, FLOATING_POINT_REGEX)
    _detect_floating_point_drift(initial_diff_actual, diff_actual, actual_file_path)

    return diff_expected, diff_actual


def split_hash_at_numeric_value(list_with_hash: List[str],
                                regex_pattern: Pattern[str]) -> (List[str], List[str]):
    """
    Helper that detects hashes within the provided list.
    Group 1 of the regex adds everything  but the numeric hash to a list.
    Group 2 of the regex adds the numeric value of the hashes to a different list.
    """

    hashes_removed = []
    hash_list = []

    for list_entry in list_with_hash:
        match_result = regex_pattern.match(list_entry)
        assert match_result, \
            f"Scene mismatch, could not match the regex '{regex_pattern}' to any entry in the list: {list_with_hash}"
        hashes_removed.append(match_result.group(1))
        hash_list.append(match_result.group(2))

    return hashes_removed, hash_list


def parse_scene_graph(scene_debug_graph_path: str | bytes | PathLike[str] | PathLike[bytes]) -> List[str]:
    """
    Helper function that parses a dbgsg file for test.
    Returns a list of strings.
    """

    logger.info(f"Parsing scene graph: {scene_debug_graph_path}")
    with open(scene_debug_graph_path, "r") as scene_file:
        debug_lines = scene_file.readlines()
    return debug_lines


def find_files_in_dir(file_dir: str | bytes | PathLike[str] | PathLike[bytes],
                      extension_to_find: str) -> List[str | bytes | PathLike[str] | PathLike[bytes]]:
    """
    Recursively finds all files within a directory that end with the requested file extension.
    :returns:
        Returns a list of file paths found within the directory.
    """
    file_path_list = [path.join(root, file)
                      for root, dirs, files in walk(file_dir)
                      for file in files if file.endswith(extension_to_find)]

    return file_path_list


def populate_expected_product_assets(workspace: AbstractWorkspaceManager,
                                     assets_to_validate: List[DBSourceAsset]) -> List[str]:
    """
    Populate a list of expected product based on jobs completed for each source asset.
    assets_to_validate: The expected job data for each source asset.
    Example data:
        "job_key": "Example Job",
          "builder_guid": "Example_Guid",
          "status": 4,
          "error_count": 0,
          "products": [
            {
              "product_name": "example_folder/example_asset.extension",
              "sub_id": 0,
              "asset_type": "Example_Asset_Type"
            },
    """

    def _append_platform_to_product_name(job: DBJob) -> List[str]:
        """
        Helper function that appends the platform on which the product was processed, to the product's name.
        Used to assign the platform for expected product before comparison.
        """
        product_list = []
        job.platform = ASSET_PROCESSOR_PLATFORM_MAP[workspace.asset_processor_platform]
        for product in job.products:
            product.product_name = job.platform + "/" + product.product_name
            product_list.append(product.product_name)
        return product_list

    expected_product_list = (
            _append_platform_to_product_name(expected_job)
            for expected_source in assets_to_validate
            for expected_job in expected_source.jobs)

    return list(chain.from_iterable(expected_product_list))
