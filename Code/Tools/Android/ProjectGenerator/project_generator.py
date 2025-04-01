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

class ProjectGenerator(ThreadedLambda):
    """
    This class knows how to create an Android project.
    It first runs the configuration commands supported by o3de.bat/o3de.sh,
    validates the configuration and generates the project.
    """

    def __init__(self, config: ConfigData):
        def _job_func():
            self._run_commands()
        super().__init__("Android Project Generator", _job_func)
        self._config = config
        o3de_cmd = config.get_o3de_cmd()
        self._sdk_root_configure_cmd = SubprocessRunner(
            [
                o3de_cmd,
                "android-configure",
                "--set-value",
                f"sdk.root={config.android_sdk_path}",
                "--project",
                config.project_path,
            ],
            timeOutSeconds=10,
        )
        self._api_level_configure_cmd = SubprocessRunner(
            [
                o3de_cmd,
                "android-configure",
                "--set-value",
                f"platform.sdk.api={config.android_sdk_api_level}",
                "--project",
                config.project_path,
            ],
            timeOutSeconds=10,
        )
        self._ndk_configure_cmd = SubprocessRunner(
            [
                o3de_cmd,
                "android-configure",
                "--set-value",
                f"ndk.version={config.android_ndk_version}",
                "--project",
                config.project_path,
            ],
            timeOutSeconds=10,
        )
        # agp is Android Gradle Plugin. FIXME: Make it a variable.
        self._agp_configure_cmd = SubprocessRunner(
            [
                o3de_cmd,
                "android-configure",
                "--set-value",
                "android.gradle.plugin=8.1.0",
                "--project",
                config.project_path,
            ],
            timeOutSeconds=10,
        )

        self._validate_configuration_cmd = SubprocessRunner(
            [
                o3de_cmd,
                "android-configure",
                "--validate",
                "--project",
                config.project_path,
            ],
            timeOutSeconds=60
        )

        arg_list = [
            o3de_cmd,
            "android-generate",
            "-p",
            config.get_project_name(),
            "-B",
            config.get_android_build_dir(),
            "--asset-mode",
            config.asset_mode,
            ]
        if config.is_meta_quest_project:
            arg_list.extend(["--oculus-project"])
        if config.extra_cmake_args:
            # It is imperative to wrap the extra cmake arguments in double quotes "-DFOO=ON -DBAR=OFF"
            # because the cmake arguments/variables are separated by spaces.
            arg_list.extend(["--extra-cmake-args", f'"{config.extra_cmake_args}"'])
        self._generate_project_cmd = SubprocessRunner(
            arg_list,
            timeOutSeconds=60,
            # The "android-generate" subcommand requires to be run from the project root directory,
            # otherwise it won't find the settings we previously defined with the multiple calls
            # to "android-configure" subcommand.
            cwd=config.project_path
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
            self._sdk_root_configure_cmd,
            self._api_level_configure_cmd,
            self._ndk_configure_cmd,
            self._agp_configure_cmd,
            self._validate_configuration_cmd,
            self._generate_project_cmd,
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
        self._report_msg += f"Next Steps:\nYou can compile, deploy and execute the android application with Android Studio by opening the project:\n{self._config.get_android_build_dir()}.\n"
        self._report_msg += "Alternatively, you can compile the project from the command line with the following commands:\n"
        self._report_msg += f"$ cd {self._config.get_android_build_dir()}\n$ .\\gradlew assembleProfile\nOr if interested in compiling for debugging:\n$ .\\gradlew assembleDebug\n"


    def _record_command_error(self, command: SubprocessRunner):
        self._is_finished = True
        self._is_success = False
        self._record_command_results(command)


    def _record_command_results(self, command: SubprocessRunner):
        self._report_msg += f"Command:\n{command.get_command_str()}\nCompleted with status code {command.get_error_code()}.\n"
        self._report_msg += command.get_stdall()

# class ProjectGenerator END
######################################################
