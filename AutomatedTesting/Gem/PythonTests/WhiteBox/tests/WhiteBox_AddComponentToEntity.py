"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# Test case ID :  C28798177
# Test Case Title : White Box Tool Component can be added to an Entity


# fmt:off
class Tests():
    white_box_entity_created = ("White box entity created",                      "Failed to create white box entity")
    white_box_component_enabled = ("White box component enabled",                "Failed to enable white box component")
# fmt:on


def C28798177_WhiteBox_AddComponentToEntity():
    import os
    import sys
    from Gems.WhiteBox.Editor.Scripts import WhiteBoxInit as init

    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.legacy.general as general

    from editor_python_test_tools.utils import Report
    
    from editor_python_test_tools.utils import TestHelper as helper

    # open level
    helper.init_idle()
    general.open_level("EmptyLevel")

    # create white box entity and attach component
    white_box_entity = init.create_white_box_entity()
    white_box_mesh_component = init.create_white_box_component(white_box_entity)
    init.create_white_box_handle(white_box_mesh_component)

    # verify results
    entity_id = general.find_editor_entity('WhiteBox')
    Report.result(Tests.white_box_entity_created, entity_id.IsValid())

    component_enabled = editor.EditorComponentAPIBus(bus.Broadcast, 'IsComponentEnabled', white_box_mesh_component)
    Report.result(Tests.white_box_component_enabled, component_enabled)

    # close editor
    helper.close_editor()


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(C28798177_WhiteBox_AddComponentToEntity)
