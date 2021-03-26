"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Utility class to resolve Lumberyard directory paths & file mappings.
"""

import os
import warnings
from abc import ABCMeta, abstractmethod

from ly_test_tools.lumberyard.asset_processor import ASSET_PROCESSOR_PLATFORM_MAP

def _find_engine_root(initial_path):
    # type: (str) -> tuple
    """
    Attempts to find the root engine directory to set the values for engine_root and dev_path.
    Assumes it exists at or above the provided "initial_path", and not in a separate directory tree
    ex. for the directory "C:\\root_dir\\dev\\":
        "C:\\root_dir\\" is the engine_root
        "C:\\root_dir\\dev\\" is the dev_path
    :param initial_path: The initial directory to search for root from
    :return: a tuple of 2 strings representing the engine_root and dev_path
    """
    root_file = "engineroot.txt"
    current_dir = initial_path

    # Look upward a handful of levels, before assuming a missing root directory
    # Assumes folder structure similar to: engine_root/dev/Tools/.../ly_test_tools/builtin
    for _ in range(15):
        if os.path.exists(os.path.join(current_dir, root_file)):
            # The parent of the directory containing the engineroot.txt is the root directory
            engine_root = os.path.abspath(os.path.join(current_dir, os.path.pardir))
            dev_path = current_dir
            return engine_root, dev_path
        # Using an explicit else to avoid aberrant behavior from following filesystem links
        else:
            current_dir = os.path.abspath(os.path.join(current_dir, os.path.pardir))

    raise OSError(f"Unable to find engine root directory. Verify root file '{root_file}' exists")


class AbstractResourceLocator(object):
    __metaclass__ = ABCMeta

    def __init__(self, build_directory, project):
        # type: (str, str) -> AbstractResourceLocator
        """
        :param build_directory: The path to the build directory (i.e. <engine_root>/dev/windows_vs2017/bin/profile)
        :param project: The game project (i.e. AutomatedTesting or StarterGame)
        """
        engine_root, dev_path = _find_engine_root(os.path.abspath(__file__))
        self._build_directory = build_directory

        self._engine_root = engine_root
        self._dev_path = dev_path
        self._project = project
        self._cache_override = None
        self._db_override = None
        self._ap_log_root = None

    def engine_root(self):
        """
        Return root path for this build.
        The engine_root path is one level up from dev.
        ex. lyengine
        :return: engine_root
        """
        return self._engine_root

    def dev(self):
        """
        Returns the path to the dev directory
        ex. <engine_root>\\dev
        :return: dev_path
        """
        return self._dev_path

    def third_party(self):
        """
        Return path to 3rdParty directory
        ex. <engine_root>\\3rdParty
        :return: path to 3rdParty
        """
        third_party_folder = os.path.join(self._engine_root, '3rdParty')
        third_party_txt_file = os.path.join(third_party_folder, '3rdParty.txt')

        if not os.path.isfile(third_party_txt_file):
            raise FileNotFoundError(
                f"3rdParty.txt file not found at third_party_txt_file location: '{third_party_txt_file}' - "
                f"Please specify a directory containing the 3rdParty.txt file.")

        return third_party_folder

    def build(self):
        """
        Return path to the build directory.
        ex. engine_root/dev/windows_vs2017/bin/profile)
        :return: full path to the bin folder
        """
        warnings.warn("build() is deprecated; use build_directory()", DeprecationWarning)
        return self.build_directory()

    def build_directory(self):
        """
        Return path to the build directory
        ex. engine_root/dev/windows_vs2017/bin/profile)
        :return: full path to the bin folder
        """
        return self._build_directory

    def project(self):
        """
        Return path to the project directory
        ex. engine_root/dev/SamplesProject
        :return: path to <engine_root>/dev/Project
        """
        return os.path.join(self.dev(), self._project)

    def asset_processor(self):
        """
        Return path for the AssetProcessor executable.
        ex.
        :return: path to <build_directory>/AssetProcessor
        """
        return os.path.join(self.build_directory(), 'AssetProcessor')

    def asset_processor_batch(self):
        """"
        Return path for the AssetProcessorBatch compatible with this build platform and configuration
        ex. engine_root/dev/mac/bin/profile/AssetProcessorBatch
        :return: path to AssetProcessorBatch
        """
        return os.path.join(self.build_directory(), 'AssetProcessorBatch')

    def editor(self):
        """
        Return path to the editor executable compatible with the current build.
        ex. engine_root/dev/mac/bin/profile/Editor
        :return: path to Editor
        """
        return os.path.join(self.build_directory(), "Editor")

    def cache(self):
        """
        Return path to the cache dir.
        :return: path to engine_root/dev/Cache/
        """
        return self._cache_override or os.path.join(self.dev(), "Cache")

    def asset_db(self):
        """
        Return path to the asset db for the current project
        :return: path to cache/<project>/assetdb.sqlite
        """
        return self._db_override or os.path.join(self.project_cache(), "assetdb.sqlite")

    def platform_cache_path(self, platform):
        """
        Return path to the cache for the current platform and project outside the game project folder
        :return: path to cache/<project>/<platform>
        """
        return os.path.join(self.project_cache(), platform)

    def asset_cache(self, platform):
        """
        Return path to the cache for the current platform and project inside the game project folder
        :return: path to cache/<project>/<platform>/<project>
        """
        return os.path.join(self.platform_cache_path(platform), self._project)

    def asset_catalog(self, platform):
        """
        Return path to the asset catalog for the current platform and project
        :return: path to cache/<project>/<platform>/<project>/assetcatalog.xml
        """
        return os.path.join(self.asset_cache(platform), "assetcatalog.xml")

    def set_ap_log_root(self, log_root):
        """
        Set or clear an override for AP's log root.  Logs folder will appear here.
        :return: path to 'logs' dir in <bin dir> folder
        """
        self._ap_log_root = log_root

    def ap_log_root(self):
        """
        Return path to AssetProcessorBatch's log directory using the project bin dir
        :return: path where the "logs" folder will be found
        """
        return self._ap_log_root or self.build_directory()

    def ap_log_dir(self):
        """
        Return path to AssetProcessorBatch's log directory using the project bin dir
        :return: path to 'logs' dir in <bin dir> folder
        """
        return os.path.join(self.ap_log_root(), 'logs')

    def ap_job_logs(self):
        """
        Return path to AssetProcessorBatch's log directory using the project bin dir
        :return: path to 'logs' dir in <bin dir> folder
        """
        return os.path.join(self.ap_log_dir(), 'JobLogs')

    def ap_batch_log(self):
        """
        Return path to AssetProcessorBatch's log file using the project bin dir
        :return: path to 'AP_Batch.log' file in <ap_log_dir> folder
        """
        return os.path.join(self.ap_log_dir(), 'AP_Batch.log')

    def ap_gui_log(self):
        """
        Return path to AssetProcessor's log file using the project bin dir
        :return: path to 'AP_Gui.log' file in <ap_log_dir> folder
        """
        return os.path.join(self.ap_log_dir(), 'AP_Gui.log')

    def project_cache(self):
        """
        Return path to the current project cache folder
        :return: path to engine_root/dev/Cache/<project>
        """
        return os.path.join(self.cache(), self._project)

    def get_shader_compiler_path(self):
        """
        Return path to shader compiler executable
        ex. engine_root/dev/windows_vs2019/bin/profile/CrySCompileServer
        :return: path to CrySCompileServer executable
        """
        return os.path.join(self.build_directory(), 'CrySCompileServer')

    def bootstrap_config_file(self):
        return os.path.join(self.dev(), 'bootstrap.cfg')

    def asset_processor_config_file(self):
        return os.path.join(self.dev(), 'AssetProcessorPlatformConfig.ini')

    def autoexec_file(self):
        return os.path.join(
            self.project(),
            'autoexec.cfg'
        )

    def test_results(self):
        """
        Return the path to the TestResults directory containing test artifacts.
        :return: path to TestResults dir
        """
        return os.path.join(self.dev(), "TestResults")

    def devices_file(self):
        """
        Return the path to the user's devices.ini file. This has OS specific functionality.
            Windows: %USERPROFILE%/ly_test_tools/devices.ini
            Mac: ~/ly_test_tools/devices.ini
        :return: Path to <root>/ly_test_tools/devices.ini
            a.k.a. %USERPROFILE%/ly_test_tools/devices.ini
        """
        return os.path.join(os.path.expanduser('~'),
                            'ly_test_tools',
                            'devices.ini')

    def shader_compiler_config_file(self):
        """
        Return path to the Shader Compiler config file
        ex. engine_root/dev/windows_vs2019/bin/profile/config.ini
        :return: path to the Shader Compiler config file
        """
        return os.path.join(self.build_directory(), 'config.ini')

    def shader_cache(self):
        """
        Return path to the shader cache for the current build
        ex. engine_root/dev/windows_vs2019/bin/profile/Cache
        :return: path to the shader cache for the current build
        """
        return os.path.join(self.build_directory(), 'Cache')

    #
    #   The following are OS specific paths and must be defined by an override
    #

    @abstractmethod
    def platform_config_file(self):
        """
        Return the path to the platform config file.
        :return: path to the platform config file (i.e. engine_root/dev/system_windows_pc.cfg)
        """
        raise NotImplementedError(
            "platform_config_file() is not implemented on the base AbstractResourceLocator() class. "
            "It must be defined by the inheriting class - "
            "i.e. _WindowsResourceLocator(AbstractResourceLocator).platform_config_file()")

    @abstractmethod
    def platform_cache(self):
        """
        Return path to the cache for the current operating system platform and project
        :return: path to engine_root/dev/cache/<project>/<platform>
        """
        raise NotImplementedError(
            "platform_cache() is not implemented on the base AbstractResourceLocator() class. "
            "It must be defined by the inheriting class - "
            "i.e. _WindowsResourceLocator(AbstractResourceLocator).platform_cache()")

    @abstractmethod
    def project_log(self):
        """
        Return path to the project's log dir using the builds project and operating system platform
        :return: path to 'log' dir in the platform cache dir
        """
        raise NotImplementedError(
            "project_log() is not implemented on the base AbstractResourceLocator() class. "
            "It must be defined by the inheriting class - "
            "i.e. _WindowsResourceLocator(AbstractResourceLocator).project_log()")

    @abstractmethod
    def project_screenshots(self):
        """
        Return path to the project's screen shot dir using the builds project and platform
        :return: path to 'screen shot' dir in the platform cache dir
        """
        raise NotImplementedError(
            "project_screenshots() is not implemented on the base AbstractResourceLocator() class. "
            "It must be defined by the inheriting class - "
            "i.e. _WindowsResourceLocator(AbstractResourceLocator).project_screenshots()")

    @abstractmethod
    def editor_log(self):
        """
        Return path to the project's editor log dir using the builds project and platform
        :return: path to editor.log
        """
        raise NotImplementedError(
            "editor_log() is not implemented on the base AbstractResourceLocator() class. "
            "It must be defined by the inheriting class - "
            "i.e. _WindowsResourceLocator(AbstractResourceLocator).editor_log()")
