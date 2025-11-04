#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

# Utility functions to discover system settings like Android SDK location, etc.
import os
import platform

def could_be_android_sdk_directory(sdk_path: str) -> bool:
    """
    @returns True if the provided @sdk_path could be an Android SDK directory.
             Will check for existence of a few directories that are typically located
             inside an Android SDK directory.
    """
    if not os.path.isdir(sdk_path):
        return False
    test_dir = os.path.join(sdk_path, "ndk")
    if os.path.isdir(test_dir):
        return True
    test_dir = os.path.join(sdk_path, "platform-tools")
    if os.path.isdir(test_dir):
        return True
    return False


def discover_android_sdk_path() -> str:
    """
    Follows common sense heuristic to find the location of the Android SDK.
    @returns absolute path as a string to the Android SDK directory.
    """
    var = os.getenv('ANDROID_HOME')
    if var and os.path.isdir(var):
        return var
    var = os.getenv('ANDROID_SDK_ROOT')
    if var and os.path.isdir(var):
        return var
    user_home_dir = os.path.expanduser("~")
    if platform.system() == "Windows":
        sdk_path = f"{user_home_dir}\\AppData\\Local\\Android\\Sdk"
        if os.path.isdir(sdk_path):
            return sdk_path
        sdk_system_path = "C:\\Program Files (x86)\\Android\\android-sdk"
        if os.path.isdir(sdk_system_path):
            return sdk_system_path
    else:
        # Linux or MacOS
        sdk_path = f"{user_home_dir}/Library/Android/sdk"
        if os.path.isdir(sdk_path):
            return sdk_path
    return ""

