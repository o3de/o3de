"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

# --runpythontest @devroot@\tests\hydra\ctests\open_level_tweak_and_exit.py
# An example of how a create a level, make an entity, and terminate successfully 

import time
import azlmbr.editor
import azlmbr.entity
import azlmbr.framework
import azlmbr.legacy.general as general
from azlmbr.bus import Broadcast

handler = None

def open_level(level):
    print('opening level {}'.format(level))
    azlmbr.editor.EditorToolsApplicationRequestBus(Broadcast, 'OpenLevelNoPrompt', level)
    general.idle_wait(1.0)

def on_entity_registered(args):
    print('on_entity_registered')
    azlmbr.framework.Terminate(0)

def main():
    print ('open_level_tweak_and_exit - starting')
    open_level('auto_test')
    
    azlmbr.editor.ToolsApplicationRequestBus(Broadcast, 'CreateNewEntity', azlmbr.entity.EntityId())
    general.idle_wait(1.0)

    handler = azlmbr.editor.ToolsApplicationNotificationBusHandler()
    handler.connect()
    handler.add_callback('EntityRegistered', on_entity_registered)
    azlmbr.editor.ToolsApplicationRequestBus(Broadcast, 'CreateNewEntity', azlmbr.entity.EntityId())

if __name__ == "__main__":
    main()
