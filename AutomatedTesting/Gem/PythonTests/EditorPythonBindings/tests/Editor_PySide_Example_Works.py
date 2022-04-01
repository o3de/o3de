"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
def check_result(result, msg):
    from editor_python_test_tools.utils import Report
    if not result:
        Report.result(msg, False)
        raise Exception(msg + " : FAILED")

def Editor_PySide_Example_Works():
    # Description: 
    # This test shows how to use PySide2 inside an Editor Python Bindings test.
    # For PySide details see automatedtesting_shared/pyside_utils.py and automatedtesting_shared/pyside_component_utils.py
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper
    import sys
    import os
    import PySide2

    sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../../automatedtesting_shared')

    import azlmbr.bus as bus
    import azlmbr.entity as entity
    import azlmbr.editor as editor
    import azlmbr.legacy.general as general
    import editor_python_test_tools.pyside_component_utils as pyside_component_utils

    # Required for automated tests
    TestHelper.init_idle()

    # Open the test level
    TestHelper.open_level(directory="", level="Base")
    azlmbr.legacy.general.idle_wait_frames(1)

    entityId = editor.ToolsApplicationRequestBus(bus.Broadcast, 'CreateNewEntity', entity.EntityId())
    check_result(entityId, 'New entity with no parent created')

    # Get Component Type for Environment Probe and attach to entity
    typenameList = ["Directional Light"]
    typeIdsList = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', typenameList, entity.EntityType().Game)
    componentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentsOfType', entityId, typeIdsList)
    check_result(componentOutcome.IsSuccess(), 'Directional Light component added to entity')

    # Waiting for one frame so that the widgets in the UI are updated with the new component information
    general.idle_enable(True)
    general.open_pane("Entity Inspector")
    general.idle_wait_frames(1)

    values = pyside_component_utils.get_component_combobox_values(typenameList[0], 'Intensity mode', print)
    check_result(len(values) > 0, f'ComboBox Values retrieved: {values}')

    # all tests worked
    Report.result("Editor_PySide_Example_Works ran", True)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Editor_PySide_Example_Works)
