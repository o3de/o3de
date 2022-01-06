"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def GradientSurfaceTagEmitter_SurfaceTagsAddRemoveSuccessfully():
    """
    Summary:
    Entity with Gradient Surface Tag Emitter and Reference Gradient components is created.
    And new surface tag has been added and removed.

    Expected Behavior:
    A new Surface Tag can be added and removed from the component

    Test Steps:
    1) Open level
    2) Create an entity with Gradient Surface Tag Emitter and Reference Gradient components.
    3) Add/ remove Surface Tags

    Note:
    - This test file must be called from the Open 3D Engine Editor command terminal
    - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import azlmbr.math as math
    import azlmbr.surface_data as surface_data

    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    # 1) Open an existing simple level
    helper.init_idle()
    helper.open_level("Physics", "Base")

    # 2) Create an entity with Gradient Surface Tag Emitter and Reference Gradient components.
    entity_position = math.Vector3(125.0, 136.0, 32.0)
    components_to_add = ["Gradient Surface Tag Emitter", "Reference Gradient"]
    entity = hydra.Entity("entity")
    entity.create_entity(entity_position, components_to_add)

    # 3) Add/ remove Surface Tags
    tag = surface_data.SurfaceTag()
    tag.SetTag("water")
    pte = hydra.get_property_tree(entity.components[0])
    path = "Configuration|Extended Tags"
    pte.add_container_item(path, 0, tag)
    success = helper.wait_for_condition(lambda: pte.get_container_count(path).GetValue() == 1, 1.0)
    tag_added_to_container = (
        "Successfully added surface tag",
        "Failed to add surface tag"
    )
    Report.result(tag_added_to_container, success)
    pte.remove_container_item(path, 0)
    success = helper.wait_for_condition(lambda: pte.get_container_count(path).GetValue() == 0, 1.0)
    tag_removed_from_container = (
        "Successfully removed surface tag",
        "Failed to remove surface tag"
    )
    Report.result(tag_removed_from_container, success)


if __name__ == "__main__":

    from editor_python_test_tools.utils import Report
    Report.start_test(GradientSurfaceTagEmitter_SurfaceTagsAddRemoveSuccessfully)

