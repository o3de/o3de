"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

OS and devices are detected and set as constants when ly_test_tools.__init__() completes.
"""
import logging
import sys
import platform

logger = logging.getLogger(__name__)

# Supported platforms
ALL_PLATFORM_OPTIONS = ['android', 'ios', 'linux', 'mac', 'windows']
ALL_LAUNCHER_OPTIONS = [
    'android', 'base', 'linux', 'mac', 'windows', 'windows_editor', 'windows_dedicated', 'windows_atom_tools']
ANDROID = False
IOS = False  # Not implemented - see SPEC-2505
LINUX = sys.platform.startswith('linux')
MAC = sys.platform.startswith('darwin')
WINDOWS = sys.platform.startswith('win')

# Detect available platforms
HOST_OS_PLATFORM = 'unknown'
HOST_OS_EDITOR = 'unknown'
HOST_OS_DEDICATED_SERVER = 'unknown'
HOST_OS_ATOM_TOOLS = 'unknown'
LAUNCHERS = {}
for launcher_option in ALL_LAUNCHER_OPTIONS:
    LAUNCHERS[launcher_option] = None
from ly_test_tools.launchers.platforms.base import Launcher
LAUNCHERS['base'] = Launcher
if WINDOWS:
    HOST_OS_PLATFORM = 'windows'
    HOST_OS_EDITOR = 'windows_editor'
    HOST_OS_DEDICATED_SERVER = 'windows_dedicated'
    HOST_OS_ATOM_TOOLS = 'windows_atom_tools'
    import ly_test_tools.mobile.android
    from ly_test_tools.launchers import (
        AndroidLauncher, WinLauncher, DedicatedWinLauncher, WinEditor, WinAtomToolsLauncher)
    ANDROID = ly_test_tools.mobile.android.can_run_android()
    LAUNCHERS['windows'] = WinLauncher
    LAUNCHERS['windows_editor'] = WinEditor
    LAUNCHERS['windows_dedicated'] = DedicatedWinLauncher
    LAUNCHERS['windows_atom_tools'] = WinAtomToolsLauncher
    LAUNCHERS['android'] = AndroidLauncher
elif MAC:
    ARM64 = platform.machine() == 'arm64'
    HOST_OS_PLATFORM = 'mac'
    HOST_OS_EDITOR = NotImplementedError('LyTestTools does not yet support Mac editor')
    HOST_OS_DEDICATED_SERVER = NotImplementedError('LyTestTools does not yet support Mac dedicated server')
    HOST_OS_ATOM_TOOLS = NotImplementedError('LyTestTools does not yet support Mac Atom Tools executables')
    from ly_test_tools.launchers import MacLauncher
    LAUNCHERS['mac'] = MacLauncher
elif LINUX:
    ARM64 = platform.machine() == 'aarch64'
    HOST_OS_PLATFORM = 'linux'
    HOST_OS_EDITOR = 'linux_editor'
    HOST_OS_DEDICATED_SERVER = 'linux_dedicated'
    HOST_OS_ATOM_TOOLS = 'linux_atom_tools'
    from ly_test_tools.launchers.platforms.linux.launcher import (
        LinuxLauncher, LinuxEditor, DedicatedLinuxLauncher, LinuxAtomToolsLauncher)
    LAUNCHERS['linux'] = LinuxLauncher
    LAUNCHERS['linux_editor'] = LinuxEditor
    LAUNCHERS['linux_dedicated'] = DedicatedLinuxLauncher
    LAUNCHERS['linux_atom_tools'] = LinuxAtomToolsLauncher
else:
    ARM64 = False
    logger.warning(f'WARNING: LyTestTools only supports Windows, Mac, and Linux. '
                   f'Unexpectedly detected HOST_OS_PLATFORM: "{HOST_OS_PLATFORM}".')
