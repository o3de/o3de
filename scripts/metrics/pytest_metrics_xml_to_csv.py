"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Scrapes metrics from Pytest xml files and creates csv formatted files.
"""
import os.path
import sys
import csv
import argparse
import xml.etree.ElementTree as xmlElementTree
import datetime

import ly_test_tools.cli.codeowners_hint as codeowners_hint
from common import logging, exception


TESTING_DIR = 'Testing'


def _get_default_csv_filename():
    # Format default file name based off of date
    now = datetime.datetime.now()
    return f"{now.year}_{now.month:02d}_{now.day:02d}_{now.hour:02d}_{now.minute:02d}_pytest.csv"


# Setup logging.
logger = logging.get_logger("test_metrics")
logging.setup_logger(logger)

# Create the csv field header
PYTEST_FIELDS_HEADER = [
    'test_name',
    'status',
    'duration_seconds',
    'sig_owner'
]
SIG_OWNER_CACHE = {}


def main():
    # Parse args
    args = parse_args()

    if not os.path.exists(args.xml_folder):
        raise exception.MetricsExn(f"Cannot find directory: {args.xml_folder}")

    # Create csv file
    full_path = os.path.join(args.output_directory, args.branch)
    if args.weeks:
        week = datetime.datetime.now().isocalendar().week
        full_path = os.path.join(full_path, f"Week{week:02d}")
    full_path = os.path.join(full_path, args.csv_file)
    if os.path.exists(full_path):
        logger.warning(f"The file {full_path} already exists. It will be overridden.")
    if not os.path.exists(os.path.split(full_path)[0]):
        # Create directory if it doesn't exist
        os.makedirs(os.path.split(full_path)[0])

    # Create csv file
    if os.path.exists(args.csv_file):
        logger.warning(f"The file {args.csv_file} already exists. It will be overriden.")
    with open(full_path, 'w', encoding='UTF8', newline='') as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=PYTEST_FIELDS_HEADER, restval='N/A')
        writer.writeheader()

        # Parse Pytest xml's and write to csv file
        for filename in os.listdir(args.xml_folder):
            if os.path.splitext(filename)[-1] == '.xml':
                parse_pytest_xmls_to_csv(os.path.join(args.xml_folder, filename), writer)


def parse_args():
    parser = argparse.ArgumentParser(
        description='This script assumes that Pytest xml files have been produced. The xml files will be parsed and '
                    'written to a csv file.')
    parser.add_argument(
        'xml_folder',
        help="Path to where the Pytest xml files live. O3DE CTest defaults this to {build_folder}/Testing/Pytest"
    )
    parser.add_argument(
        "--csv-file", action="store", default=_get_default_csv_filename(),
        help=f"The directory and file name for the csv to be saved (defaults to YYYY_MM_DD_hh_mm_pytest)."
    )
    parser.add_argument(
        "-o", "--output-directory", action="store", default="",
        help=f"The directory where the csv to be saved. Prepends the --csv-file arg."
    )
    parser.add_argument(
        "-b", "--branch", action="store", default="",
        help="The branch the metrics were generated on. Used for directory saving."
    )
    parser.add_argument(
        "-w", "--weeks", action="store_true",
        help="Boolean whether to include the week number in the created csv path."
    )
    args = parser.parse_args()
    return args


def _determine_test_result(test_case_node):
    # type (xml.etree.ElementTree.Element) -> str
    """
    Inspects the test case node and determines the test result based on the presence of known element
    names such as 'error', 'failure' and 'skipped'. If the elements are not found, the test case is assumed
    to have passed. This is how the JUnit XML schema is generated for failed tests.

    :param test_case_node: The element node for the test case.
    :return: The test result
    """
    # Mapping from JUnit elements to test result to keep it consistent with CTest result reporting.
    failure_elements = [('error', 'failed'), ('failure', 'failed'), ('skipped', 'skipped')]

    for element in failure_elements:
        if test_case_node.find(element[0]) is not None:
            return element[1]
    return 'passed'


def parse_pytest_xmls_to_csv(full_xml_path, writer):
    # type (str, DictWriter) -> None
    """
    Parses the PyTest xml file to write to a csv file. The structure of the PyTest xml is assumed to be as followed:
    <testcase classname="ExampleClass" file="SamplePath\test_Sample.py" line="43" name="test_Sample" time="113.225">
        <properties>
            <property name="test_case_id" value="IDVal"/>
        </properties>
        <system-out></system-out>
    </testcase>
    :param full_xml_path: The full path to the xml file
    :param writer: The DictWriter object to write to the csv file.
    :return: None
    """
    xml_root = xmlElementTree.parse(full_xml_path).getroot()
    test_data_dict = {}
    # Each PyTest test module will have a Test entry
    for test in xml_root.findall('./testsuite/testcase'):
        try:
            test_data_dict['test_name'] = test.attrib['name']
            test_data_dict['duration_seconds'] = float(test.attrib['time'])
            # using 'status' to keep it consistent with CTest xml schema
            test_data_dict['status'] = _determine_test_result(test)
            # replace slashes to match codeowners file
            test_file_path = test.attrib['file'].replace("\\", "/")
        except KeyError as exc:
            logger.exception(f"KeyError when parsing xml file: {full_xml_path}. Check xml keys for changes. Printing"
                             f"attribs:\n{test.attrib}", exc)
            continue
        if sig_owner in SIG_OWNER_CACHE:
            sig_owner = SIG_OWNER_CACHE[test_file_path]
        else:
            # Index 1 is the sig owner
            _, sig_owner, _ = codeowners_hint.get_codeowners(test_file_path)
            SIG_OWNER_CACHE[test_file_path] = sig_owner
        test_data_dict['sig_owner'] = sig_owner if sig_owner else "N/A"

        writer.writerow(test_data_dict)


if __name__ == "__main__":
    sys.exit(main())
