"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Helper functions for test result xml merging and processing.
"""
import glob
import os
import xml.etree.ElementTree as xet

TEST_RESULTS_DIR = 'Testing'


def _get_ctest_tag_content(cmake_build_path):
    """
    Get the content of the CTest TAG file. This file contains the name of the CTest test results directory.
    :param cmake_build_path: Path to the CMake build directory.
    :return: First line of the TAG file.
    """
    tag_file_path = os.path.join(cmake_build_path, TEST_RESULTS_DIR, 'TAG')

    if not os.path.exists(tag_file_path):
        raise FileNotFoundError(f'Could not find CTest TAG file at {tag_file_path}')
    
    first_line = None

    with open(tag_file_path) as tag_file:
        first_line = tag_file.readline().strip()

    return first_line


def _build_ctest_test_results_path(cmake_build_path):
    """
    Build the path to the CTest test results directory.
    :param cmake_build_path: Path to the CMake build directory.
    :return: Path to the CTest test results directory.
    """
    tag_content = _get_ctest_tag_content(cmake_build_path)
    if not tag_content:
        raise Exception('TAG file is empty.')

    ctest_results_path = os.path.join(cmake_build_path, TEST_RESULTS_DIR, tag_content)
    return ctest_results_path


def _build_gtest_test_results_path(cmake_build_path):
    """
    Build the path to the GTest test results directory.
    :param cmake_build_path: Path to the CMake build directory.
    :return: Path to the GTest test results directory.
    """
    gtest_results_path = os.path.join(cmake_build_path, TEST_RESULTS_DIR, 'Gtest')

    return gtest_results_path


def _build_pytest_test_results_path(cmake_build_path):
    """
    Build the path to the Pytest test results directory.
    :param cmake_build_path: Path to the CMake build directory.
    :return: Path to the Pytest test results directory.
    """
    pytest_results_path = os.path.join(cmake_build_path, TEST_RESULTS_DIR, 'Pytest')

    return pytest_results_path


def _get_all_test_results_paths(cmake_build_path, ctest_path_error_ok=False):
    """
    Build and return the test result paths for all test harnesses.
    :param cmake_build_path: Path to the CMake build directory.
    :param ctest_path_error_ok: Ignore errors that occur while building CTest results path.
    :return: Test result paths for all test harnesses.
    """
    paths = [
        _build_gtest_test_results_path(cmake_build_path),
        _build_pytest_test_results_path(cmake_build_path)
    ]

    try:
        paths.append(_build_ctest_test_results_path(cmake_build_path))
    except FileNotFoundError as e:
        if ctest_path_error_ok:
            print(e)
        else:
            raise

    return paths


def _merge_xml_results(xml_results_path, prefix, merged_xml_name, parent_element_name, child_element_name,
                       attributes_to_aggregate):
    """
    Merge the contents of XML test result files.
    :param xml_results_path: Path to the directory containing the files to merge.
    :param prefix: Test result file prefix.
    :param merged_xml_name: Name for the merged test result file.
    :param parent_element_name: Name of the XML element that will store the test results.
    :param child_element_name: Name of the XML element that contains the test results.
    :param attributes_to_aggregate: List of AttributeInfo items used for test result aggregation.
    """
    xml_files = glob.glob(os.path.join(xml_results_path, f'{prefix}*.xml'))

    if not xml_files:
        return

    temp_dict = {}
    for attribute in attributes_to_aggregate:
        temp_dict[attribute.name] = attribute.func(0)

    def _aggregate_attributes(nodes):
        for node in nodes:
            for attribute in attributes_to_aggregate:
                if attribute.name in node.attrib:
                    temp_dict[attribute.name] += attribute.func(node.attrib[attribute.name])
                else:
                    print("Failed to find key {} in {}, continuing...".format(attribute.name, node.tag))

    base_tree = xet.parse(xml_files[0])
    base_tree_root = base_tree.getroot()

    if base_tree_root.tag == parent_element_name:
        parent_element = base_tree_root
    else:
        parent_element = base_tree_root.find(parent_element_name)

    _aggregate_attributes(base_tree_root.findall(child_element_name))

    for xml_file in xml_files[1:]:
        root = xet.parse(xml_file).getroot()
        child_nodes = root.findall(child_element_name)

        _aggregate_attributes(child_nodes)
        parent_element.extend(child_nodes)

    for attribute in attributes_to_aggregate:
        parent_element.attrib[attribute.name] = str(temp_dict[attribute.name])

    base_tree.write(os.path.join(xml_results_path, merged_xml_name), encoding='UTF-8', xml_declaration=True)


def clean_test_results(cmake_build_path):
    """
    Clean the test results directories.
    :param cmake_build_path: Path to the CMake build directory.
    """

    # Using ctest_path_error_ok=True since the CTest path might not exist before tests are run for the first
    # time in a clean build.

    for path in _get_all_test_results_paths(cmake_build_path, ctest_path_error_ok=True):
        xml_files = glob.glob(os.path.join(path, '*.xml'))
        for xml_file in xml_files:
            os.remove(xml_file)


def rename_test_results(cmake_build_path, prefix, iteration, total):
    """
    Rename the test result files with a prefix to prevent files being overwritten by subsequent test runs.
    :param cmake_build_path: Path to the CMake build directory.
    :param prefix: Test result file prefix.
    :param iteration: Test run number.
    :param total: Total number of test runs.
    """
    for path in _get_all_test_results_paths(cmake_build_path):
        xml_files = glob.glob(os.path.join(path, '*.xml'))
        for xml_file in xml_files:
            filename = os.path.basename(xml_file)
            directory = os.path.dirname(xml_file)

            if not filename.startswith(f'{prefix}-'):
                new_name = os.path.join(directory, f'{prefix}-{iteration}-{total}-{filename}')
                os.rename(xml_file, new_name)


def collect_test_results(cmake_build_path, prefix):
    """
    Combines and aggregates test results for each test harness.
    :param cmake_build_path: Path to the CMake build directory.
    :param prefix: Test result file prefix.
    """
    class AttributeInfo:
        def __init__(self, name, func):
            self.name = name
            self.func = func

    # Attributes that will be aggregated for JUnit-like reports (GTest and Pytest)
    attributes_to_aggregate = [AttributeInfo('tests', int),
                               AttributeInfo('failures', int),
                               AttributeInfo('disabled', int),
                               AttributeInfo('errors', int),
                               AttributeInfo('time', float)]

    results_to_process = [
        # CTest results don't need aggregation, just merging.
        [_build_ctest_test_results_path(cmake_build_path), 'Site', 'Testing', []],
        # GTest and Pytest results need aggregation and merging.
        [_build_gtest_test_results_path(cmake_build_path), 'testsuites', 'testsuite', attributes_to_aggregate],
        [_build_pytest_test_results_path(cmake_build_path), 'testsuites', 'testsuite', attributes_to_aggregate]
    ]

    for result in results_to_process:
        _merge_xml_results(result[0], prefix, 'Merged.xml', result[1], result[2], result[3])


def summarize_test_results(cmake_build_path, total):
    """
    Writes a summary of the test results.
    :param cmake_build_path: Path to the CMake build directory.
    :param total: Total number of times the tests were executed.
    :return: A list of tests failed with their failure rate.
    """
    failed_tests = {}

    ctest_results_file = os.path.join(_build_ctest_test_results_path(cmake_build_path), 'Merged.xml')
    base_tree = xet.parse(ctest_results_file)
    base_tree_root = base_tree.getroot()
    testing_nodes = base_tree_root.findall('Testing')

    for testing_node in testing_nodes:
        test_nodes = testing_node.findall('Test')

        for test_node in test_nodes:
            if test_node.get('Status') == 'failed':
                name_element = test_node.find('Name')
                name = name_element.text
                failed_tests[name] = failed_tests.get(name, 0) + 1

    report = []
    for test, count in failed_tests.items():
        percent = count/total
        report.append(f'{test} failed {count}/{total} times for a failure rate of ~{percent:.2%}')
            
    return report