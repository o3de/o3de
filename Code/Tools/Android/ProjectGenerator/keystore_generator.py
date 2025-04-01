#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

import os

from config_data import ConfigData
from threaded_lambda import ThreadedLambda
from subprocess_runner import SubprocessRunner

class KeystoreGenerator(ThreadedLambda):
    """
    This class knows how to use the keytool JAVA command to create
    a new keystore. It also uses o3de.bat/o3de.sh to store configuration
    data for the current project.
    """

    def __init__(self, config: ConfigData):
        def _job_func():
            self._run_commands()
        super().__init__("Keystore Generator", _job_func)
        ks = config.keystore_settings
        o3de_cmd = config.get_o3de_cmd()
        self._keystoreFileConfigureCmd = SubprocessRunner(
            [
                o3de_cmd,
                "android-configure",
                "--set-value",
                f"signconfig.store.file={ks.keystore_file}",
                "--project",
                config.project_path,
            ],
            timeOutSeconds=10,
        )

        self._keystoreKeyAliasConfigureCmd = SubprocessRunner(
            [
                o3de_cmd,
                "android-configure",
                "--set-value",
                f"signconfig.key.alias={ks.key_alias}",
                "--project",
                config.project_path,
            ],
            timeOutSeconds=10,
        )

        self._keystoreStorePasswordConfigureCmd = SubprocessRunner(
            [
                o3de_cmd,
                "android-configure",
                "--set-value",
                f"signconfig.store.password={ks.keystore_password}",
                "--project",
                config.project_path,
            ],
            timeOutSeconds=10,
        )

        self._keystoreKeyPasswordConfigureCmd = SubprocessRunner(
            [
                o3de_cmd,
                "android-configure",
                "--set-value",
                f"signconfig.key.password={ks.key_password}",
                "--project",
                config.project_path,
            ],
            timeOutSeconds=10,
        )

        self._keystoreCreateCmd = SubprocessRunner(
            [
                "keytool",
                "-genkey",
                "-keystore",
                ks.keystore_file,
                "-storepass",
                ks.keystore_password,
                "-alias",
                ks.key_alias,
                "-keypass",
                ks.key_password,
                "-keyalg",
                "RSA",
                "-keysize",
                ks.key_size,
                "-validity",
                ks.validity_days,
                "-dname",
                ks.get_distinguished_name(),
            ],
            timeOutSeconds=10,
            )

    def _run_commands(self):
        """
        This is a "protected" function. It is invoked when super().start() is called.
        """
        if self._is_cancelled:
            self._is_finished = True
            self._is_success = False
            return
        commands = [
            self._keystoreFileConfigureCmd,
            self._keystoreKeyAliasConfigureCmd,
            self._keystoreStorePasswordConfigureCmd,
            self._keystoreKeyPasswordConfigureCmd,
            self._keystoreCreateCmd,
        ]
        for command in commands:
            if self._is_cancelled:
                self._is_finished = True
                self._is_success = False
                return
            if not command.run():
                self._record_command_error(command)
                return
            else:
                self._record_command_results(command)
        self._is_finished = True
        self._is_success = True
        self._report_msg += f"Next Steps:\nYou can now generate the android project by pushing the `Generate Project` button.\n"


    def _record_command_error(self, command: SubprocessRunner):
        self._is_finished = True
        self._is_success = False
        self._record_command_results(command)


    def _record_command_results(self, command: SubprocessRunner):
        self._report_msg += f"Command:\n{command.get_command_str()}\nCompleted with status code {command.get_error_code()}.\n"
        self._report_msg += command.get_stdall()

# class KeystoreGenerator END
######################################################
