"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

OS and devices are detected and set as constants when ly_test_tools.__init__() completes.
"""
import logging
import sys

logger = logging.getLogger(__name__)

# Supported platforms.
ALL_PLATFORM_OPTIONS = ['android', 'ios', 'linux', 'mac', 'windows']
ALL_LAUNCHER_OPTIONS = ['android', 'base', 'mac', 'windows', 'windows_editor', 'windows_dedicated', 'windows_generic']
ANDROID = False
IOS = False  # Not implemented - see SPEC-2505
LINUX = sys.platform.startswith('linux')  # Not implemented - see SPEC-2501
MAC = sys.platform.startswith('darwin')
WINDOWS = sys.platform.startswith('win')

# Detect platforms.
HOST_OS_PLATFORM = 'unknown'
HOST_OS_EDITOR = 'unknown'
HOST_OS_DEDICATED_SERVER = 'unknown'
HOST_OS_GENERIC_EXECUTABLE = 'unknown'
LAUNCHERS = {}
for launcher_option in ALL_LAUNCHER_OPTIONS:
    LAUNCHERS[launcher_option] = None
from ly_test_tools.launchers.platforms.base import Launcher
LAUNCHERS['base'] = Launcher
if WINDOWS:
    HOST_OS_PLATFORM = 'windows'
    HOST_OS_EDITOR = 'windows_editor'
    HOST_OS_DEDICATED_SERVER = 'windows_dedicated'
    HOST_OS_GENERIC_EXECUTABLE = 'windows_generic'
    import ly_test_tools.mobile.android
    from ly_test_tools.launchers import (
        AndroidLauncher, WinLauncher, DedicatedWinLauncher, WinEditor, WinGenericLauncher)
    ANDROID = ly_test_tools.mobile.android.can_run_android()
    LAUNCHERS['windows'] = WinLauncher
    LAUNCHERS['windows_editor'] = WinEditor
    LAUNCHERS['windows_dedicated'] = DedicatedWinLauncher
    LAUNCHERS['windows_generic'] = WinGenericLauncher
    LAUNCHERS['android'] = AndroidLauncher
elif MAC:
    HOST_OS_PLATFORM = 'mac'
    HOST_OS_EDITOR = NotImplementedError('LyTestTools does not yet support Mac editor')
    HOST_OS_DEDICATED_SERVER = NotImplementedError('LyTestTools does not yet support Mac dedicated server')
    from ly_test_tools.launchers import MacLauncher
    LAUNCHERS['mac'] = MacLauncher
elif LINUX:
    logger.warning(f'Linux operating system is currently not supported, LyTestTools only supports Windows and Mac.')
    HOST_OS_PLATFORM = 'linux'
    HOST_OS_EDITOR = NotImplementedError('LyTestTools does not yet support Linux editor')
    HOST_OS_DEDICATED_SERVER = NotImplementedError('LyTestTools does not yet support Linux dedicated server')
else:
    logger.warning(f'WARNING: LyTestTools only supports Windows and Mac, got HOST_OS_PLATFORM: "{HOST_OS_PLATFORM}".')
