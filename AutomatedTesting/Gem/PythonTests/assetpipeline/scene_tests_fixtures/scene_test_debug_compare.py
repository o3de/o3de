"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
from __future__ import annotations

import os
import re
from typing import List
import ly_test_tools.o3de.pipeline_utils as utils
import warnings
import logging
from automatedtesting_shared import asset_database_utils as asset_db_utils

logger = logging.getLogger(__name__)


def populate_expected_product_assets(assets_to_validate: List[asset_db_utils.DBSourceAsset]) -> List[str]:
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
    expected_product_list = []
    for expected_source in assets_to_validate:
        for expected_job in expected_source.jobs:
            for expected_product in expected_job.products:
                expected_product_list.append(expected_product.product_name)

    return expected_product_list


def trim_floating_point_values_from_same_length_lists(diff_actual: List[str], diff_expected: List[str],
                                                      actual_file_path: str, expected_file_path: str) -> (List[str], List[str]):
    """
    This function will truncate any floating point values to be short enough to be stable.

    o3de/o3de#13713: We need to stabilize the output of product assets, so this will emit a warning.

    diff_actual: A list containing the debug output (dbgsg file) of the processed source asset under test.
    diff_expected: A list containing the expected debug output of the source asset under test.
    actual_file_path: Path to the dbgsg file containing the debug output of the processed source asset under test.
    expected_file_path: Path to the dbgsg file containing the expected debug output to compare against.
    """

    # Get the initial list lengths, so they can be compared to the list lengths later to see if any differences were
    # removed due to floating point value drift.
    initial_diff_actual_len = len(diff_actual)
    initial_diff_expected_len = len(diff_expected)

    # Decide the number of digits past the decimal point to preserve.
    DECIMAL_DIGITS_TO_PRESERVE = 3

    # This function requires the two lists to be equal length.
    assert initial_diff_actual_len == initial_diff_expected_len, "Scene mismatch - different line counts"

    # Store the trimmed values and compare the un-trimmed values separately, emitting warnings.
    # Trim decimals from the lists to be compared, if any where found, re-compare and generate new lists.
    floating_point_regex = re.compile(
        f"(.*?-?[0-9]+\\.[0-9]{{{DECIMAL_DIGITS_TO_PRESERVE},{DECIMAL_DIGITS_TO_PRESERVE}}})[0-9]+(.*)")
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


def scan_scene_debug_xml_file_for_issues(diff_actual: List[str], diff_expected: List[str],
                                         actual_hashes_to_skip: List[str], expected_hashes_to_skip: List[str]) -> (List[str], List[str]):
    """
    Checks if the differences in the XML file are the same as the hashes that were skipped in the debug file.

    o3de/o3de#13714: Hashes are generated differently on different platforms right now, and we need to stabilize the
    output of product assets, so this will emit a warning.

    diff_actual: A list containing the xml debug output (dbgsg.xml file) of the processed source asset under test.
    diff_expected: A list containing the expected xml debug output of the source asset under test.
    actual_hashes_to_skip: A list containing the hashes, of products generated during the test, that will be skipped during comparison.
    expected_hashes_to_skip: A list containing the expected hashes, of products generated during the test, that will be skipped during comparison.
    """

    # If the difference count doesn't match the non-XML file, then it's not just hash mis-matches in the XML file, and the test has failed.
    assert len(expected_hashes_to_skip) == len(diff_expected), "Scene mismatch"
    assert len(actual_hashes_to_skip) == len(diff_actual), "Scene mismatch"

    # Extract the number and compare it to the known list of hashes.
    # If this regex fails or the number isn't in the hash list, then it means this is a non-hash difference and should cause a test failure.
    xml_number_regex = re.compile(
        '.*<Class name="AZ::u64" field="m_data" value="([0-9]*)" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"\\/>')

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


def scan_scene_debug_scene_graph_file_differences_for_issues(diff_actual: List[str], diff_expected: List[str],
                                                             actual_file_path: str,
                                                             expected_file_path: str) -> (List[str], List[str]):
    """
    Compares an expected scene graph to the generated scene graph for differences in their hases.

    o3de/o3de#13714: Hashes are generated differently on different platforms right now, it will be important
    to stabilize the output of product assets, so this will emit a warning to help keep track of the related GHI.

    diff_actual: A list containing the debug output (dbgsg file) of the processed source asset under test.
    diff_expected: A list containing the expected debug output of the source asset under test.
    actual_file_path: Path to the dbgsg file containing the debug output of the processed source asset under test.
    expected_file_path: Path to the dbgsg file containing the expected debug output to compare against.
    """

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
        warnings.warn(
            f"Hash values no longer match for debug scene graph between files {expected_file_path} and {actual_file_path}")

    return expected_hashes, actual_hashes


def compare_scene_debug_file(asset_processor: object, expected_file_path: str, actual_file_path: str,
                             expected_hashes_to_skip: List[str] = None,
                             actual_hashes_to_skip: List[str] = None) -> tuple[None, None] | tuple[list[str], list[str]]:
    """
    Loads both the debug scene graph, generated by re-processing the test scene file, and the last known good debug
    scene graph for that file. Scans both files for differences.

    o3de/o3de#13713: We need to stabilize the output of product assets.
    o3de/o3de#13714: Hashes are generated differently on different platforms right now, it will be important
    to stabilize the output of product assets.
    Warnings are emitted on the known issues o3de/o3de#13713 and o3de/o3de#13714, a failure will emit on unknown issues.

    asset_processor: Instance of the asset_processor used in test.
    expected_file_path: Path to the dbgsg file containing the expected debug output to compare against.
    actual_file_path: Path to the dbgsg file containing the debug output of the processed source asset under test.
    expected_hashes_to_skip: A list, generated by the 'compare_scene_debug_file' function, containing hashes from the
    actual_hashes_to_skip: A list containing the hashes, of products generated during the test, that will be skipped
    during comparison.
    expected_hashes_to_skip: A list containing the expected hashes, of products generated during the test, that will
    be skipped during comparison.
    """
    debug_graph_path = actual_file_path
    expected_debug_graph_path = os.path.join(asset_processor.project_test_source_folder(), "SceneDebug",
                                             expected_file_path)

    logger.info(f"Parsing scene graph: {debug_graph_path}")
    with open(debug_graph_path, "r") as scene_file:
        actual_lines = scene_file.readlines()

    logger.info(f"Parsing scene graph: {expected_debug_graph_path}")
    with open(expected_debug_graph_path, "r") as scene_file:
        expected_lines = scene_file.readlines()
    diff_actual, diff_expected = utils.get_differences_between_lists(actual_lines, expected_lines)
    if diff_actual == None and diff_expected == None:
        return None, None

    diff_actual, diff_expected = trim_floating_point_values_from_same_length_lists(
        diff_actual, diff_expected, actual_file_path, expected_file_path)

    is_xml_dbgsg = os.path.splitext(expected_file_path)[-1].lower() == ".xml"

    if is_xml_dbgsg:
        return scan_scene_debug_xml_file_for_issues(diff_actual,
                                                    diff_expected,
                                                    actual_hashes_to_skip,
                                                    expected_hashes_to_skip)
    else:
        return scan_scene_debug_scene_graph_file_differences_for_issues(diff_actual,
                                                                        diff_expected,
                                                                        actual_file_path,
                                                                        expected_file_path)
