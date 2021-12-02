# coding:utf-8
# !/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# File Description:
# Contains helper functions to assist when processing files using the DCCsi. This file will be consistently added to
# as new helper functions are needed
# -------------------------------------------------------------------------


import logging as _logging

try:
    import pathlib
except:
    import pathlib2 as pathlib

from pathlib import Path

from box import Box
import os


module_name = 'legacy_asset_converter.main.utilities'
_LOGGER = _logging.getLogger(module_name)


def convert_box_dict_to_standard(target_dict):
    """
    This converts all aspects of a dictionary to be compatible for Python 2.x and 3.x. Converting
    dictionaries back and forth is needed in the event that they need to be processed with DCC apps (
    specifically Maya) that run on earlier versions of Python. The Pathlib and Box modules will cause
    errors otherwise.

    :param target_dict: This specifies which dictionary to convert
    :return:
    """
    _LOGGER.info('Convert Box database with Pathlib to standard dictionary listings for Maya 2.7')
    converted_dictionary = {}
    for key, values in target_dict.items():
        updated_dictionary = {}
        for k, v in values.items():
            if str(type(v)) == "<class 'box.box.Box'>":
                updated_dictionary[k] = v.to_dict()
            elif str(type(v)) == "<class 'box.box_list.BoxList'>":
                updated_dictionary[k] = [os.path.normpath(x) for x in v.to_list()]
            elif isinstance(v, pathlib.WindowsPath):
                updated_dictionary[k] = os.path.abspath(v)
            else:
                updated_dictionary[k] = v
        converted_dictionary[key] = clear_pathlib_instances(updated_dictionary)
    return converted_dictionary


def convert_standard_dict_to_box(target_dict):
    """
    This converts all aspects of a dictionary to the preferred formatting for Python 3.x to run it.
    To allow more clarity when handling dictionaries extensive use of Box and Pathlib is using in
    the handling of data. Unfortunately this is not compatible with Python 2.7 so in some situations
    the formatting needs to be removed. This reinstates formatting to include Box and Pathlib.

    :param target_dict: This specifies which dictionary to convert
    :return:
    """
    _LOGGER.info('Convert Standard dictionary database back to Box Dictionary with Pathlib')
    converted_dictionary = {}
    for key, values in target_dict.items():
        updated_dictionary = Box({})
        for k, v in values.items():
            if str(type(v)) == "<class 'dict'>":
                updated_dictionary[k] = Box(v)
            elif str(type(v)) == "<class 'list'>":
                updated_dictionary[k] = BoxList(v)
            elif os.path.isfile(v):
                updated_dictionary[k] = Path(v)
            else:
                updated_dictionary[k] = v
        converted_dictionary[key] = add_pathlib_instances(updated_dictionary)
    return Box(converted_dictionary)


def add_pathlib_instances(target_dictionary):
    """
    Converts string paths to pathlib instances.
    :param target_dictionary:
    :return:
    """
    pathlib_added = {}
    for k, v in target_dictionary.items():
        if isinstance(v, dict):
            nested = add_pathlib_instances(v)
            if len(nested.keys()):
                pathlib_added[k] = nested
        elif isinstance(v, Box):
            nested = add_pathlib_instances(v)
            if len(nested.keys()):
                pathlib_added[k] = nested
        elif isinstance(v, str):
            if os.path.isfile(v):
                pathlib_added[k] = Path(v)
            elif os.path.isdir(v):
                pathlib_added[k] = Path(v)
            else:
                pathlib_added[k] = v
        else:
            pathlib_added[k] = v
    return pathlib_added


def clear_pathlib_instances(target_dictionary):
    """
    Removes all pathlib instances and replaces with string paths
    :param target_dictionary:
    :return:
    """
    clean = {}
    for k, v in target_dictionary.items():
        if isinstance(v, dict):
            nested = clear_pathlib_instances(v)
            if len(nested.keys()):
                clean[k] = nested
        elif isinstance(v, pathlib.WindowsPath):
            clean[k] = os.path.abspath(v)
        else:
            clean[k] = v
    return clean

