"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID :  C28798205
# Test Case Title : From the White Box Component Card the White Box Mesh can be set to be invisible in Game View


# fmt:off
class Tests():
    white_box_initially_visible       = ("White box initially invisible",            "White box not initially invisible")
    white_box_set_to_invisible        = ("White box set to invisible",               "Failed to set white box to invisible")
# fmt:on


def C28798205_WhiteBox_SetInvisible():
    # note: This automated test does not fully replicate the test case in Test Rail as it's
    # not currently possible using the Hydra API to get an EntityComponentIdPair at runtime,
    # in future game_mode will be activated and a runtime White Box Component queried
    import os
    import sys
    from Gems.WhiteBox.Editor.Scripts import WhiteBoxInit as init
    import editor_python_test_tools.hydra_editor_utils as hydra

    import azlmbr.whitebox.api as api
    import azlmbr.whitebox
    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.entity as entity
    import azlmbr.legacy.general as general

    from editor_python_test_tools.utils import Report

    from editor_python_test_tools.utils import TestHelper as helper

    # open level
    helper.init_idle()
    general.open_level("EmptyLevel")

    white_box_entity_name = 'WhiteBox-Visibility'
    white_box_visibility_path = 'White Box Material|Visible'

    # create white box entity and attach component
    white_box_entity = init.create_white_box_entity(white_box_entity_name)
    white_box_mesh_component = init.create_white_box_component(white_box_entity)

    # verify preconditions
    visible_property = hydra.get_component_property_value(white_box_mesh_component, white_box_visibility_path)
    Report.result(Tests.white_box_initially_visible, visible_property)

    # verify setting visibility
    editor.EditorComponentAPIBus(bus.Broadcast, "SetComponentProperty", white_box_mesh_component, white_box_visibility_path, False)
    visible_property = hydra.get_component_property_value(white_box_mesh_component, white_box_visibility_path)
    Report.result(Tests.white_box_set_to_invisible, not visible_property)

    helper.close_editor()


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(C28798205_WhiteBox_SetInvisible)
