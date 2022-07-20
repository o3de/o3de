"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os, sys
sys.path.append(os.path.dirname(__file__))
from Editor_TestClass import BaseClass

class Editor_ComponentCommands_BuildComponentTypeNameList(BaseClass):
    # Description: 
    # Regression test for crash with BuildComponentTypeNameList command
    
    @staticmethod
    def test():
        import azlmbr.bus as bus
        import azlmbr.editor as editor
        import azlmbr.entity as entity

        componentList = editor.EditorComponentAPIBus(bus.Broadcast, 'BuildComponentTypeNameListByEntityType', entity.EntityType().Game)
        BaseClass.check_result(componentList is not None, "BuildComponentTypeNameList returned a valid list")
        BaseClass.check_result(len(componentList) > 0, "BuildComponentTypeNameList returned a non-empty list")

if __name__ == "__main__":
    tester = Editor_ComponentCommands_BuildComponentTypeNameList()
    tester.test_case(tester.test)
