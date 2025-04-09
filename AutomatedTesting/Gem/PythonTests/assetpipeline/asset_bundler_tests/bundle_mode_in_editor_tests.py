"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import azlmbr.bus
import azlmbr.editor
import azlmbr.legacy.general
import sys

# Print out the passed in bundle_path, so the outer test can verify this was sent in correctly
bundle_path = sys.argv[1]
print('Bundle mode test running with path {}'.format(sys.argv[1]))

# Turn on bundle mode. This will trigger some printouts that the outer test logic will validate.
azlmbr.legacy.general.set_cvar_integer("sys_report_files_not_found_in_paks", 1)
azlmbr.legacy.general.run_console(f"loadbundles {bundle_path}")

azlmbr.editor.EditorToolsApplicationRequestBus(azlmbr.bus.Broadcast, 'ExitNoPrompt')
