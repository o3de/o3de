#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
"""
This script loads every level in a game project and exports them.  You will be prompted with the standard
Check-Out/Overwrite/Cancel dialog for each level.
This is useful when there is a version update to a file that requires re-exporting.
"""
import sys, os
import azlmbr.legacy.general as general
import azlmbr.legacy.checkout_dialog as checkout_dialog


class CheckOutDialogEnableAll:
    """
    Helper class to wrap enabling the "Apply to all" checkbox in the CheckOutDialog.
    Guarantees that the old setting will be restored.
    To use, do:

    with CheckOutDialogEnableAll():
        # your code here
    """

    def __init__(self):
        self.old_setting = False

    def __enter__(self):
        self.old_setting = checkout_dialog.enable_for_all(True)
        print(self.old_setting)

    def __exit__(self, type, value, traceback):
        checkout_dialog.enable_for_all(self.old_setting)


level_list = []


def is_in_special_folder(file_full_path):
    if file_full_path.find("_savebackup") >= 0:
        return True
    if file_full_path.find("_autobackup") >= 0:
        return True
    if file_full_path.find("_hold") >= 0:
        return True
    if file_full_path.find("_tmpresize") >= 0:
        return True

    return False


game_folder = general.get_game_folder()


# Recursively search every directory in the game project for files ending with .cry and .ly
for root, dirs, files in os.walk(game_folder):
    for file in files:
        if file.endswith(".cry") or file.endswith(".ly"):
            # The engine expects the full path of the .cry file
            file_full_path = os.path.abspath(os.path.join(root, file))
            # Exclude files in special directories
            if not is_in_special_folder(file_full_path):
                level_list.append(file_full_path)

# make the checkout dialog enable the 'Apply to all' checkbox
with CheckOutDialogEnableAll():

    # For each valid .cry file found, open it in the editor and export it
    for level in level_list:
        if not isinstance(level, str):
            # general.open_level_no_prompt expects the file path in utf8 format
            level = level.encode("utf-8")

        general.open_level_no_prompt(level)
        general.export_to_engine()
