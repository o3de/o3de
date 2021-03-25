"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

# --runpythontest @devroot@\tests\hydra\ctests\start_stop.py
# an example of a test script that loads a level, listens for the first entity, and terminates the Editor with a 0

import azlmbr.framework
import azlmbr.editor
import azlmbr.bus

handler = None

def on_entity_registered(args):
    print('on_entity_registered')
    azlmbr.framework.Terminate(0)

def main():
    print ('hello, start_stop')
    handler = azlmbr.editor.ToolsApplicationNotificationBusHandler()
    handler.connect()
    handler.add_callback('EntityRegistered', on_entity_registered)
    azlmbr.editor.EditorToolsApplicationRequestBus(azlmbr.bus.Broadcast, 'OpenLevelNoPrompt', 'auto_test')
    print ('start_stop started')

if __name__ == "__main__":
    main()
