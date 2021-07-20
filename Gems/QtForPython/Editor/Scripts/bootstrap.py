"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import sys
import os
import azlmbr
import azlmbr.bus


params = azlmbr.qt.QtForPythonRequestBus(azlmbr.bus.Broadcast, 'GetQtBootstrapParameters')

if len(params.qtPluginsFolder):
    # add the Qt plugins to the environment
    os.environ['QT_PLUGIN_PATH'] = params.qtPluginsFolder

# add Qt binaries to the Windows path to handle findings DLL file dependencies
if len(params.qtBinaryFolder) and sys.platform.startswith('win'):
    path = os.environ['PATH']
    newPath = ''
    newPath += params.qtBinaryFolder + os.pathsep
    newPath += path
    os.environ['PATH'] = newPath
    print('PySide2 bootstrapped PATH for Windows.')

    # Once PySide2 has been bootstrapped, register our Object Tree visualizer with the Editor
    try:
        import az_qt_helpers
        from show_object_tree import ObjectTreeDialog
        az_qt_helpers.register_view_pane('Object Tree', ObjectTreeDialog)
    except:
        print('Skipping register our Object Tree visualizer with the Editor.')

