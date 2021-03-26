"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import filecmp
import os
import pytest


pytestmark = pytest.mark.SUITE_smoke


class TestLySettings(object):

    @pytest.mark.parametrize("project", ["AutomatedTesting"])
    @pytest.mark.parametrize("wait_for_connect", [1234, 4567])
    def test_BootstrapSettings_BackupModifyRestore_SettingsMatch(self, workspace, wait_for_connect):
        backup_path = workspace.tmp_path

        # create backup
        workspace.settings.backup_bootstrap_settings(backup_path)

        # verify files match
        bootstrap_settings = workspace.paths.bootstrap_config_file()
        bootstrap_settings_backup = os.path.join(backup_path, '{}.bak'.format(os.path.basename(bootstrap_settings)))
        assert os.path.exists(bootstrap_settings), "Bootstrap settings file does not exist"
        assert os.path.exists(bootstrap_settings_backup), "Bootstrap settings backup does not exist"
        assert filecmp.cmp(bootstrap_settings, bootstrap_settings_backup), "Bootstrap settings and backup do not match"

        # modify settings
        workspace.settings.modify_bootstrap_setting('remote_filesystem', 0)
        workspace.settings.modify_bootstrap_setting('wait_for_connect', wait_for_connect)
        workspace.settings.modify_bootstrap_setting('remote_ip', '0.36.27.18')
        workspace.settings.modify_bootstrap_setting('additional_setting', 'value1')

        # verify files are different
        assert not filecmp.cmp(bootstrap_settings, bootstrap_settings_backup), "Modified bootstrap settings and backup" \
                                                                               " are equal, they should be different"

        # restore backup
        workspace.settings.restore_bootstrap_settings(backup_path)

        # verify files match
        assert filecmp.cmp(bootstrap_settings, bootstrap_settings_backup), "Restored bootstrap settings and backup " \
                                                                           "are different, they should be equal"
