"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Tests the functionality of the PlatformSetting class
"""

import pytest

import ly_test_tools.builtin.helpers as helpers
from Tests.ly_shared.PlatformSetting import PlatformSetting

all_platforms = helpers.all_host_platforms_params()
automatic_platform_skipping = helpers.automatic_platform_skipping
targetProjects = ["Helios"]


@pytest.mark.usefixtures("automatic_platform_skipping")
@pytest.mark.parametrize("platform", all_platforms)
@pytest.mark.parametrize("configuration", ["profile"])
@pytest.mark.parametrize("spec", ["all"])
@pytest.mark.parametrize("project", targetProjects)
class TestsPlatformSetting(object):
    """
    Tests for the PlatformSetting class
    """

    def test_PlatformSetting(self, workspace):

        key = "Software"
        subkey = "TemporarySystemSetting"

        # Create setting reference
        setting = PlatformSetting.get_system_setting(workspace, subkey, key)

        # Test storing integer
        value = 74
        setting.set_value(value)

        # Test creation of subkey
        assert setting.entry_exists(), f"Failed creating key:subkey, {key}:{subkey}"

        # Test data retrieval (without type)
        retrieved = setting.get_value()
        # fmt:off
        assert retrieved == value, \
            f"Unexpected value retrieved from system settings. Expected: {value}, Actual: {retrieved}"
        # fmt:on

        # Test data retrieval (with type)
        retrieved = setting.get_value(get_type=True)
        assert type(retrieved) == tuple, "Getting value with type DID NOT return a tuple"
        assert len(retrieved) == 2, f"Getting value with type returned a tuple of size {len(retrieved)}: expected 2"
        assert retrieved[1] == PlatformSetting.DATA_TYPE.INT, "Value stored was int, but type retrieved was NOT int"
        assert type(retrieved[0]) == int, "Value stored was int, but value retrieved was NOT int"

        # fmt:off
        assert retrieved[0] == value, \
            f"Unexpected value retrieved from system settings. Expected: {value}, Actual: {retrieved[0]}"
        # fmt:on

        # Test storing string
        value = "Some Text"
        setting.set_value(value)
        retrieved = setting.get_value(get_type=True)
        assert (
            retrieved[1] == PlatformSetting.DATA_TYPE.STR
        ), "Value stored was string, but type retrieved was NOT string"
        assert type(retrieved[0]) == str, "Value stored was string, but value retrieved was NOT string"
        assert value == retrieved[0], f"Value retrieved not expected. Expected: {value}, Actual: {retrieved[0]}"

        # Test storing list of strings
        value = ["Some", "List", "Of", "Text"]
        setting.set_value(value)
        retrieved = setting.get_value(get_type=True)
        assert (
            retrieved[1] == PlatformSetting.DATA_TYPE.STR_LIST
        ), "Value stored was string list, but type retrieved was NOT string list"
        assert type(retrieved[0]) == list, "Value stored was string, but value retrieved was NOT string"
        # fmt:off
        assert sorted(value) == sorted(retrieved[0]), f"Value retrieved not expected. " \
                                                      f"Expected: {value}, Actual: {retrieved[0]}"
        # fmt:on

        setting.delete_entry()
        assert not setting.entry_exists(), f"Failed to delete key:subkey, {key}:{subkey}"
