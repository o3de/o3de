"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Tests the PythonProxyObject __str__ and __repr__


import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity as entity
import azlmbr.object

test_id = entity.EntityId()
test_id_repr = repr(test_id)

if test_id_repr.startswith('<EntityId via PythonProxyObject at'):
    print("repr() returned expected value")

test_id_str = str(test_id)
test_id_to_string = test_id.ToString()

print(test_id_str)
print(test_id_to_string)
if test_id_str == test_id_to_string:
    print("str() returned expected value")

editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'ExitNoPrompt')
