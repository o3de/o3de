"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import json
import os
import re
import xml.etree.ElementTree as xml
import itertools
from typing import List
from dataclasses import dataclass
import xml.parsers.expat as expat
from automatedtesting_shared import asset_database_utils as asset_db_utils
from ly_test_tools.o3de.asset_processor import ASSET_PROCESSOR_PLATFORM_MAP
import ly_test_tools.o3de.pipeline_utils as utils
import warnings
import logging

logger = logging.getLogger(__name__)


@dataclass
class BlackboxDebugTest:
    values_to_find = {}


def populate_value_list_from_json(data_products):
    """
    Helper function used by the test_builder.py module to populate an expected product assets list from a json file.
    :param data_products: The "product" key from the target json dict.
    :return: list_products: A list of products.
    """

    list_products = []
    for product in data_products:
        pass
    return list_products


def populate_expected_product_assets(assets_to_validate: List[str]):
    # Check that each given source asset resulted in the expected jobs and products.
    expected_product_list = []
    for expected_source in assets_to_validate:
        for expected_job in expected_source.jobs:
            for expected_product in expected_job.products:
                expected_product_list.append(expected_product.product_name)

    return expected_product_list


def trim_floating_point_values_from_same_length_lists(diff_actual: List[str], diff_expected: List[str],
                                                      actual_file_path: str, expected_file_path: str) -> (
        List[str], List[str]):
    # Linux and non-Linux platforms generate slightly different values for floating points.
    # Long term, it will be important to stabilize the output of product assets, because this difference
    # will cause problems: If an Android asset is generated from a Linux versus Windows machine, for example,
    # it will be different when it's not expected to be different.
    # In the short term, it's not something addressed yet, so instead this function will
    # truncate any floating point values to be short enough to be stable.
    # It will then emit a warning, to help keep track of this issue.

    # Get the initial list lengths, so they can be compared to the list lengths later to see if any differences were
    # removed due to floating point value drift.
    initial_diff_actual_len = len(diff_actual)
    initial_diff_expected_len = len(diff_expected)

    # This function requires the two lists to be equal length.
    assert initial_diff_actual_len == initial_diff_expected_len, "Scene mismatch - different line counts"

    # Floating point values between Linux and Windows aren't consistent yet.
    # For now, trim these values for comparison.
    # Store the trimmed values and compare the un-trimmed values separately, emitting warnings.
    # Trim decimals from the lists to be compared, if any where found, re-compare and generate new lists.
    DECIMAL_DIGITS_TO_PRESERVE = 3
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
                                         actual_hashes_to_skip: List[str], expected_hashes_to_skip: List[str]) -> (
        List[str], List[str]):
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
        warnings.warn(
            f"Hash values no longer match for debug scene graph between files {expected_file_path} and {actual_file_path}")

    return expected_hashes, actual_hashes


def compare_scene_debug_file(asset_processor, expected_file_path: str, actual_file_path: str,
                             expected_hashes_to_skip: List[str] = None, actual_hashes_to_skip: List[str] = None):
    # Given the paths to the debug scene graph generated by re-processing the test scene file and the path to the
    # last known good debug scene graph for that file, load both debug scene graphs into memory and scan them for differences.
    # Warns on known issues, and fails on unknown issues.
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

    # There are some differences that are currently considered warnings.
    # Long term these should become errors, but right now product assets on Linux and non-Linux
    # are showing differences in hashes.
    # Linux and non-Linux platforms are also generating different floating point values.
    diff_actual, diff_expected = trim_floating_point_values_from_same_length_lists(
        diff_actual, diff_expected, actual_file_path, expected_file_path)

    # If this is the XML debug file, then it will be difficult to verify if a line is a hash line or another integer.
    # However, because XML files are always compared after standard dbgsg files, the hashes from that initial comparison can be used here to check.
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


"""
def build_dict(lists):

    built_dict = {}
    i = 0
    try:
        for sub_list in lists:
            sub_dict = {}
            for line in sub_list:
                if ":" in line:
                    key, value = line.strip().split(":", 1)
                    sub_dict[key] = value
                elif line == '\n':
                    pass
                else:
                    key = line
                    value = None
                    sub_dict[key] = value
            built_dict[sub_dict['Node Name']] = sub_dict
            i += 1

    except: ValueError

    return built_dict

#def build_dict_from_xml(xml_file):
def test_build_dict_from_xml():
    
    #A function that reads the contents of an xml file to create a dictionary.
    #Returns the dict data as an instance of the BlackboxDebugTest dataclass.
    #:param xml_file: An xml file that provides the data required to compare test results vs expected values.
    #:return: Dictionary.
    
    debug_file = "C:/new/onemeshonematerial.dbgsg"
    with open(debug_file, "r") as file:
        data = file.read()

    split_data = data.strip().splitlines()

    split_list = []
    for key, value in itertools.groupby(split_data, key=lambda line: line == ""):
        if not key:
            split_list.append(list(value))

    lines = build_dict(split_list)

    test_file = "C:/new/onemeshonematerial.json"
    with open(test_file, "w") as test:
        json.dump(lines, test, indent=4)
"""
