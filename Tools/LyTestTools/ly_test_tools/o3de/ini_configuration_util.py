"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Small library of functions to manipulate ini configuration files.
Please see INI Specification for in depth explanations on parameter terms.

"""

import logging

from configparser import ConfigParser


logger = logging.getLogger(__name__)


def check_section_exists(file_location, section):
    """
    Searches an INI Configuration file for the existance of a section

    :param file_location: The file to get a key value from
    :param section: The section to find the key value
    :return: The boolean value of whether or not the section exists
    """
    config = ConfigParser()
    config.read(file_location)

    return config.has_section(section)


def check_key_exists(file_location, section, key):
    """
    Searches an INI Configuration file for the existance of a section & key

    :param file_location: The file to get a key value from
    :param section: The section to find the key value
    :param key: The key that can contain a value to retrieve
    :return: The boolean value of whether or not the key exists
    """
    config = ConfigParser()
    config.read(file_location)

    return config.has_option(section, key)


def get_string_value(file_location, section, key):
    """
    Searches an INI Configuration file for a section & key and returns it as a string.

    :param file_location: The file to get a key value from
    :param section: The section to find the key value
    :param key: The key that can contain a value to retrieve
    :return: The string value retained in the key
    """

    if check_key_exists(file_location, section, key):
        config = ConfigParser()
        config.read(file_location)

        return config.get(section, key)
    else:
        assert False, "Was unable to find the key. Please verify the existance of the section '{0}' and key '{1}"\
            .format(section, key)


def get_boolean_value(file_location, section, key):
    """
    Searches an INI Configuration file for a section & key and returns it as a string.

    :param file_location: The file to get a key value from
    :param section: The section to find the key value
    :param key: The key that can contain a value to retrieve
    :return: The boolean value retained in the key
    """

    if check_key_exists(file_location, section, key):
        config = ConfigParser()
        config.read(file_location)

        return config.getboolean(section, key)
    else:
        assert False, "Was unable to find the key. Please verify the existance of the section '{0}' and key '{1}" \
            .format(section, key)


def get_integral_value(file_location, section, key):
    """
    Searches an INI Configuration file for a section & key and returns it as a string.

    :param file_location: The file to get a key value from
    :param section: The section to find the key value
    :param key: The key that can contain a value to retrieve
    :return: The boolean value retained in the key
    """

    if check_key_exists(file_location, section, key):
        config = ConfigParser()
        config.read(file_location)

        return config.getint(section, key)
    else:
        assert False, "Was unable to find the key. Please verify the existance of the section '{0}' and key '{1}" \
            .format(section, key)


def get_float_value(file_location, section, key):
    """
    Searches an INI Configuration file for a section & key and returns it as a string.

    :param file_location: The file to get a key value from
    :param section: The section to find the key value
    :param key: The key that can contain a value to retrieve
    :return: The boolean value retained in the key
    """

    if check_key_exists(file_location, section, key):
        config = ConfigParser()
        config.read(file_location)

        return config.getfloat(section, key)
    else:
        assert False, "Was unable to find the key. Please verify the existance of the section '{0}' and key '{1}" \
            .format(section, key)


def check_string_value(file_location, section, key, expected):
    """
    Compare the string contained in a key against expected.

    :param file_location: The file to get a key value from
    :param section: The section to find the key value
    :param key: The key that can contain a value to retrieve
    :param expected: They expected value to compare to
    :assert: If the values do not match
    :return: None
    """

    if check_key_exists(file_location, section, key):
        config = ConfigParser()
        config.read(file_location)

        actual = get_string_value(file_location, section, key)

        assert actual == expected, "The value of the '{0}' key in the '{1}' section was '{2}'" \
                                   "and did not match the expected value of {3}".format(key, section, actual, expected)
    else:
        assert False, "Was unable to find the key to do a comparison. " \
                      "Please verify the existance of the section '{0}' and key '{1}" \
            .format(section, key)


def check_boolean_value(file_location, section, key, expected):
    """
    Compare the boolean contained in a key against expected.

    :param file_location: The file to get a key value from
    :param section: The section to find the key value
    :param key: The key that can contain a value to retrieve
    :param expected: They expected value to compare to
    :assert: If the values do not match
    :return: None
    """

    if check_key_exists(file_location, section, key):
        config = ConfigParser()
        config.read(file_location)

        actual = get_boolean_value(file_location, section, key)

        assert actual == expected, "The value of the '{0}' key in the '{1}' section was '{2}'" \
                                   "and did not match the expected value of {3}".format(key, section, actual, expected)
    else:
        assert False, "Was unable to find the key to do a comparison. " \
                      "Please verify the existance of the section '{0}' and key '{1}" \
            .format(section, key)


def check_integral_value(file_location, section, key, expected):
    """
    Compare the integral contained in a key against expected.

    :param file_location: The file to get a key value from
    :param section: The section to find the key value
    :param key: The key that can contain a value to retrieve
    :param expected: They expected value to compare to
    :assert: If the values do not match
    :return: None
    """

    if check_key_exists(file_location, section, key):
        config = ConfigParser()
        config.read(file_location)

        actual = get_integral_value(file_location, section, key)

        assert actual == expected, "The value of the '{0}' key in the '{1}' section was '{2}'" \
                                   "and did not match the expected value of {3}".format(key, section, actual, expected)
    else:
        assert False, "Was unable to find the key to do a comparison. " \
                      "Please verify the existance of the section '{0}' and key '{1}" \
            .format(section, key)


def check_float_value(file_location, section, key, expected):
    """
    Compare the float contained in a key against expected.

    :param file_location: The file to get a key value from
    :param section: The section to find the key value
    :param key: The key that can contain a value to retrieve
    :param expected: They expected value to compare to
    :assert: If the values do not match
    :return: None
    """

    if check_key_exists(file_location, section, key):
        config = ConfigParser()
        config.read(file_location)

        actual = get_float_value(file_location, section, key)

        assert actual == expected, "The value of the '{0}' key in the '{1}' section was '{2}'" \
                                   "and did not match the expected value of {3}".format(key, section, actual, expected)
    else:
        assert False, "Was unable to find the key to do a comparison. " \
                      "Please verify the existance of the section '{0}' and key '{1}" \
            .format(section, key)


def add_section(file_location, section):
    """
    Add section to the configuration file provided

    :param file_location: The file to get a key value from
    :param section: The section to add
    :assert: If the the section does not exist in the file after attempting to add it
    :return: None
    """

    config = ConfigParser()
    config.read(file_location)

    config.add_section(section)

    with open(file_location, 'w') as configfile:
        config.write(configfile)

    assert check_section_exists(file_location, section), \
        "Section '{0}' failed to add to the configuration file '{1}'".format(section, file_location)


def add_key(file_location, section, key, value=''):
    """
    Add key to the section in the configuration file provided

    :param file_location: The file to get a key value from
    :param section: The section to add the key value
    :param key: The section to add the key value
    :param value: The value to set the key to
    :assert: If the the key does not exist in the file after attempting to add it
    :return: None
    """

    logger.debug("Section exists: {0}".format(check_section_exists(file_location, section)))
    assert check_section_exists(file_location, section), \
        "Cannot add a key to section '{0}' since it does not exist in configuration file '{1}'".format(section,
                                                                                                       file_location)

    config = ConfigParser()
    config.read(file_location)

    config.set(section, key, value)

    with open(file_location, 'w') as configfile:
        config.write(configfile)

    assert check_key_exists(file_location, section, key), "Key '{0}' failed to add to the configuration file '{1}'"\
        .format(key, file_location)


def delete_section(file_location, section):
    """
    Delete section from the configuration file provided

    :param file_location: The file to modify
    :param section: The section to delete
    :assert: If the the section does exists in the file after attempting to add it
    :return: None
    """

    config = ConfigParser()
    config.read(file_location)

    config.remove_section(section)

    with open(file_location, 'w') as configfile:
        config.write(configfile)

    assert not check_section_exists(file_location, section), \
        "Section '{0}' still exists in the configuration file '{1}'".format(section, file_location)


def delete_key(file_location, section, key):
    """
    Delete key from the section in the configuration file provided

    :param file_location: The file to modify
    :param section: The section to delete the key value
    :param key: The section to delete the key value
    :assert: If the the section exists in the file after attempting to delete it
    :return: None
    """

    logger.debug("Section exists: {0}".format(check_section_exists(file_location, section)))
    assert check_section_exists(file_location, section), \
        "Cannot add a key to section '{0}' since it does not exist in configuration file '{1}'".format(section, file)

    config = ConfigParser()
    config.read(file_location)

    config.remove_option(section, key)

    with open(file_location, 'w') as configfile:
        config.write(configfile)

    assert not check_key_exists(file_location, section, key), "Key '{0}' still exists in the configuration file '{1}'"\
        .format(key, file_location)
