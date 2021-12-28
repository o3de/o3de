"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    prefab_created = (
        "Prefab created successfully",
        "Failed to create prefab"
    )
    prefab_instantiated = (
        "Prefab instantiated successfully",
        "Failed to instantiate prefab"
    )


def Prefab_CreateInstantiate():
    """
    Summary:
    A prefab containing the LandscapeCanvas component can be created/instantiated.

    Expected Result:
    Prefab is created/processed/instantiated successfully and free of errors/warnings.

    Test Steps:
     1) Open a simple level
     2) Create a new entity with a Landscape Canvas component
     3) Create a slice of the new entity
     4) Instantiate a new copy of the slice

     Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.
    :return: None
    """

    import azlmbr.math as math

    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.prefab_utils import Prefab

    # Open an existing simple level
    hydra.open_base_level()

    # Create entity with Landscape Canvas component
    position = math.Vector3(512.0, 512.0, 32.0)
    landscape_canvas = hydra.Entity("landscape_canvas_entity")
    landscape_canvas.create_entity(position, ["Landscape Canvas"])

    # Create in-memory prefab from the created entity
    lc_prefab_filename = "LC_PrefabTest"
    lc_prefab, lc_prefab_instance = Prefab.create_prefab([landscape_canvas], lc_prefab_filename)

    # Verify if slice is created
    Report.result(Tests.prefab_created, lc_prefab.is_prefab_loaded(lc_prefab_filename))

    # Instantiate slice
    lc_prefab_instance2 = lc_prefab.instantiate()
    Report.result(Tests.prefab_instantiated, lc_prefab_instance2.is_valid())


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(Prefab_CreateInstantiate)
