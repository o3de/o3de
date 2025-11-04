#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

import os
import json
import platform

from keystore_settings import KeystoreSettings

class ConfigData:
    """
    This class contains all the configuration parameters that are required
    to create a keystore for android applications and the configuration
    data that is required to create an android project for O3DE.
    """
    ASSET_MODE_LOOSE = "LOOSE"
    DEFAULT_NDK_VERSION = "25*"
    DEFAULT_API_LEVEL = "31" # The API level for Android 12.
    def __init__(self):
        self.engine_path = "" # Not serialized.
        self.project_path = "" # Not serialized.
        self.build_path = "" # Not serialized.
        self.third_party_path = "" # Not serialized.
        self.keystore_settings = KeystoreSettings()
        self.android_sdk_path = ""
        self.android_ndk_version = self.DEFAULT_NDK_VERSION
        self.android_sdk_api_level = self.DEFAULT_API_LEVEL
        self.asset_mode = self.ASSET_MODE_LOOSE
        self.is_meta_quest_project = False
        # When non-empty, looks like a series of space separated strings like
        # "-DENABLE_MY_LIB=ON -DENABLE_ANOTHER_GEM=OFF"
        self.extra_cmake_args = ""


    def as_dictionary(self) -> dict:
        d = { "keystore_settings" : self.keystore_settings.as_dictionary(),
              "android_sdk_path" : self.android_sdk_path,
              "android_ndk_version" : self.android_ndk_version,
              "android_sdk_api_level" : self.android_sdk_api_level,
              "asset_mode":  self.asset_mode,
              "is_meta_quest_project": self.is_meta_quest_project,
              "extra_cmake_args": self.extra_cmake_args,
              }
        return d


    def update_from_dictionary(self, d: dict):
        """
        Partially updates this object from a dictionary
        """
        self.keystore_settings = KeystoreSettings.from_dictionary(d["keystore_settings"])
        self.android_sdk_path = d.get("android_sdk_path", "")
        self.android_ndk_version = d.get("android_ndk_version", self.DEFAULT_NDK_VERSION)
        self.android_sdk_api_level = d.get("android_sdk_api_level", self.DEFAULT_API_LEVEL)
        self.asset_mode = d.get("asset_mode", self.ASSET_MODE_LOOSE)
        self.is_meta_quest_project = d.get("is_meta_quest_project", False)
        self.extra_cmake_args = d.get("extra_cmake_args", "")


    def save_to_json_file(self, filePath: str) -> bool:
        """
        @returns True if successful
        """
        try:
            with open(filePath, "w") as f:
                json.dump(self.as_dictionary(), f, indent=4)
        except Exception as err:
            print(f"ConfigData.save_to_json_file failed for file {filePath}.\nException: {err}")
            return False
        return True


    def load_from_json_file(self, filePath: str) -> bool:
        """
        Partially updates this object from a json file
        """
        try:
            with open(filePath, "r") as f:
                loaded_dict = json.load(f)
                self.update_from_dictionary(loaded_dict)
        except Exception as err:
            print(f"ConfigData.load_from_json_file failed for file {filePath}.\nException: {err}")
            return False
        return True


    def get_o3de_cmd(self) -> str:
        if platform.system() == "Windows":
            return os.path.join(self.engine_path, "scripts", "o3de.bat")
        else:
            return os.path.join(self.engine_path, "scripts", "o3de.sh")


    def get_project_name(self) -> str:
        return os.path.basename(self.project_path)
    

    def get_android_build_dir(self) -> str:
        return os.path.join(self.build_path, "android")
    
# class ConfigData END
######################################################
