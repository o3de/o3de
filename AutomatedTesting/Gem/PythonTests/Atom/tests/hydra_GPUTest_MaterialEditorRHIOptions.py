"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# import azlmbr.materialeditor will fail with a ModuleNotFound error when using this script with Editor.exe
# This is because azlmbr.materialeditor only binds to MaterialEditor.exe and not Editor.exe
# You need to launch this script with MaterialEditor.exe in order for azlmbr.materialeditor to appear.

import os
import sys

sys.path.append(os.path.join(azlmbr.paths.projectroot, "Gem", "PythonTests"))


def run():
    """
    Summary:
    This test is just used to ensure the MaterialEditor fully opens and finishes loading viewport configurations before
    it closes.

    Steps:
    1. Open an existing material.
    2. Close all documents open in the MaterialEditor.

    Expected Result:
    "Finished loading viewport configurations." prints to the log. Opening the material ensures we get a fully finished
    loading of the MaterialEditor process since that message prints when it finishes loading (not needed in test code).

    :return: None
    """
    import azlmbr.paths

    from Atom.atom_utils import material_editor_utils as material_editor

    material_type_path = os.path.join(
        azlmbr.paths.engroot, "Gems", "Atom", "Feature", "Common", "Assets",
        "Materials", "Types", "StandardPBR.materialtype",
    )

    # 1. Open an existing material.
    # The "Finished loading viewport configurations." will print once the viewport finishes loading.
    # We don't need to print it in the test.
    material_editor.open_material(material_type_path)

    # 2. Close all documents open in the MaterialEditor.
    material_editor.close_all_documents()


if __name__ == "__main__":
    run()
