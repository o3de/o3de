"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import json
import xml.etree.ElementTree as xml
import itertools
from typing import List
from dataclasses import dataclass
import xml.parsers.expat as expat
from automatedtesting_shared import asset_database_utils as asset_db_utils


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
    """
    A function that reads the contents of an xml file to create a dictionary.
    Returns the dict data as an instance of the BlackboxDebugTest dataclass.
    :param xml_file: An xml file that provides the data required to compare test results vs expected values.
    :return: Dictionary.
    """
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
