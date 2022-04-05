"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os, sys
sys.path.append(os.path.dirname(__file__))
from Editor_TestClass import BaseClass

class Editor_ObjectStringRepresentation_Works(BaseClass):
    # Description: 
    # Tests the PythonProxyObject __str__ and __repr__
    
    @staticmethod
    def test():
        import azlmbr.bus as bus
        import azlmbr.editor as editor
        import azlmbr.entity as entity
        import azlmbr.object

        test_id = entity.EntityId()
        test_id_repr = repr(test_id)
        BaseClass.check_result(test_id_repr.startswith('<EntityId via PythonProxyObject'),  "repr() returned expected value")

        test_id_str = str(test_id)
        test_id_to_string = test_id.ToString()
        BaseClass.check_result(test_id_str == test_id_to_string, f"str() returned {test_id_str} {test_id_to_string}")

if __name__ == "__main__":
    tester = Editor_ObjectStringRepresentation_Works()
    tester.test_case(tester.test)
