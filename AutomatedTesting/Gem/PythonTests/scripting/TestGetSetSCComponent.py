"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import azlmbr.math as math
import editor_python_test_tools.pyside_utils as pyside_utils
import editor_python_test_tools.hydra_editor_utils as hydra
import azlmbr.paths as paths


class TestGetSetSCComponent:

    @pyside_utils.wrap_async
    async def run_test(self):

        SOURCE_FILE_0 = os.path.join(paths.projectroot, "ScriptCanvas", "ScriptCanvas_TwoComponents0.scriptcanvas")
        source0 = azlmbr.scriptcanvas.SourceHandleFromPath(SOURCE_FILE_0)
        componentPropertyPath = "Configuration|Source"
        position = math.Vector3(512.0, 512.0, 32.0)

        test_entity2 = hydra.Entity("test_entity2")
        test_entity2.create_entity(position,  ["Script Canvas"])
        #script_canvas_component = test_entity2.add_component("Script Canvas")
        script_canvas_component = test_entity2.components[0]
        hydra.set_component_property_value(script_canvas_component, componentPropertyPath, source0)
        script_file = hydra.get_component_property_value(script_canvas_component, componentPropertyPath)

        print(script_file)


test = TestGetSetSCComponent()
test.run_test()
